#pragma once
#include <memory>

class non_copyable
{
protected:
    non_copyable(void) = default;
    virtual ~non_copyable(void) = default;

    non_copyable(non_copyable const &) = delete;
    void operator=(non_copyable const &) = delete;
};

template<class T>
class singleton : public non_copyable
{
public:
    using ptr = std::unique_ptr<T>;

    /// retrieve a reference to the singleton object, it MUST have been created beforehand by a call
    /// to 'create_singleton'. Otherwise, bad things will happen.
    static T* instance(void)
    {
        // Make sure to call 'create_singleton()' before accessing it!
        assert(nullptr != s_instance.get());
        return s_instance.get();
    }

    /// private API used to create/parametrize the singleton.
    template <typename... arg_types>
    static void create_singleton(arg_types... args)
    {
        // not allowed to call twice!
        assert(nullptr == s_instance.get());
        s_instance.reset(new T(std::forward<arg_types>(args)...));
    }


    /// private API used to free the singleton during shutdown processing.
    static void destroy_singleton(void)
    {
        s_instance.reset();
    }

private:
    static ptr s_instance;
};

template <class T> typename singleton<T>::ptr singleton<T>::s_instance;

