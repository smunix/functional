// -*- C++ -*-
// Inspired by LLVM compiler implementation of libcxx

#pragma once

#include <type_traits>
#include <typeinfo>
#include <exception>
#include <memory>
#include <tuple>
#include <boost/compressed_pair.hpp>

namespace mstd {
  // bad_function_call
  class bad_function_call : public std::exception {};
  template<class Fp> class function;

  namespace details {
    struct any { any(...); };

    struct undefined {
      undefined() = delete;
      undefined(const undefined&) = delete;
      undefined& operator=(const undefined&) = delete;
      ~undefined() = delete;
    };
  } // details

  namespace details {
    namespace function {
      template<class Rp>
      struct maybe_derive_from_unary_function
      {};
      template<class Rp, class A1>
      struct maybe_derive_from_unary_function<Rp(A1)> : public std::unary_function<A1, Rp>
      {};
      template<class Rp>
      struct maybe_derive_from_binary_function
      {};
      template<class Rp, class A1, class A2>
      struct maybe_derive_from_binary_function<Rp(A1, A2)> : public std::binary_function<A1, A2, Rp>
      {};

      template <class Fp>
      bool not_null(Fp const&) { return true; }
      template <class Fp>
      bool not_null(Fp* ptr) { return ptr; }
      template <class _Ret, class _Class>
      bool not_null(_Ret _Class::*ptr) { return ptr; }
      template <class Fp>
      bool not_null(mstd::function<Fp> const& f) { return !!f; }
    } // function
  } // details

  namespace details {
    namespace alloc {
      template <class _Traits, class _Tp> struct rebind_helper {
        using type = typename _Traits::template rebind_alloc<_Tp>;
      };

      template <class _Alloc> class allocator_destructor {
        typedef std::allocator_traits<_Alloc> __alloc_traits;
      public:
        typedef typename __alloc_traits::pointer pointer;
        typedef typename __alloc_traits::size_type size_type;
      private:
        _Alloc& __alloc_;
        size_type __s_;
      public:
        allocator_destructor(_Alloc& __a, size_type __s) noexcept : __alloc_(__a), __s_(__s) {}
        void operator()(pointer __p) noexcept { __alloc_traits::deallocate(__alloc_, __p, __s_); }
      };
    } // alloc
  } // details


  namespace details {
    namespace check {
      template <class ..._Tp> struct complete;
      template <> struct complete<> {};

      template <class _Hp, class _T0, class ..._Tp> struct complete<_Hp, _T0, _Tp...> : private complete<_Hp>,  private complete<_T0, _Tp...> {};

      template <class _Hp> struct complete<_Hp, _Hp> : private complete<_Hp> {};

      template <class _Tp> struct complete<_Tp> {
        static_assert(sizeof(_Tp) > 0, "Type is not complete.");
      };

      template <class _Tp> struct complete<_Tp&> : private complete<_Tp> {};

      template <class _Tp> struct complete<_Tp&&> : private complete<_Tp> {};

      template <class _Rp, class ..._Param> struct complete<_Rp (*)(_Param...)> : private complete<_Rp> {};

      template <class ..._Param> struct complete<void (*)(_Param...)> {};

      template <class _Rp, class ..._Param> struct complete<_Rp (_Param...)> : private complete<_Rp> {};

      template <class ..._Param> struct complete<void (_Param...)> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...)> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) volatile> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const volatile> : private complete<_Class> {};

#if 1

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) &> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) volatile&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const volatile&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) &&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const&&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) volatile&&> : private complete<_Class> {};

      template <class _Rp, class _Class, class ..._Param> struct complete<_Rp (_Class::*)(_Param...) const volatile&&> : private complete<_Class> {};
