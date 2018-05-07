#include <cstddef>
#include <memory_resource.hpp>
#include <string.hpp>
#include <vector.hpp>

// Bar, the class we will make allocator aware.
struct Bar {
  int d_i = 0;
  std::vector<int> d_v{};
};

// Foo, the allocator aware version of Bar.
struct Foo {
  // See 23.10.7.2 for information on when something can use "allocator
  // construction". See also "uses_allocator"

  using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

  Foo(std::pmr::polymorphic_allocator<std::byte> alloc = {}) : d_v(alloc) {}

  Foo(int i, std::pmr::polymorphic_allocator<std::byte> alloc = {})
      : d_i(i), d_v(alloc) {}

  Foo(int i, const std::pmr::vector<int> &v,
      std::pmr::polymorphic_allocator<std::byte> alloc = {})
      : d_i(i), d_v(v, alloc) {}

  Foo(int i, std::pmr::vector<int> &&v,
      std::pmr::polymorphic_allocator<std::byte> alloc = {})
      : d_i(i), d_v(std::move(v), alloc) {}

  // Note that we cannot use the "pass by value and move" shorthand.
  Foo(const Foo &other, std::pmr::polymorphic_allocator<std::byte> alloc = {})
      : d_i(other.d_i), d_v(other.d_v, alloc) {}

  // This is subtle! We need two move constructors and we're using another
  // resource's allocator. Move constructors always move the allocator.
  // Assignment never does.
  Foo(const Foo &&other)
      : d_i(other.d_i), d_v(std::move(other.d_v), other.get_allocator()) {}

  Foo(const Foo &&other, std::pmr::polymorphic_allocator<std::byte> alloc)
      : d_i(other.d_i), d_v(std::move(other.d_v), alloc) {}

  Foo &operator=(const Foo &other) {
    d_i = other.d_i;
    d_v = other.d_v;
    return *this;
  }

  // Note that this is not (nor can it be) noexcept.
  Foo &operator=(Foo &&other) {
    d_i = other.d_i;
    d_v = std::move(other.d_v);
    return *this;
  }

  allocator_type get_allocator() const noexcept { return d_v.get_allocator(); }

  int d_i = 0;
  std::pmr::vector<int> d_v{};
};

int main() {
  Foo foo1{};
  Foo foo2{33};
  Foo foo3{33, {1, 2, 3}};

  Bar bar1{};
  Bar bar2{33};
  Bar bar3{33, {1, 2, 3}};
}
