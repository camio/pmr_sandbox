#include <memory_resource.hpp>
#include <string.hpp>
#include <vector.hpp>

#include <cassert>
#include <cstddef> // std::byte
#include <iostream>
#include <string>

class LoggingResource : public std::pmr::memory_resource {
public:
  LoggingResource(std::pmr::memory_resource *underlyingResource)
      : d_underlyingResource(underlyingResource) {}

private:
  std::pmr::memory_resource *d_underlyingResource;

  void *do_allocate(size_t bytes, size_t align) override {
    std::cout << "Allocating " << bytes << " bytes" << std::endl;
    return d_underlyingResource->allocate(bytes, align);
  }
  void do_deallocate(void *p, size_t bytes, size_t align) override {
    return d_underlyingResource->deallocate(p, bytes, align);
  }
  bool do_is_equal(memory_resource const &other) const noexcept override {
    return d_underlyingResource->is_equal(other);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo: the way we do things now.                                           //
//////////////////////////////////////////////////////////////////////////////

class Bar {
  std::string data{"data"};
};

class Foo {
  std::unique_ptr<Bar> d_bar{std::make_unique<Bar>()};
};

//////////////////////////////////////////////////////////////////////////////
// Foo2: use a polymorphic allocator with 'std::unique_ptr'                 //
//////////////////////////////////////////////////////////////////////////////

class polymorphic_allocator_delete {
public:
  polymorphic_allocator_delete(
      std::pmr::polymorphic_allocator<std::byte> allocator)
      : d_allocator(std::move(allocator)) {}
  template <typename T> void operator()(T *tPtr) {
    std::pmr::polymorphic_allocator<T>(d_allocator).destroy(tPtr);
    std::pmr::polymorphic_allocator<T>(d_allocator).deallocate(tPtr, 1);
  }

private:
  std::pmr::polymorphic_allocator<std::byte> d_allocator;
};

class Bar2 {
  std::string data{"data"};
};

class Foo2 {
  // Note: Core dump if this is a plain 'std::unique_ptr'
  // Note: Could use a std::pmr::make_unique.
  std::unique_ptr<Bar2, polymorphic_allocator_delete> d_bar;

public:
  Foo2() : d_bar(nullptr, {{std::pmr::get_default_resource()}}) {
    // Note: Would be nice to have a 'std::pmr::make_unique_ptr'
    std::pmr::polymorphic_allocator<Bar2> alloc{
        std::pmr::get_default_resource()};
    Bar2 *const bar = alloc.allocate(1);
    alloc.construct(bar);
    d_bar.reset(bar);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo3: make the string in Bar use std::pmr                               //
//////////////////////////////////////////////////////////////////////////////

class Bar3 {
  std::pmr::string data{"data"};
};

class Foo3 {
  std::unique_ptr<Bar3, polymorphic_allocator_delete> d_bar;

public:
  Foo3() : d_bar(nullptr, {{std::pmr::get_default_resource()}}) {
    std::pmr::polymorphic_allocator<Bar3> alloc{
        std::pmr::get_default_resource()};
    Bar3 *const bar = alloc.allocate(1);
    alloc.construct(bar);
    d_bar.reset(bar);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo4: recapture the space lost by holding the allocator in unique_ptr    //
//////////////////////////////////////////////////////////////////////////////

class default_polymorphic_allocator_delete {
public:
  template <typename T> void operator()(T *tPtr) {
    // !!! This is dangerous when the default resource changes!
    std::pmr::polymorphic_allocator<T>(std::pmr::get_default_resource())
        .destroy(tPtr);
    std::pmr::polymorphic_allocator<T>(std::pmr::get_default_resource())
        .deallocate(tPtr, 1);
  }
};

struct Bar4 {
public:
  std::pmr::string data{"data"};
};

class Foo4 {
public:
  std::unique_ptr<Bar4, default_polymorphic_allocator_delete> d_bar;

  Foo4() {
    std::pmr::polymorphic_allocator<Bar4> alloc{
        std::pmr::get_default_resource()};
    Bar4 *const bar = alloc.allocate(1);
    alloc.construct(bar);
    d_bar.reset(bar);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo5: demonstrate that the std::pmr::string is actually allocating       //
//////////////////////////////////////////////////////////////////////////////

struct Bar5 {
public:
  std::pmr::string data{"Lorem ipsum dolor sit amet, consectetur adipiscing "
                        "elit, sed do eiusmod tempor incididunt ut labore "
                        "et"};
};
class Foo5 {
public:
  std::unique_ptr<Bar5, default_polymorphic_allocator_delete> d_bar;

  Foo5() {
    std::pmr::polymorphic_allocator<Bar5> alloc{
        std::pmr::get_default_resource()};
    Bar5 *const bar = alloc.allocate(1);
    alloc.construct(bar);
    d_bar.reset(bar);
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo6: show what we need to get basic allocator awareness                 //
//////////////////////////////////////////////////////////////////////////////

class Bar6 {
public:
  Bar6(std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : data("data", allocator) {}

private:
  std::pmr::string data;
};

class Foo6 {
  std::pmr::polymorphic_allocator<std::byte> d_allocator;
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // New. We're now storing the allocator locally so we can use it later.
  std::unique_ptr<Bar6, polymorphic_allocator_delete> d_bar;

public:
  typedef std::pmr::polymorphic_allocator<std::byte> allocator_type;
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //New. Required for 'std::vector' to realize this is allocator aware.

  std::pmr::polymorphic_allocator<std::byte> get_allocator()
  // Return the allocator this object was constructed with.
  {
    return d_allocator;
  }

  Foo6(std::pmr::polymorphic_allocator<std::byte> allocator =
           std::pmr::get_default_resource())
      : d_allocator(allocator), d_bar(nullptr, {allocator}) {
    std::pmr::polymorphic_allocator<Bar6> barAlloc{allocator};
    Bar6 *const bar = barAlloc.allocate(1);
    try {
      barAlloc.construct(bar, allocator);
      //                      ^^^^^^^^^
      //                      New
    } catch (...) {
      barAlloc.deallocate(bar, 1);
      throw;
    }
    d_bar.reset(bar);
  }

  Foo6(const Foo6 &other,
       std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : Foo6(allocator) {
    *d_bar = *other.d_bar;
  }

  Foo6 &operator=(const Foo6 &other) {
    *d_bar = *other.d_bar;
    return *this;
  }

  Foo6(Foo6 &&other, std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : d_allocator(allocator), d_bar(nullptr, {d_allocator}) {
    if (get_allocator() == other.get_allocator())
      d_bar.reset(other.d_bar.release());
    else {
        std::pmr::polymorphic_allocator<Bar6> barAlloc{allocator};
        Bar6 *const bar = barAlloc.allocate(1);
        try {
          barAlloc.construct(bar, allocator);
        } catch (...) {
          barAlloc.deallocate(bar, 1);
          throw;
        }
        d_bar.reset(bar);
      operator=(other);
    }
  };
};

//////////////////////////////////////////////////////////////////////////////
// Foo7: add the nothrow move constructor                                   //
//////////////////////////////////////////////////////////////////////////////

class Bar7 {
public:
  Bar7(std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : data("data", allocator) {}

private:
  std::pmr::string data;
};

class Foo7 {
public:
  std::pmr::polymorphic_allocator<std::byte> d_allocator;
  std::unique_ptr<Bar7, polymorphic_allocator_delete> d_bar;

public:
  typedef std::pmr::polymorphic_allocator<std::byte> allocator_type;

  Foo7(std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : d_allocator(allocator), d_bar(nullptr, {allocator}) {
    std::pmr::polymorphic_allocator<Bar7> barAlloc{allocator};
    Bar7 *const bar = barAlloc.allocate(1);
    try {
      barAlloc.construct(bar, allocator);
      //                      ^^^^^^^^^
      //                      New
    } catch (...) {
      barAlloc.deallocate(bar, 1);
      throw;
    }
    d_bar.reset(bar);
  }

  Foo7(const Foo7 &other,
       std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : Foo7(allocator) {
    *d_bar = *other.d_bar;
  };

  Foo7(Foo7 &&other) noexcept
      : d_allocator(other.d_allocator), d_bar(nullptr, {d_allocator}) {
    d_bar.reset(other.d_bar.release());
  }

  Foo7 &operator=(const Foo7 &other) {
    *d_bar = *other.d_bar;
    return *this;
  }

  Foo7(Foo7 &&other, std::pmr::polymorphic_allocator<std::byte> allocator)
      //                     New. Removed default argument        ^
      : d_allocator(allocator), d_bar(nullptr, {d_allocator}) {
    if (get_allocator() == other.get_allocator())
      d_bar.reset(other.d_bar.release());
    else {
      *this = std::move(Foo7(allocator));
      operator=(other);
    }
  };

  std::pmr::polymorphic_allocator<std::byte> get_allocator()
  // Return the allocator this object was constructed with.
  {
    return d_allocator;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo8: show how to remove a unneeded pointer from Foo7                    //
//////////////////////////////////////////////////////////////////////////////

class Bar8 {
public:
  Bar8(std::pmr::polymorphic_allocator<std::byte> allocator =
           std::pmr::get_default_resource())
      : data("data", allocator) {}

private:
  std::pmr::string data;
};

class Foo8 {
  Bar8 *d_bar;
  //^^^^
  //New, we're using a pointer instead of a unique_ptr.
  std::pmr::polymorphic_allocator<std::byte> d_allocator;

public:
  typedef std::pmr::polymorphic_allocator<std::byte> allocator_type;

  Foo8(std::pmr::polymorphic_allocator<std::byte> allocator = {}) {
    std::pmr::polymorphic_allocator<Bar8> barAlloc{allocator};
    d_bar = barAlloc.allocate(1);
    try {
      barAlloc.construct(d_bar, allocator);
      //                 ^^^^^
      //New, we're allocating and constructing the pointer directly
    } catch (...) {
      barAlloc.deallocate(d_bar, 1);
      throw;
    }
  }

  Foo8(const Foo8 &other,
       std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : Foo8(allocator) {
    *d_bar = *other.d_bar;
  };

  Foo8(Foo8 &&other) noexcept : d_allocator(other.d_allocator), d_bar(nullptr) {
    std::swap(d_bar, other.d_bar);
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^
    // New, this is implemented differently
  }

  Foo8(Foo8 &&other, std::pmr::polymorphic_allocator<std::byte> allocator)
      : d_allocator(allocator), d_bar(nullptr) {
    if (get_allocator() == other.get_allocator())
      std::swap(d_bar, other.d_bar);
    else {
      *d_bar = *other.d_bar;
    }
  };

  std::pmr::polymorphic_allocator<std::byte> get_allocator() {
    return d_allocator;
  }

  ~Foo8()
  //^^^^^
  //New, we have a custom destructor
  {
    if (d_bar) {
      std::pmr::polymorphic_allocator<Bar8> barAlloc = get_allocator();
      barAlloc.destroy(d_bar);
      barAlloc.deallocate(d_bar, 1);
    }
  }
};

//////////////////////////////////////////////////////////////////////////////
// Foo9: remove another unneeded pointer from Foo6                          //
//////////////////////////////////////////////////////////////////////////////

class Bar9 {
public:
  Bar9(std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : data("data", allocator) {}

  std::pmr::polymorphic_allocator<std::byte> get_allocator()
  //                                         ^^^^^^^^^^^^^^^
  // New.
  {
    return data.get_allocator();
  }

private:
  std::pmr::string data;
};

class Foo9 {
  Bar9 *d_bar;
  // New: Foo9 no longer holds an allocator directly.

public:
  typedef std::pmr::polymorphic_allocator<std::byte> allocator_type;

  Foo9(std::pmr::polymorphic_allocator<std::byte> allocator = {}) {
    std::pmr::polymorphic_allocator<Bar9> barAlloc{allocator};
    d_bar = barAlloc.allocate(1);
    try {
      barAlloc.construct(d_bar, allocator);
    } catch (...) {
      barAlloc.deallocate(d_bar, 1);
      throw;
    }
  }
  Foo9(const Foo9 &other,
       std::pmr::polymorphic_allocator<std::byte> allocator = {})
      : Foo9(allocator) {
    *d_bar = *other.d_bar;
  };

  Foo9(Foo9 &&other) noexcept : d_bar(nullptr) {
    std::swap(d_bar, other.d_bar);
  }

  Foo9(Foo9 &&other, std::pmr::polymorphic_allocator<std::byte> allocator)
      : d_bar(nullptr) {
    if (allocator == other.get_allocator())
      std::swap(d_bar, other.d_bar);
    else {
      std::pmr::polymorphic_allocator<Bar9> barAlloc{allocator};
      d_bar = barAlloc.allocate(1);
      try {
        barAlloc.construct(d_bar, allocator);
      } catch (...) {
        barAlloc.deallocate(d_bar, 1);
        throw;
      }
      *d_bar = *other.d_bar;
    }
  };

  std::pmr::polymorphic_allocator<std::byte> get_allocator()
  // Return the allocator this object was constructed with.
  {
    return d_bar->get_allocator();
  }

  ~Foo9() {
    if (d_bar) {
      std::pmr::polymorphic_allocator<Bar9> barAlloc = get_allocator();
      barAlloc.destroy(d_bar);
      barAlloc.deallocate(d_bar, 1);
    }
  }
};

int main() {
  static LoggingResource memoryResource{std::pmr::new_delete_resource()};

  std::pmr::set_default_resource(&memoryResource);

  std::cout << "## sizeof(std::vector<int>) = " << sizeof(std::vector<int>)
            << std::endl;
  std::cout << "## sizeof(std::pmr::vector<int>) = "
            << sizeof(std::pmr::vector<int>) << std::endl;
  std::cout << "## sizeof(std::string) = " << sizeof(std::string) << std::endl;
  std::cout << "## sizeof(std::pmr::string) = " << sizeof(std::pmr::string)
            << std::endl;
  std::cout << std::endl;

  std::cout << "## vector<int> test" << std::endl;
  std::pmr::vector<int> ints;
  ints.push_back(33);
  ints.push_back(34);

  // Note that with Foo, I'm not capturing allocations of the inner thing.
  std::cout << "\n## vector<Foo> test" << std::endl;
  std::pmr::vector<Foo> foos;
  foos.emplace_back();
  foos.emplace_back();

  // Note that
  // - 'Foo2' is now 16 bytes instead of 8 because it is keeping track of the
  //   allocator for destruction
  // - The 'std::string' inside 'Bar' is not using the allocator.
  std::cout << "\n## vector<Foo2> test" << std::endl;
  std::pmr::vector<Foo2> foo2s;
  foo2s.emplace_back();
  foo2s.emplace_back();

  // Note that
  // - 'Bar2' is now 32 bytes instead of 24 because it is keeping track of
  //   the allocator for destruction
  std::cout << "\n## vector<Foo3> test" << std::endl;
  std::pmr::vector<Foo3> foo3s;
  foo3s.emplace_back();
  foo3s.emplace_back();

  // Note that
  // - We recaptured the memory taken by using unique_ptr
  std::cout << "\n## vector<Foo4> test" << std::endl;
  std::pmr::vector<Foo4> foo4s;
  foo4s.emplace_back();
  foo4s.emplace_back();

  // Note that
  // - The allocator is being used all the way down now.
  std::cout << "\n## vector<Foo5> test" << std::endl;
  std::pmr::vector<Foo5> foo5s;
  foo5s.emplace_back();
  foo5s.emplace_back();

  std::pmr::set_default_resource(nullptr);

  // Note that
  // - We went from 8 bytes to 24 bytes per Foo6. Allocator is now stored in
  //   the deleter and the allocator member (it is also stored in the Bar!).
  // - We're starting to see what "allocator aware" looks like
  // - Weird extra output
  std::cout << "\n## vector<Foo6> test" << std::endl;
  std::pmr::vector<Foo6> foo6s(
      std::pmr::polymorphic_allocator<Foo6>{&memoryResource});
  foo6s.emplace_back();
  foo6s.emplace_back();

  std::cout << "\n## vector<Foo7> test" << std::endl;
  std::pmr::vector<Foo7> foo7s(
      std::pmr::polymorphic_allocator<Foo7>{&memoryResource});
  foo7s.emplace_back();
  foo7s.emplace_back();

  // Note that
  // - We've gone from 24 bytes back to 16 bytes again
  // - We've lost some safety to gain space performance
  std::cout << "\n## vector<Foo8> test" << std::endl;
  std::pmr::vector<Foo8> foo8s(
      std::pmr::polymorphic_allocator<Foo8>{&memoryResource});
  foo8s.emplace_back();
  foo8s.emplace_back();

  // Note that
  // - We've gone from 16 bytes back to 8 bytes again
  // - We're delegating storage of the allocator to data members
  std::cout << "\n## vector<Foo9> test" << std::endl;
  std::pmr::vector<Foo9> foo9s(
      std::pmr::polymorphic_allocator<Foo9>{&memoryResource});
  foo9s.emplace_back();
  foo9s.emplace_back();

  std::cout << "\n## Tuple test" << std::endl;
  std::tuple<std::pmr::vector<int>, std::pmr::string> t{
      std::allocator_arg, &memoryResource, {1}, ""};

  // Does std::vector recognize std::tuple as allocator aware? Yes.
  std::cout << "\n## Vector<tuple> test" << std::endl;
  std::pmr::vector<std::tuple<std::pmr::vector<char>, std::pmr::string>> vt(
      &memoryResource);
  vt.resize(1);
  std::get<0>(vt[0]).resize(100);
}
