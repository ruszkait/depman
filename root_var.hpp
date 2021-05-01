#include <memory>
#include <future>
#include <cassert>
#include <atomic>

namespace depman
{

class DestructionPolicy
{
public:
    virtual void variable_deleted() = 0;
    virtual void destruction_check() = 0;
};

class WaitingDestructionPolicy : public DestructionPolicy
{
public:
    WaitingDestructionPolicy()
    : _var_destroyed_future(_var_destroyed_promise.get_future())
    {}

    void variable_deleted() override
    {
        _var_destroyed_promise.set_value();
    }

    void destruction_check() override
    {
        _var_destroyed_future.wait();
    }

private:
    std::promise<void> _var_destroyed_promise;
    std::future<void> _var_destroyed_future;
};

class AssertDestructionPolicy : public DestructionPolicy
{
public:
    AssertDestructionPolicy()
    : _variable_deleted(false)
    {}

    void variable_deleted() override
    {
        _variable_deleted = true;
    }

    void destruction_check() override
    {
        assert(_variable_deleted);
    }

private:
    std::atomic<bool> _variable_deleted;
};

template<typename T>
class root_var
{
public:
    template<typename... Args>
    root_var(std::unique_ptr<DestructionPolicy> policy, Args&&... args)
        : _destruction_policy(std::move(policy))
        , _var(new T(std::forward<Args>(args)...), [this](T* var){ destroy_object(var); })
    {
    }

    root_var& custom_deleter(std::function<void(T*)> deleter)
    {
        _deleter = deleter;
    }

    ~root_var()
    {
        _var.reset();
        _destruction_policy->destruction_check();
    }

    std::shared_ptr<T> get()
    {
        return _var;
    }

private:
    void destroy_object(T* to_delete)
    {
        if (_deleter)
            _deleter(to_delete);
        else
            delete to_delete;

        _destruction_policy->variable_deleted();
    }

    std::unique_ptr<DestructionPolicy> _destruction_policy;
    std::shared_ptr<T> _var;
    std::function<void(T*)> _deleter;
};

}