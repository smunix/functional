#pragma once
#include <memory>
#include <type_traits>

namespace func {
  namespace alloc {
    template <class T>
    struct Custom {
      typedef T value_type;
      Custom() noexcept {}
      template <class U> Custom (const Custom<U>&) noexcept {
        logtv(this);
      }
      T* allocate (std::size_t n) {
        auto r = static_cast<T*>(::operator new(n*sizeof(value_type)));
        logtv(this);
        log(r << ", " << n << ", " << n*sizeof(value_type));
        return r;
      }
      void deallocate (T* p, std::size_t n) {
        logtv(this);
        log(p << ", " << n << ", " << n * sizeof(value_type));
        ::operator delete(p);
      }
      template<class V> struct rebind {
        using other = Custom<V>;
      };
    };

    template <class T, class U>
    constexpr bool operator== (const Custom<T>&, const Custom<U>&) noexcept
    {return true;}

    template <class T, class U>
    constexpr bool operator!= (const Custom<T>&, const Custom<U>&) noexcept
    {return false;}
  } // alloc
} // func

namespace func {

  template<class F> struct Size {
    static constexpr size_t apply() { return sizeof(F); }
  };

  template<size_t N> struct Allocator {
    template<class Ap> static Ap& apply() {
      __thread static Ap* g_ap;
      if (not g_ap)
        g_ap = new Ap;
      logtv(g_ap);
      log(g_ap << ", size=" << N);
      return *g_ap;
    }
  };

  template<class T> struct Make {
    template<class F> static auto apply() -> std::function<T> { return std::function<T>{}; }
    template<class F> static auto apply(F&& f) -> std::function<T> {
      return std::function<T>(std::allocator_arg, Allocator<Size<typename std::decay<F>::type>::apply()>::template apply<alloc::Custom<int>>(), std::forward<F>(f));
    }
  };
  template<class T, class F> auto make(F&& f) -> std::function<T> {
    return Make<T>::template apply(std::forward<F>(f));
  }
  template<class T> auto make() -> std::function<T> {
    return Make<T>::template apply();
  }
  template<class F> struct Assign;
  template<class T> struct Assign<std::function<T>> {
    template<class F, class NF> static auto apply(F&& f, NF&& nf) -> void {
      std::forward<F>(f).assign(std::forward<NF>(nf), Allocator<Size<typename std::decay<NF>::type>::apply()>::template apply<alloc::Custom<int>>());
    }
  };
  template<class F, class NF> auto assign(F&& f, NF&& nf) -> void {
    Assign<typename std::decay<F>::type>::template apply(std::forward<F>(f), std::forward<NF>(nf));
  }
} // func
