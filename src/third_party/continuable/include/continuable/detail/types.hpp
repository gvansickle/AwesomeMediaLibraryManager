
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v3.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef CONTINUABLE_DETAIL_TYPES_HPP_INCLUDED
#define CONTINUABLE_DETAIL_TYPES_HPP_INCLUDED

#include <utility>

#include <continuable/detail/features.hpp>

#ifndef CONTINUABLE_WITH_CUSTOM_ERROR_TYPE
#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS
#include <exception>
#else // CONTINUABLE_WITH_NO_EXCEPTIONS
#include <system_error>
#endif // CONTINUABLE_WITH_NO_EXCEPTIONS
#endif // CONTINUABLE_WITH_CUSTOM_ERROR_TYPE

namespace cti {
template <typename Data, typename Annotation>
class continuable_base;

namespace detail {
/// Contains types used globally across the library
namespace types {
#ifdef CONTINUABLE_WITH_CUSTOM_ERROR_TYPE
using error_type = CONTINUABLE_WITH_CUSTOM_ERROR_TYPE;
#else // CONTINUABLE_WITH_CUSTOM_ERROR_TYPE
#ifndef CONTINUABLE_WITH_NO_EXCEPTIONS
/// Represents the error type when exceptions are enabled
using error_type = std::exception_ptr;
#else  // CONTINUABLE_WITH_NO_EXCEPTIONS
/// Represents the error type when exceptions are disabled
using error_type = std::error_condition;
#endif // CONTINUABLE_WITH_NO_EXCEPTIONS
#endif // CONTINUABLE_WITH_CUSTOM_ERROR_TYPE

/// A tag which is used to execute the continuation inside the current thread
struct this_thread_executor_tag {};
/// A tag which is used to continue with an error
struct dispatch_error_tag {};

/// Marks a given callable object as transformation
template <typename T>
class transform : T {
public:
  explicit transform(T callable) : T(std::move(callable)) {
  }

  using T::operator();
};

/// Tag for constructing an empty promise_base .
struct promise_no_init_tag {};
} // namespace types
} // namespace detail
} // namespace cti

#endif // CONTINUABLE_DETAIL_TYPES_HPP_INCLUDED
