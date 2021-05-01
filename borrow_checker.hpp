#pragma once

#include "borrow_ref_policy.hpp"

#include <array>
#include <optional>
#include <utility>
#include <type_traits>
#include <cassert>

namespace depman
{

template<typename T>
class borrow
{
public:
    borrow(std::remove_const_t<T>* owned, borrow_ref_policy* ref_policy)
        : ref_(owned)
        , ref_policy_(ref_policy)
    {
        ref_policy_->register_mutation();
    }

    borrow(const borrow& other);
    borrow& operator=(const borrow& other) = delete;

    borrow(borrow&& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        other.ref_ = nullptr;
        other.ref_policy_ = nullptr;
    }

    borrow& operator=(borrow&& other) = delete;

    ~borrow()
    {
        ref_policy_->unregister_mutation();
    }

    std::remove_const_t<T>* operator*()
    {
        assert(ref_ != nullptr);
        return ref_;
    }

    std::remove_const_t<T>* operator->()
    {
        assert(ref_ != nullptr);
        return ref_;
    }

    std::remove_const_t<T>& value()
    {
        assert(ref_ != nullptr);
        return *ref_;
    }

private:
    std::remove_const_t<T> *ref_;
    borrow_ref_policy *ref_policy_;
};

template<typename T>
class borrow<const T>
{
public:
    borrow(std::add_const_t<T>* owned, borrow_ref_policy* ref_policy)
        : ref_(owned)
        , ref_policy_(ref_policy)
    {
        ref_policy_->register_aliasing();
    }

    borrow(const borrow& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        ref_policy_->register_aliasing();
    }

    borrow& operator=(const borrow& other) = delete;

    borrow(borrow&& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        other.ref_ = nullptr;
        other.ref_policy_ = nullptr;
    }

    borrow& operator=(borrow&& other) = delete;

    ~borrow()
    {
        ref_policy_->unregister_aliasing();
    }

    std::add_const_t<T>* operator*()
    {
        assert(ref_ != nullptr);
        return ref_;
    }

    std::add_const_t<T>* operator->()
    {
        assert(ref_ != nullptr);
        return ref_;
    }

    std::add_const_t<T>& value()
    {
        assert(ref_ != nullptr);
        return *ref_;
    }

private:
    std::add_const_t<T> *ref_;
    borrow_ref_policy *ref_policy_;
};


template<typename T, typename RefPolicy = assert_borrow_ref_policy>
class var
{
public:
    var(const var &other)
        : ref_policy_(other.ref_policy_)
    {
        const bool valid = ref_policy_.has_value();
        if (valid)
        {
            auto other_owned = reinterpret_cast<const T&>(other.owned_buffer_);
            new (&owned_buffer_) T(other_owned);
        }
    }

    var(var&& other)
        : ref_policy_(std::move(other.ref_policy_))
    {
        auto& other_owned = reinterpret_cast<T&>(other.owned_buffer_);
        new (&owned_buffer_) T(std::move(other_owned));

        other.ref_policy_.reset();
    }

    borrow<const T> get()
    {
        return this->operator borrow<const T>();
    }

    borrow<T> get_mut()
    {
        return this->operator borrow<T>();
    }

    template<typename D, typename std::enable_if_t<std::is_const_v<D>>* = nullptr>
    operator borrow<D>()
    {
        assert(ref_policy_);
        auto borrowed = reinterpret_cast<D*>(&owned_buffer_);
        borrow_ref_policy* ref_manager = &ref_policy_.value();
        return borrow<D>(borrowed, ref_manager);
    }

    template<typename D, typename std::enable_if_t<!std::is_const_v<D>>* = nullptr>
    operator borrow<D>()
    {
        assert(ref_policy_);
        auto borrowed = reinterpret_cast<D*>(&owned_buffer_);
        borrow_ref_policy* ref_manager = &ref_policy_.value();
        return borrow<D>(borrowed, ref_manager);
    }

private:
    using ref_policy_type = RefPolicy;
    var(RefPolicy&& ref_policy)
        : ref_policy_(std::move(ref_policy))
    {}

    template<typename... Args>
    void create_owned(Args&&... args)
    {
        new (&owned_buffer_) T(std::forward<Args>(args)...);
    }

    std::optional<RefPolicy> ref_policy_;
    std::array<std::uint8_t, sizeof(T)> owned_buffer_;

    template<typename T>
    friend class borrow;

    template<typename T, typename... Args>
    friend var<T> make_var(Args&&... args);
};

template<typename T, typename... Args>
var<T> make_var(Args&&... args)
{
    auto ref_policy = var<T>::ref_policy_type();
    var<T> new_borrowable(std::move(ref_policy));
    new_borrowable.create_owned(std::forward<Args>(args)...);
    return new_borrowable;
}

};