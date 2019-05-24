
/*
  Copyright(c) 2015 - 2019 Denis Blank <denis.blank at outlook dot com>

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
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <test-continuable.hpp>

using namespace cti;

auto add(promise<>& all) {
  return make_continuable<void>([&](auto&& promise) {
    auto res = split(std::move(all), std::forward<decltype(promise)>(promise));
    EXPECT_TRUE(res);
    all = std::move(res);
  });
}

TYPED_TEST(single_dimension_tests, operations_split) {
  promise<> all;
  bool resolved = false;

  when_all(add(all), add(all), add(all)).then([&resolved] {
    EXPECT_FALSE(resolved);
    resolved = true;
  });

  ASSERT_FALSE(resolved);
  all.set_value();
  ASSERT_TRUE(resolved);
}

TYPED_TEST(single_dimension_tests, operations_split_plain_callback) {
  promise<> all;
  bool resolved = false;

  all = split(std::move(all), [&](auto&&... args) {
    detail::util::unused(args...); // GCC build fix
    EXPECT_FALSE(resolved);
    resolved = true;
  });

  ASSERT_FALSE(resolved);
  all.set_value();
  ASSERT_TRUE(resolved);
}