#endif

      template <class _Rp, class _Class> struct complete<_Rp _Class::*> : private complete<_Class> {};
    } // check
  } // details

  namespace details {
    namespace invoke {
      template <class _Fp, class _A0, class ..._Args, class>
      inline
      auto call(_Fp&& __f, _A0&& __a0, _Args&& ...__args) -> decltype((std::forward<_A0>(__a0).*__f)(std::forward<_Args>(__args)...)) {
        return (std::forward<_A0>(__a0).*__f)(std::forward<_Args>(__args)...);
      }

      template <class _Fp, class _A0, class ..._Args, class>
      inline
      auto call(_Fp&& __f, _A0&& __a0, _Args&& ...__args) -> decltype(((*std::forward<_A0>(__a0)).*__f)(std::forward<_Args>(__args)...)) {
        return ((*std::forward<_A0>(__a0)).*__f)(std::forward<_Args>(__args)...);
      }

      template <class _Fp, class _A0, class>
      inline
      auto call(_Fp&& __f, _A0&& __a0) -> decltype(std::forward<_A0>(__a0).*__f) {
        return std::forward<_A0>(__a0).*__f;
      }

      template <class _Fp, class _A0, class>
      inline auto call(_Fp&& __f, _A0&& __a0) -> decltype((*std::forward<_A0>(__a0)).*__f) {
        return (*std::forward<_A0>(__a0)).*__f;
      }

      template <class _Fp, class ..._Args>
      inline
      auto call(_Fp&& __f, _Args&& ...__args) -> decltype(std::forward<_Fp>(__f)(std::forward<_Args>(__args)...)) {
        return std::forward<_Fp>(__f)(std::forward<_Args>(__args)...);
      }
      template <class _Tp, class ..._Args> struct __invoke_return {
        typedef decltype(call(std::declval<_Tp>(), std::declval<_Args>()...)) type;
      };

      template <class _Ret> struct void_return_wrapper {
        template <class ..._Args> static _Ret call(_Args&&... __args) {
          return call(std::forward<_Args>(__args)...);
        }
      };

      template <> struct void_return_wrapper<void> {
        template <class ..._Args> static void call(_Args&&... __args) {
          call(std::forward<_Args>(__args)...);
        }
      };
    } // invoke
  } //details

  namespace details {
    namespace traits {
      template <class _MP, bool _IsMemberFunctionPtr, bool _IsMemberObjectPtr> struct member_pointer_imp {};

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...), true, false> {
        typedef _Class _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...), true, false> {
        typedef _Class _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const, true, false> {
        typedef _Class const _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const, true, false> {
        typedef _Class const _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) volatile, true, false> {
        typedef _Class volatile _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) volatile, true, false> {
        typedef _Class volatile _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const volatile, true, false> {
        typedef _Class const volatile _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const volatile, true, false> {
        typedef _Class const volatile _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

#if 1
      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) &, true, false> {
        typedef _Class& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) &, true, false> {
        typedef _Class& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const&, true, false> {
        typedef _Class const& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const&, true, false> {
        typedef _Class const& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) volatile&, true, false> {
        typedef _Class volatile& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) volatile&, true, false> {
        typedef _Class volatile& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const volatile&, true, false> {
        typedef _Class const volatile& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const volatile&, true, false> {
        typedef _Class const volatile& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) &&, true, false> {
        typedef _Class&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) &&, true, false> {
        typedef _Class&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const&&, true, false> {
        typedef _Class const&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const&&, true, false> {
        typedef _Class const&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) volatile&&, true, false> {
        typedef _Class volatile&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) volatile&&, true, false> {
        typedef _Class volatile&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param...) const volatile&&, true, false> {
        typedef _Class const volatile&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param...);
      };

      template <class _Rp, class _Class, class ..._Param>
      struct member_pointer_imp<_Rp (_Class::*)(_Param..., ...) const volatile&&, true, false> {
        typedef _Class const volatile&& _ClassType;
        typedef _Rp _ReturnType;
        typedef _Rp (_FnType) (_Param..., ...);
      };

#endif  // 1 

      template <class _Rp, class _Class>
      struct member_pointer_imp<_Rp _Class::*, false, true> {
        typedef _Class _ClassType;
        typedef _Rp _ReturnType;
      };

      template <class _MP>
      struct member_pointer : public member_pointer_imp<typename std::remove_cv<_MP>::type,
                                                        std::is_member_function_pointer<_MP>::value,
                                                        std::is_member_object_pointer<_MP>::value> {
        //     typedef ... _ClassType;
        //     typedef ... _ReturnType;
        //     typedef ... _FnType;
      };
    } // traits
  } // details

  namespace details {
    namespace traits {
      template <class ..._Args> auto call(any, _Args&& ...__args) -> undefined;

      template <class _Fp, class _A0, class ..._Args,
                class = typename std::enable_if
                <
                  std::is_member_function_pointer<typename std::remove_reference<_Fp>::type>::value and
                  std::is_base_of<typename std::remove_reference<typename traits::member_pointer<typename std::remove_reference<_Fp>::type>::_ClassType>::type,
                             typename std::remove_reference<_A0>::type>::value
                  >::type
                >
        inline
        auto
        call(_Fp&& __f, _A0&& __a0, _Args&& ...__args) -> decltype((std::forward<_A0>(__a0).*__f)(std::forward<_Args>(__args)...));

      template <class _Fp, class _A0, class ..._Args,
                class = typename std::enable_if
                <
                  std::is_member_function_pointer<typename std::remove_reference<_Fp>::type>::value and
                  not std::is_base_of<typename std::remove_reference<typename traits::member_pointer<typename std::remove_reference<_Fp>::type>::_ClassType>::type,
                                 typename std::remove_reference<_A0>::type>::value
                  >::type
                >
        inline
        auto
        call(_Fp&& __f, _A0&& __a0, _Args&& ...__args) -> decltype(((*std::forward<_A0>(__a0)).*__f)(std::forward<_Args>(__args)...));

      template <class _Fp, class _A0,
                class = typename std::enable_if
                <
                  std::is_member_object_pointer<typename std::remove_reference<_Fp>::type>::value and
                  std::is_base_of<typename traits::member_pointer<typename std::remove_reference<_Fp>::type>::_ClassType,
                                  typename std::remove_reference<_A0>::type>::value
                  >::type
                >
        inline
        auto
        call(_Fp&& __f, _A0&& __a0) -> decltype(std::forward<_A0>(__a0).*__f);

      template <class _Fp, class _A0,
                class = typename std::enable_if
                <
                  std::is_member_object_pointer<typename std::remove_reference<_Fp>::type>::value and
                  not std::is_base_of<typename traits::member_pointer<typename std::remove_reference<_Fp>::type>::_ClassType,
                                 typename std::remove_reference<_A0>::type>::value
                  >::type
                >
        inline
        auto
        call(_Fp&& __f, _A0&& __a0) -> decltype((*std::forward<_A0>(__a0)).*__f);

      template <class _Fp, class ..._Args>
      inline
      auto
      call(_Fp&& __f, _Args&& ...__args)
        -> decltype(std::forward<_Fp>(__f)(std::forward<_Args>(__args)...));

      // callable

      template <class _Fp, class ..._Args> struct callable_imp : private check::complete<_Fp> {
        typedef decltype(
                         call(std::declval<_Fp>(), std::declval<_Args>()...)
                         ) type;
        static const bool value = not std::is_same<type, undefined>::value;
      };

      template <class _Fp, class ..._Args> struct callable : public std::integral_constant<bool, callable_imp<_Fp, _Args...>::value> {};

      // call_of

      template <bool _Callable, class _Fp, class ..._Args> struct call_of_imp {};

      template <class _Fp, class ..._Args> struct call_of_imp<true, _Fp, _Args...> {
        typedef typename callable_imp<_Fp, _Args...>::type type;
      };

      template <class _Fp, class ..._Args> struct call_of : public call_of_imp<callable<_Fp, _Args...>::value, _Fp, _Args...> {};

    } // traits
  } // details

  namespace details {
    namespace function {
      template<class Fp> class base;
      template<class A, class B> using compressed_pair = boost::compressed_pair<A, B>;

      template<class Rp, class ...ArgTypes>
      class base<Rp(ArgTypes...)>
      {
        base(const base&);
        base& operator=(const base&);
      public:
        base() {}
        virtual ~base() {}
        virtual base* clone() const = 0;
        virtual void clone(base*) const = 0;
        virtual void destroy() noexcept = 0;
        virtual void destroy_deallocate() noexcept = 0;
        virtual Rp operator()(ArgTypes&& ...) = 0;
      };

      template<class _FD, class _Alloc, class _FB> class func;

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      class func<Fp, _Alloc, Rp(ArgTypes...)> : public base<Rp(ArgTypes...)> {
        compressed_pair<Fp, _Alloc> __f_;
      public:
        explicit func(Fp&& __f) : __f_(std::move(__f)) {}
        explicit func(const Fp& __f, const _Alloc& __a) : __f_(__f, __a) {}
        explicit func(const Fp& __f, _Alloc&& __a) : __f_(__f, std::move(__a)) {}
        explicit func(Fp&& __f, _Alloc&& __a) : __f_(std::move(__f), std::move(__a)) {}
        virtual base<Rp(ArgTypes...)>* clone() const;
        virtual void clone(base<Rp(ArgTypes...)>*) const;
        virtual void destroy() noexcept;
        virtual void destroy_deallocate() noexcept;
        virtual Rp operator()(ArgTypes&& ... __arg);
      };

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      base<Rp(ArgTypes...)>*
      func<Fp, _Alloc, Rp(ArgTypes...)>::clone() const {
        using __alloc_traits = std::allocator_traits<_Alloc>;
        using _Ap = typename alloc::rebind_helper<__alloc_traits, func>::type;
        _Ap __a(__f_.second());
        using _Dp = alloc::allocator_destructor<_Ap>;
        std::unique_ptr<func, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
        ::new (__hold.get()) func(__f_.first(), _Alloc(__a));
        return __hold.release();
      }

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      void
      func<Fp, _Alloc, Rp(ArgTypes...)>::clone(base<Rp(ArgTypes...)>* __p) const {
        ::new (__p) func(__f_.first(), __f_.second());
      }

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      void
      func<Fp, _Alloc, Rp(ArgTypes...)>::destroy() noexcept {
        __f_.~compressed_pair<Fp, _Alloc>();
      }

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      void
      func<Fp, _Alloc, Rp(ArgTypes...)>::destroy_deallocate() noexcept {
        typedef std::allocator_traits<_Alloc> __alloc_traits;
        typedef typename alloc::rebind_helper<__alloc_traits, func>::type _Ap;
        _Ap __a(__f_.second());
        __f_.~compressed_pair<Fp, _Alloc>();
        __a.deallocate(this, 1);
      }

      template<class Fp, class _Alloc, class Rp, class ...ArgTypes>
      Rp
      func<Fp, _Alloc, Rp(ArgTypes...)>::operator()(ArgTypes&& ... __arg) {
        typedef invoke::void_return_wrapper<Rp> Caller;
        return Caller::template call(__f_.first(), std::forward<ArgTypes>(__arg)...);
      }
    } // function
  } // details

  // function
  template<class Rp, class ...ArgTypes>
  class function<Rp(ArgTypes...)> : public details::function::maybe_derive_from_unary_function<Rp(ArgTypes...)>,
                                    public details::function::maybe_derive_from_binary_function<Rp(ArgTypes...)> {
    typedef details::function::base<Rp(ArgTypes...)> base;
    typename std::aligned_storage<3*sizeof(void*)>::type __buf_;
    base* __f_;

    template <class Fp, bool = not std::is_same<Fp, function>::value and details::traits::callable<Fp&, ArgTypes...>::value> struct __callable;

    template <class Fp> struct __callable<Fp, true> {
      static const bool value = std::is_same<void, Rp>::value or std::is_convertible<typename details::traits::call_of<Fp&, ArgTypes...>::type, Rp>::value;
    };
    template <class Fp> struct __callable<Fp, false> {
      static const bool value = false;
    };
  public:
    typedef Rp result_type;

    // construct/copy/destroy:
    function() noexcept : __f_(0) {}
    function(std::nullptr_t) noexcept : __f_(0) {}
    function(const function&);
    function(function&&) noexcept;
    template<class Fp> function(Fp, typename std::enable_if <__callable<Fp>::value and not std::is_same<Fp, function>::value>::type* = 0);
    template<class _Alloc> function(std::allocator_arg_t, const _Alloc&) noexcept : __f_(0) {}
    template<class _Alloc> function(std::allocator_arg_t, const _Alloc&, std::nullptr_t) noexcept : __f_(0) {}
    template<class _Alloc> function(std::allocator_arg_t, const _Alloc&, const function&);
    template<class _Alloc> function(std::allocator_arg_t, const _Alloc&, function&&);
    template<class Fp, class _Alloc> function(std::allocator_arg_t, const _Alloc& __a, Fp __f, typename std::enable_if<__callable<Fp>::value>::type* = 0);

    function& operator=(const function&);
    function& operator=(function&&) noexcept;
    function& operator=(std::nullptr_t) noexcept;
    template<class Fp>
    typename std::enable_if<__callable<typename std::decay<Fp>::type>::value and not std::is_same<typename std::remove_reference<Fp>::type, function>::value, function&>::type
    operator=(Fp&&);
    ~function();

    // function modifiers:
    void swap(function&) noexcept;
    template<class Fp, class _Alloc> void assign(Fp&& __f, const _Alloc& __a) {
      function(std::allocator_arg, __a, std::forward<Fp>(__f)).swap(*this);
    }

    // function capacity:
    explicit operator bool() const noexcept { return __f_; }

    // deleted overloads close possible hole in the type system
    template<class _R2, class... ArgTypes2>
      bool operator==(const function<_R2(ArgTypes2...)>&) const = delete;
    template<class _R2, class... ArgTypes2>
      bool operator!=(const function<_R2(ArgTypes2...)>&) const = delete;
  public:
    // function invocation:
    Rp operator()(ArgTypes...) const;
  };
} // mstd

namespace mstd {
  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>::function(const function& __f)
  {
    if (__f.__f_ == 0)
      __f_ = 0;
    else if (__f.__f_ == (const base*)&__f.__buf_)
      {
        __f_ = (base*)&__buf_;
        __f.__f_->clone(__f_);
      }
    else
      __f_ = __f.__f_->clone();
  }

  template<class Rp, class ...ArgTypes>
  template <class _Alloc>
  function<Rp(ArgTypes...)>::function(std::allocator_arg_t, const _Alloc&,
                                      const function& __f)
  {
    if (__f.__f_ == 0)
      __f_ = 0;
    else if (__f.__f_ == (const base*)&__f.__buf_)
      {
        __f_ = (base*)&__buf_;
        __f.__f_->clone(__f_);
      }
    else
      __f_ = __f.__f_->clone();
  }

  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>::function(function&& __f) noexcept
  {
    if (__f.__f_ == 0)
      __f_ = 0;
    else if (__f.__f_ == (base*)&__f.__buf_)
      {
        __f_ = (base*)&__buf_;
        __f.__f_->clone(__f_);
      }
    else
      {
        __f_ = __f.__f_;
        __f.__f_ = 0;
      }
  }

  template<class Rp, class ...ArgTypes>
  template <class _Alloc>
  function<Rp(ArgTypes...)>::function(std::allocator_arg_t, const _Alloc&,
                                      function&& __f)
  {
    if (__f.__f_ == 0)
      __f_ = 0;
    else if (__f.__f_ == (base*)&__f.__buf_)
      {
        __f_ = (base*)&__buf_;
        __f.__f_->clone(__f_);
      }
    else
      {
        __f_ = __f.__f_;
        __f.__f_ = 0;
      }
  }

  template<class Rp, class ...ArgTypes>
  template <class Fp>
  function<Rp(ArgTypes...)>::function(Fp __f,
                                      typename std::enable_if
                                      <
                                      __callable<Fp>::value &&
                                      not std::is_same<Fp, function>::value
                                      >::type*)
    : __f_(0)
  {
    if (details::function::not_null(__f))
      {
        typedef details::function::func<Fp, std::allocator<Fp>, Rp(ArgTypes...)> _FF;
        if (sizeof(_FF) <= sizeof(__buf_) and std::is_nothrow_copy_constructible<Fp>::value)
          {
            __f_ = (base*)&__buf_;
            ::new (__f_) _FF(std::move(__f));
          }
        else
          {
            typedef std::allocator<_FF> _Ap;
            _Ap __a;
            typedef details::alloc::allocator_destructor<_Ap> _Dp;
            std::unique_ptr<base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1)); // todo : add std::unique_ptr
            ::new (__hold.get()) _FF(std::move(__f), std::allocator<Fp>(__a));
            __f_ = __hold.release();
          }
      }
  }

  template<class Rp, class ...ArgTypes>
  template <class Fp, class _Alloc>
  function<Rp(ArgTypes...)>::function(std::allocator_arg_t, const _Alloc& __a0, Fp __f,
                                      typename std::enable_if<__callable<Fp>::value>::type*)
    : __f_(0)
  {
    typedef std::allocator_traits<_Alloc> __alloc_traits;
    if (details::function::not_null(__f))
      {
        typedef details::function::func<Fp, _Alloc, Rp(ArgTypes...)> _FF;
        typedef typename details::alloc::rebind_helper<__alloc_traits, _FF>::type _Ap;
        _Ap __a(__a0);
        if (sizeof(_FF) <= sizeof(__buf_) && 
            std::is_nothrow_copy_constructible<Fp>::value and std::is_nothrow_copy_constructible<_Ap>::value)
          {
            __f_ = (base*)&__buf_;
            ::new (__f_) _FF(std::move(__f), _Alloc(__a));
          }
        else
          {
            typedef details::alloc::allocator_destructor<_Ap> _Dp;
            std::unique_ptr<base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
            ::new (__hold.get()) _FF(std::move(__f), _Alloc(__a));
            __f_ = __hold.release();
          }
      }
  }

  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>&
  function<Rp(ArgTypes...)>::operator=(const function& __f)
  {
    function(__f).swap(*this);
    return *this;
  }

  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>&
  function<Rp(ArgTypes...)>::operator=(function&& __f) noexcept
  {
    if (__f_ == (base*)&__buf_)
      __f_->destroy();
    else if (__f_)
      __f_->destroy_deallocate();
    __f_ = 0;
    if (__f.__f_ == 0)
      __f_ = 0;
    else if (__f.__f_ == (base*)&__f.__buf_)
      {
        __f_ = (base*)&__buf_;
        __f.__f_->clone(__f_);
      }
    else
      {
        __f_ = __f.__f_;
        __f.__f_ = 0;
      }
    return *this;
  }

  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>&
  function<Rp(ArgTypes...)>::operator=(std::nullptr_t) noexcept
  {
    if (__f_ == (base*)&__buf_)
      __f_->destroy();
    else if (__f_)
      __f_->destroy_deallocate();
    __f_ = 0;
    return *this;
  }

  template<class Rp, class ...ArgTypes>
  template <class Fp>
  typename std::enable_if
  <
    function<Rp(ArgTypes...)>::template __callable<typename std::decay<Fp>::type>::value and
  not std::is_same<typename std::remove_reference<Fp>::type, function<Rp(ArgTypes...)>>::value,
    function<Rp(ArgTypes...)>&
    >::type
    function<Rp(ArgTypes...)>::operator=(Fp&& __f)
  {
    function(std::forward<Fp>(__f)).swap(*this);
    return *this;
  }

  template<class Rp, class ...ArgTypes>
  function<Rp(ArgTypes...)>::~function()
  {
    if (__f_ == (base*)&__buf_)
      __f_->destroy();
    else if (__f_)
      __f_->destroy_deallocate();
  }

  template<class Rp, class ...ArgTypes>
  void
  function<Rp(ArgTypes...)>::swap(function& __f) noexcept
  {
    if (__f_ == (base*)&__buf_ && __f.__f_ == (base*)&__f.__buf_)
      {
        typename std::aligned_storage<sizeof(__buf_)>::type __tempbuf;
        base* __t = (base*)&__tempbuf;
        __f_->clone(__t);
        __f_->destroy();
        __f_ = 0;
        __f.__f_->clone((base*)&__buf_);
        __f.__f_->destroy();
        __f.__f_ = 0;
        __f_ = (base*)&__buf_;
        __t->clone((base*)&__f.__buf_);
        __t->destroy();
        __f.__f_ = (base*)&__f.__buf_;
      }
    else if (__f_ == (base*)&__buf_)
      {
        __f_->clone((base*)&__f.__buf_);
        __f_->destroy();
        __f_ = __f.__f_;
        __f.__f_ = (base*)&__f.__buf_;
      }
    else if (__f.__f_ == (base*)&__f.__buf_)
      {
        __f.__f_->clone((base*)&__buf_);
        __f.__f_->destroy();
        __f.__f_ = __f_;
        __f_ = (base*)&__buf_;
      }
    else
      std::swap(__f_, __f.__f_);
  }

  template<class Rp, class ...ArgTypes>
  Rp
  function<Rp(ArgTypes...)>::operator()(ArgTypes... __arg) const
  {
#if 0 // do or don't check ?
    if (__f_ == 0)
      throw bad_function_call();
#endif
    return (*__f_)(std::forward<ArgTypes>(__arg)...);
  }

  template <class Rp, class... ArgTypes>
  inline
  bool
  operator==(const function<Rp(ArgTypes...)>& __f, std::nullptr_t) noexcept { return !__f; }

  template <class Rp, class... ArgTypes>
  inline
  bool
  operator==(std::nullptr_t, const function<Rp(ArgTypes...)>& __f) noexcept { return !__f; }

  template <class Rp, class... ArgTypes>
  inline
  bool
  operator!=(const function<Rp(ArgTypes...)>& __f, std::nullptr_t) noexcept { return (bool)__f; }

  template <class Rp, class... ArgTypes>
  inline
  bool
  operator!=(std::nullptr_t, const function<Rp(ArgTypes...)>& __f) noexcept { return (bool)__f; }

  template <class Rp, class... ArgTypes>
  inline
  void
  swap(function<Rp(ArgTypes...)>& __x, function<Rp(ArgTypes...)>& __y) noexcept { return __x.swap(__y); }

} // namespace mstd

