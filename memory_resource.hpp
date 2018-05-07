#ifndef MEMORY_RESOURCE_HPP_
#define MEMORY_RESOURCE_HPP_

// header <memory_resource>
#include <experimental/memory_resource>

#if __has_include(<bdlma_bufferedsequentialallocator.h>)
#include <bdlma_bufferedsequentialallocator.h>
#endif

namespace std::pmr {
#ifdef __cpp_lib_experimental_memory_resources
using std::experimental::fundamentals_v2::pmr::polymorphic_allocator;
using std::experimental::fundamentals_v2::pmr::memory_resource;
using std::experimental::fundamentals_v2::pmr::set_default_resource;
using std::experimental::fundamentals_v2::pmr::get_default_resource;
using std::experimental::fundamentals_v2::pmr::new_delete_resource;
#elif defined(_LIBCPP_EXPERIMENTAL_MEMORY_RESOURCE)
using std::experimental::fundamentals_v1::pmr::polymorphic_allocator;
using std::experimental::fundamentals_v1::pmr::memory_resource;
using std::experimental::fundamentals_v1::pmr::set_default_resource;
using std::experimental::fundamentals_v1::pmr::get_default_resource;
using std::experimental::fundamentals_v1::pmr::new_delete_resource;
#else
#error No known memory resource.
#endif

#ifdef INCLUDED_BDLMA_BUFFEREDSEQUENTIALALLOCATOR
class monotonic_buffer_resource : public std::pmr::memory_resource {
    BloombergLP::bdlma::BufferedSequentialAllocator  d_bsa;
    std::pmr::memory_resource                       *d_underlyingResource;

  public:
    monotonic_buffer_resource(void                      *buffer,
                              std::size_t                size,
                              std::pmr::memory_resource *underlyingResource)
    : d_bsa(static_cast<char *>(buffer),
            size)  // todo, needs underlying resource
    , d_underlyingResource(underlyingResource)
    {
    }

  private:
    void *do_allocate(size_t bytes, size_t align) override
    {
        return d_bsa.allocate(bytes);
    }
    void do_deallocate(void *p, size_t bytes, size_t align) override
    {
        return d_bsa.deallocate(p);
    }
    bool do_is_equal(memory_resource const& other) const noexcept override
    {
        return d_underlyingResource->is_equal(other);
    }
};
#endif
}

#endif
