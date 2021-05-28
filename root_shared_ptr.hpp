#include <memory>
#include <future>
#include <cassert>
#include <atomic>

namespace depman
{

// When the root_var is destroyed, what shall it do if the owned object is still alive
class DestructionPolicy
{
public:
    virtual void variable_deleted() = 0;
    virtual void destruction_check() = 0;
};

// When the root_var is destroyed, but the owned object is still alive,
// the destructor ot the root_var waits until the owned object is destroyed
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

// When the root_var is destroyed, but the owned object is still alive,
// the destructor ot the root_var terminates the program
class TerminateDestructionPolicy : public DestructionPolicy
{
public:
    TerminateDestructionPolicy()
    : _variable_deleted(false)
    {}

    void variable_deleted() override
    {
        _variable_deleted = true;
    }

    void destruction_check() override
    {
        if (!_variable_deleted)
            std::terminate();
    }

private:
    std::atomic<bool> _variable_deleted;
};

// Master shared pointer, when destroyed it expect all subordinate shared_ptr-s to be destroyed
template<typename T>
class root_shared_ptr
{
public:
    class PolicyBuilder
    {
    public:
        PolicyBuilder& premature_destruction_terminates()
        {
            destruction_policy_ = std::make_unique<WaitingDestructionPolicy>();
            return *this;
        }

        PolicyBuilder& premature_destruction_waits()
        {
            destruction_policy_ = std::make_unique<TerminateDestructionPolicy>();
            return *this;
        }

        // setter: custom deleter for the owned object
        PolicyBuilder& custom_deleter(std::function<void(T*)> deleter)
        {
            deleter_ = deleter;
            return *this;
        }

        std::unique_ptr<DestructionPolicy> destruction_policy_;
        std::function<void(T*)> deleter_;
    };

    template<typename... Args>
    root_shared_ptr(PolicyBuilder& policy_builder, Args&&... args)
        : destruction_policy_(std::move(policy_builder.destruction_policy_))
        , var_(new T(std::forward<Args>(args)...), [this](T* var){ destroy_object(var); })
        , deleter_(std::move(policy_builder.deleter_))
    {
    }

    ~root_shared_ptr()
    {
        var_.reset();
        destruction_policy_->destruction_check();
    }

    // The owned object
    std::shared_ptr<T> get() const
    {
        return var_;
    }

    operator std::shared_ptr<T>() const
    {
        return get();
    }


private:
    void destroy_object(T* to_delete)
    {
        if (deleter_)
            deleter_(to_delete);
        else
            delete to_delete;

        destruction_policy_->variable_deleted();
    }

    std::unique_ptr<DestructionPolicy> destruction_policy_;
    std::shared_ptr<T> var_;
    std::function<void(T*)> deleter_;
};

}