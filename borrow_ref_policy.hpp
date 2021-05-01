#pragma once

#include <cassert>

namespace depman
{

class borrow_ref_policy
{
public:
    virtual ~borrow_ref_policy() = default;

    virtual void register_aliasing() = 0;

    virtual void unregister_aliasing() = 0;

    virtual void register_mutation() = 0;

    virtual void unregister_mutation() = 0;
};

class assert_borrow_ref_policy : public borrow_ref_policy
{
public:
    ~assert_borrow_ref_policy() override
    {
        assert(borrows_ == -2 || borrows_ == 0);
    }

    void register_aliasing() override
    {
        assert(borrows_ >= 0);
        borrows_++;
    }

    void unregister_aliasing() override
    {
        assert(borrows_ >= 1);
        borrows_--;
    }

    void register_mutation() override
    {
        assert(borrows_ == 0);
        borrows_ = -1;
    }

    void unregister_mutation() override
    {
        assert(borrows_ == -1);
        borrows_ = 0;
    }

private:
    // + aliasing borrows
    // 0 no borrow
    // -1 mutation borrow
    int borrows_;
};

};