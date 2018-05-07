#ifndef VECTOR_HPP_
#define VECTOR_HPP_

// header <vector>
#include <experimental/vector>
namespace std::pmr
{
#ifdef __cpp_lib_experimental_memory_resources 
    using std::experimental::fundamentals_v2::pmr::vector;
#elif defined(_LIBCPP_EXPERIMENTAL_MEMORY_RESOURCE)
    using std::experimental::fundamentals_v1::pmr::vector;
#else
    #error No known vector
#endif
}

#endif
