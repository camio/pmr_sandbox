#ifndef STRING_HPP_
#define STRING_HPP_

// header <string>
#include <experimental/string>
namespace std::pmr
{
#ifdef _GLIBCXX_EXPERIMENTAL_STRING
//    using std::experimental::fundamentals_v2::pmr::string;
  template<typename _CharT, typename _Traits = char_traits<_CharT>>
   using basic_string =
     std::basic_string<_CharT, _Traits, polymorphic_allocator<_CharT>>;

  // basic_string typedef names using polymorphic allocator in namespace
  // std::experimental::pmr
  typedef basic_string<char> string;
  typedef basic_string<char16_t> u16string;
  typedef basic_string<char32_t> u32string;
  typedef basic_string<wchar_t> wstring;
#elif defined(_LIBCPP_EXPERIMENTAL_STRING)
    using std::experimental::fundamentals_v1::pmr::string;
#else
    #error No known vector
#endif
}

#endif
