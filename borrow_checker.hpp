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
class ref
{
public:
    ref(std::remove_const_t<T>* owned, borrow_ref_policy* ref_policy)
        : ref_(owned)
        , ref_policy_(ref_policy)
    {
        ref_policy_->register_mutation();
    }

    ref(const ref& other);
    ref& operator=(const ref& other) = delete;

    ref(ref&& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        other.ref_ = nullptr;
        other.ref_policy_ = nullptr;
    }

    ref& operator=(ref&& other) = delete;

    ~ref()
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
class ref<const T>
{
public:
    ref(std::add_const_t<T>* owned, borrow_ref_policy* ref_policy)
        : ref_(owned)
        , ref_policy_(ref_policy)
    {
        ref_policy_->register_aliasing();
    }

    ref(const ref& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        ref_policy_->register_aliasing();
    }

    ref& operator=(const ref& other) = delete;

    ref(ref&& other)
        : ref_(other.ref_)
        , ref_policy_(other.ref_policy_)
    {
        other.ref_ = nullptr;
        other.ref_policy_ = nullptr;
    }

    ref& operator=(ref&& other) = delete;

    ~ref()
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
class ref_cell
{
public:
    ref_cell(const ref_cell &other)
        : ref_policy_(other.ref_policy_)
    {
        const bool valid = ref_policy_.has_value();
        if (valid)
        {
            auto other_owned = reinterpret_cast<const T&>(other.owned_buffer_);
            new (&owned_buffer_) T(other_owned);
        }
    }

    ref_cell(ref_cell&& other)
        : ref_policy_(std::move(other.ref_policy_))
    {
        auto& other_owned = reinterpret_cast<T&>(other.owned_buffer_);
        new (&owned_buffer_) T(std::move(other_owned));

        other.ref_policy_.reset();
    }

    ref<const T> borrow()
    {
        return this->operator ref<const T>();
    }

    ref<T> borrow_mut()
    {
        return this->operator ref<T>();
    }

    template<typename D, typename std::enable_if_t<std::is_const_v<D>>* = nullptr>
    operator ref<D>()
    {
        assert(ref_policy_);
        auto borrowed = reinterpret_cast<D*>(&owned_buffer_);
        borrow_ref_policy* ref_manager = &ref_policy_.value();
        return ref<D>(borrowed, ref_manager);
    }

    template<typename D, typename std::enable_if_t<!std::is_const_v<D>>* = nullptr>
    operator ref<D>()
    {
        assert(ref_policy_);
        auto borrowed = reinterpret_cast<D*>(&owned_buffer_);
        borrow_ref_policy* ref_manager = &ref_policy_.value();
        return ref<D>(borrowed, ref_manager);
    }

private:
    using ref_policy_type = RefPolicy;
    ref_cell(RefPolicy&& ref_policy)
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
    friend ref_cell<T> make_ref_cell(Args&&... args);
};

template<typename T, typename... Args>
ref_cell<T> make_ref_cell(Args&&... args)
{
    auto ref_policy = ref_cell<T>::ref_policy_type();
    ref_cell<T> new_ref_cell(std::move(ref_policy));
    new_ref_cell.create_owned(std::forward<Args>(args)...);
    return new_ref_cell;
}

};