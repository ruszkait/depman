#include "borrow_checker.hpp"
#include "root_var.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

class FooI
{
public:
    virtual int a() const = 0;
    virtual FooI& a(int value) = 0;
};

class Foo : public FooI
{
public:
    Foo() 
        : a_(5)
    {}

    Foo(int a) 
        : a_(a)
    {}

    int a() const override { return a_; }
    Foo& a(int value) override { a_ = value; return *this; }

private:
    int a_;
};

void fn_mut(depman::ref<FooI> b)
{
    b->a(7);
    std::cout << b->a();

// mutable cannot be converted to immutable
//    borrow<const Foo> c2 = b;
}

void fn_immut(depman::ref<const FooI> b)
{
    std::cout << b->a();
    std::cout << b.value().a();

    depman::ref<const FooI> c = b;


// immutable cannot be converted to mutable
//    borrow<Foo> c2 = b;
}

void simple_test()
{
    depman::ref_cell<Foo> f2(depman::make_ref_cell<Foo>(4));
    // var is copyable - if the underlying type supports it
    depman::ref_cell<Foo> f3 = f2;
    // var is movable - if the underlying type supports it
    depman::ref_cell<Foo> f4 = std::move(f2);

    {
        depman::ref<const FooI> f4b = f4;
        std::cout << f4b->a();

    /* active aliasing prevents mutation
        boche::borrow<FooI> f4c = f4;
        f4c->a(4);
        std::cout << f4c->a();
        */
    }

    // Temporal aliasing borrow
    std::cout << f4.borrow()->a();

    // Implicit temporal borrows
    fn_mut(f4);
    fn_immut(f3);
}

void arc_test()
{
    auto f2 = std::make_shared<depman::ref_cell<Foo>>(depman::make_ref_cell<Foo>(4));
    auto f3 = f2;

    fn_mut(*f2);
    fn_immut(*f3);
}

void root_var_AssertPolicy_test()
{
    struct System
    {
        System()
        : _foo(std::make_unique<depman::AssertDestructionPolicy>(), 5)
        {}

        depman::root_var<Foo> _foo;
    };

    auto system = std::make_unique<System>();

    auto foo = system->_foo.get();
    system.reset();
//    foo.reset();
}

void root_var_WaitPolicy_test()
{
    struct System
    {
        System()
        : _foo(std::make_unique<depman::WaitingDestructionPolicy>(), 5)
        {}

        depman::root_var<Foo> _foo;
    };

    auto system = std::make_unique<System>();

    std::thread t([foo = system->_foo.get()]() mutable { 
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(3s);
            foo.reset();
        });
    system.reset();

    t.join();
}

int main()
{
    //simple_test();
    //arc_test();
    //root_var_AssertPolicy_test();
    root_var_WaitPolicy_test();
    return 0;
}