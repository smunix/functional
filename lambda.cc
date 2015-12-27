#include <iostream>
#include <cxxabi.h>
#include <array>
#include <memory>

namespace cpp {
  template<class T> std::string demangle() {
    int status = -4;
    std::unique_ptr<char, void(*)(void*)> p {
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
        std::free
        };
    return not status ? p.get() : typeid(T).name(); 
  }
} // namespace cpp

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <sstream>
#include <functional>
#include <functional.H>

struct Logger {
  static void apply(std::ostringstream& os) {
    static std::mutex m;
    std::unique_lock<std::mutex> l(m);
    std::cout << "tid=" << std::this_thread::get_id() << ":" << os.str() << std::endl;
  }
};

#define log(x) do { std::ostringstream os; os << "line=<" << __LINE__ << ">" << ", func=" << __PRETTY_FUNCTION__ << ", "<< x; Logger::apply(os); } while(0)
#define logtv(x) log(cpp::demangle<decltype(x)>())
#define logt(x) log(cpp::demangle<x>())
namespace p = std::placeholders;

#include <functional.hh>

namespace test {
  template<class _Fp> using function = smunix::function<_Fp, 8>; // Sz = 8 * sizeof(void*) <- 8 pointers == 64 bytes
} // test

struct ThreadTraits {
  using Element = test::function<void()>;
  using Queue = std::deque<Element>;
};

template<class TT = ThreadTraits, bool TWait = true> struct Thread {
  using Queue = typename TT::Queue;
  using Element = typename TT::Element;
  template<class A> using up = std::unique_ptr<A>;
  template<class A> using sp = std::shared_ptr<A>;

  explicit Thread(bool a_started = true) : running(true) {
    if (a_started)
      dispatcher.reset(new std::thread([this](){ apply(); }));
  }
  virtual ~Thread() {
    stop();
  }
  void start() {
    if (not dispatcher) {
      running = true;
      dispatcher.reset(new std::thread([this](){ apply(); }));
    }
  }
  void stop() {
    try {
      if (running) {
        running = false;
        cv.notify_one();
        if (dispatcher) {
          dispatcher->join();
          dispatcher.reset();
        }
      }
    } catch(...) {
    }
  }
  template<class F> void exec(F&& f) {
    push(Element(f));
  }
private:
  void push(Element&& e) {
    if (not running) return;
    std::unique_lock<std::mutex> l(m);
    queue.push_back(std::move(e));
    cv.notify_one();
  }
  bool process() {
    Element e;
    {
      std::unique_lock<std::mutex> l(m);
      if (queue.empty()) return false;
      std::swap(e, queue.front());
      queue.pop_front();
    }
    e();
    return true;
  }
  void apply() {
    while(running) {
      try {
        while (running) {
          {
            std::unique_lock<std::mutex> l(m);
            while(queue.empty() and running) {
              cv.wait(l);
            }
          }
          while ((running or TWait) and process());
        }
      } catch(...) {
      }
    }
  }
  bool transparent = false;
  bool running = false;
  up<std::thread> dispatcher;
  std::mutex m;
  std::condition_variable cv;
  Queue queue;
};
#define Assert() do { int *t = nullptr; /* *t = 5; */ } while(0)

struct Actor {
  template<class A> using up = std::unique_ptr<A>;
  template<class A> using sp = std::shared_ptr<A>;
  template<class A> using seq = std::vector<A>;
  using Element = test::function<void()>;

  template <class T>
  struct Custom {
    using value_type = T;
    Custom() noexcept {}
    template <class U> Custom (const Custom<U>& o) noexcept {
    }
    T* allocate (std::size_t n) {
      auto r = static_cast<T*>(::operator new(n*sizeof(value_type)));
      log(r << ", " << n << ", " << n*sizeof(value_type));
      Assert();
      return r;
    }
    void deallocate (T* p, std::size_t n) {
      log(p << ", " << n << ", " << n * sizeof(value_type));
      ::operator delete(p);
    }
    template<class V> struct rebind {
      using other = Custom<V>;
    };
  };
  Thread<>& thread;
  Custom<Element> alloc;

