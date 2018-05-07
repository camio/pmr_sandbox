#include <memory_resource.hpp>
#include <string.hpp>
#include <vector.hpp>

#include <cassert>
#include <cstddef>  // std::byte
#include <iostream>
#include <string>

class LoggingResource : public std::pmr::memory_resource {
  public:
    LoggingResource(std::pmr::memory_resource *underlyingResource)
: d_underlyingResource(underlyingResource)
    {
    }

  private:
    std::pmr::memory_resource *d_underlyingResource;

    void *do_allocate(size_t bytes, size_t align) override
    {
        std::cout << "Allocating " << bytes << " bytes" << std::endl;
        return d_underlyingResource->allocate(bytes, align);
    }
    void do_deallocate(void *p, size_t bytes, size_t align) override
    {
        return d_underlyingResource->deallocate(p, bytes, align);
    }
    bool do_is_equal(memory_resource const& other) const noexcept override
    {
        return d_underlyingResource->is_equal(other);
    }
};

class polymorphic_allocator_delete {
  public:
    polymorphic_allocator_delete(
                          std::pmr::polymorphic_allocator<std::byte> allocator)
: d_allocator(std::move(allocator))
    {
    }
    template <typename T>
    void operator()(T *tPtr)
    {
        std::pmr::polymorphic_allocator<T>(d_allocator).destroy(tPtr);
        std::pmr::polymorphic_allocator<T>(d_allocator).deallocate(tPtr, 1);
    }

  private:
    std::pmr::polymorphic_allocator<std::byte> d_allocator;
};

class Bar {
  public:
    Bar(std::pmr::polymorphic_allocator<std::byte> allocator =
             std::pmr::get_default_resource())
    : data("data", allocator)
    {
    }

  private:
    std::pmr::string data;
};

class Foo {
  public:
    std::pmr::polymorphic_allocator<std::byte>          d_allocator;

  public:
    bool d_moved_from = false;

    typedef std::pmr::polymorphic_allocator<std::byte> allocator_type;

    Foo(std::pmr::polymorphic_allocator<std::byte> allocator =
             std::pmr::get_default_resource())
    : d_allocator(allocator)
    {
    }

    Foo(const Foo&                                other,
         std::pmr::polymorphic_allocator<std::byte> allocator =
             std::pmr::get_default_resource())
    : Foo(allocator)
    {
    };

    Foo(Foo&& other) noexcept : d_allocator(other.d_allocator)
    {
        other.d_moved_from = true;
        static int i = 0;
        std::cout << "move0 index=" << i++ << std::endl;
    }

    Foo& operator=(const Foo& other)
    {
        return *this;
    }

    Foo(Foo&& other, std::pmr::polymorphic_allocator<std::byte> allocator)
        : d_allocator(allocator)
    {
        static int i = 0;
        std::cout << "move1 index=" << i++ << std::endl;

        // Throw an exception on this call
        if (i == 3)
            throw std::bad_alloc();

        if (get_allocator() == other.get_allocator()) {
            other.d_moved_from = true;
        }
        else {
            *this    = std::move(Foo(allocator));
            operator =(other);
        }
    };

    std::pmr::polymorphic_allocator<std::byte> get_allocator()
    {
        return d_allocator;
    }
};

int main()
{
    std::cout << "\n## vector<Foo> test" << std::endl;
    LoggingResource        loggingResource(std::pmr::get_default_resource());
    std::pmr::vector<Foo> foos(
        std::pmr::polymorphic_allocator<Foo>{&loggingResource});
    foos.emplace_back();
    foos.emplace_back();
    try {
        // Essentially, the wrong move constructor is being called. The
        // noexcept one should be used in this case instead of the one that
        // also takes in an allocator.
        //
        // 23.2.1 paragraph 11.2
        // - if an exception is thrown by a push_back(), push_front(),
        //   emplace_back(), or emplace_front() function, that function has no effects.
        foos.emplace_back();
    }
    catch (const std::bad_alloc&) {
        std::cout << "No 0s should be below" << std::endl;
        for (int i = 0; i < foos.size(); ++i) {
            std::cout << " " << foos[i].d_moved_from;
        }
        std::cout << std::endl;
    }

    std::cout << "\n## std::is_nothrow_move_constructible_v<Foo>="
              << std::is_nothrow_move_constructible_v<Foo> << std::endl;
}