  Actor(Thread<>& thread) : thread(thread) {}
  template<class F> void dispatch(F&& f) {
#if 1
    log(this << ", sizeof(f)=" << sizeof(f) << ", (4*sizeof(void*))=" << 4*(sizeof(void*)));
    log(std::boolalpha << "std::is_nothrow_copy_constructible<" << cpp::demangle<F>() << ">::value=" << std::is_nothrow_copy_constructible<F>::value);
    log(std::boolalpha << "std::is_nothrow_copy_constructible<" << cpp::demangle<std::allocator_traits<Custom<Element>>>() << ">::value=" << std::is_nothrow_copy_constructible<std::allocator_traits<Custom<Element>>>::value);
    using _Fp = F;
    using _Alloc = Custom<Element>;
    typedef smunix::details::function::func<_Fp, _Alloc, void()> _FF;
    typedef std::allocator_traits<_Alloc> __alloc_traits;
    typedef typename smunix::details::alloc::rebind_helper<__alloc_traits, _FF>::type _Ap;
    test::function<void()> func (std::allocator_arg, alloc, std::forward<F>(f));
    log("sizeof(" << cpp::demangle<_FF>() << ")=" << sizeof(_FF) << ", sizeof(func.__buf_)=" << sizeof(func.__buf_) << ", typeof(func.__buf_)=" << cpp::demangle<decltype(func.__buf_)>());
#endif
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // static_assert(sizeof(F) < (3*sizeof(void*)), "lambda captures to be allocated on the heap"); //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    thread.exec(Element(std::allocator_arg, alloc, f));
  }
};

template<size_t N> struct Env {
  template<class A> using up = std::unique_ptr<A>;
  template<class A> using sp = std::shared_ptr<A>;
  std::array<sp<Actor>, N> actors;
  std::array<sp<Thread<>>, N> threads;
  void init() {
    initThreads();
    initActors();
  }
  Actor& operator[] (size_t n) { return *actors[n]; }
private:
  void initThreads() {
    for(auto& t: threads)
      t.reset(new Thread<>);
  }
  void initActors() {
    auto i = 0;
    do {
      threads[i]->exec([this, i](){
          actors[i].reset(new Actor(*threads[i]));
        });
    } while (++i < N);
  }
};

template<size_t N> struct Dispatch {
  template<class E> static void apply(E&& env) {
    std::array<char, N> ar;
    env[0].dispatch([ar]() mutable { log(N); ar[N-1] = 'A'; });
    Dispatch<N-1>::apply(std::forward<E>(env));
  }
};

template<> struct Dispatch<1> {
  template<class E> static void apply(E&& env) {
    constexpr auto N = 1;
    std::array<char, N> ar;
    env[0].dispatch([ar]() mutable { log(N); ar[N-1] = 'A';});
  }
};

int main() {
#if 1
  constexpr auto N = 1;
  std::array<char, (1<<10)> ar;
  Env<N> env;
  env.init();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Dispatch<25>::apply(env); // <- 25 allocates on the heap, [24..1] don't
  std::this_thread::sleep_for(std::chrono::seconds(1));
#else
  std::cout << "max stored locally size: " << sizeof(std::_Nocopy_types) << ", align: " << __alignof__(std::_Nocopy_types) << std::endl;
  auto lambda = [](){};

  typedef decltype(lambda) lambda_t;

  std::cout << "lambda size: " << sizeof(lambda_t) << std::endl;
  std::cout << "lambda align: " << __alignof__(lambda_t) << std::endl;

  std::cout << "stored locally: " << ((std::__is_location_invariant<lambda_t>::value
                                       && sizeof(lambda_t) <= std::_Function_base::_M_max_size
                                       && __alignof__(lambda_t) <= std::_Function_base::_M_max_align
                                       && (std::_Function_base::_M_max_align % __alignof__(lambda_t) == 0)) ? "true" : "false") << std::endl;
#endif
  return 0;
}
