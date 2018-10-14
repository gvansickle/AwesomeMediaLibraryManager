
//  Copyright 2015-2018 Denis Blank <denis.blank at outlook dot com>
//     Distributed under the Boost Software License, Version 1.0
//       (See accompanying file LICENSE_1_0.txt or copy at
//             http://www.boost.org/LICENSE_1_0.txt)

#include <memory>
#include <string>
#include <vector>

#include "function2-test.hpp"

struct stateful_callable {
  std::string test;

  void operator()() {
  }
};

/// Iterator dereference (nullptr) crash in Visual Studio
///
/// This was caused through an issue with the allocated pointer swap on move
TEST(regression_tests, move_iterator_dereference_nullptr) {
  std::string test = "hey";
  fu2::function<void()> fn = stateful_callable{std::move(test)};

  auto fn2 = std::move(fn);
  (void)fn2;
}

int function_issue_7_regression(int& i) {
  return i;
}

/// The following code does not compile on
/// MSVC version 19.12.25830.2 (Visual Studio 2017 15.5.1):
///
/// https://github.com/Naios/function2/issues/7
TEST(regression_tests, reference_parameters_issue_7) {
  fu2::function<int(int&)> f = function_issue_7_regression;
  int i = 4384674;
  ASSERT_EQ(f(i), 4384674);
}

struct scalar_member {
  explicit scalar_member(int num) : num_(num) {
  }
  int num_;
};

/// https://github.com/Naios/function2/issues/10
TEST(regression_tests, scalar_members_issue_10) {
  scalar_member const obj(4384674);

  fu2::function<int(scalar_member const&)> fn = &scalar_member::num_;
  ASSERT_EQ(fn(obj), 4384674);
}

TEST(regression_tests, size_match_layout) {
  fu2::function<void() const> fn;

  ASSERT_EQ(sizeof(fn), fu2::detail::object_size::value);
}

struct trash_obj {
  int raw[3];

  int operator()() {
    return 12345;
  }
};

template <typename T>
struct no_allocate_allocator {
  using value_type = T;
  using size_type = size_t;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  no_allocate_allocator() = default;
  template <typename O>
  no_allocate_allocator(no_allocate_allocator<O>) {
  }

  template <typename M>
  struct rebind {
    typedef no_allocate_allocator<M> other;
  };

  pointer allocate(size_type, void const* = nullptr) {
    EXPECT_TRUE(false);
    return nullptr;
  }

  void deallocate(pointer, size_type) {
    FAIL();
  }
};

TEST(regression_tests, can_take_capacity_obj) {
  fu2::function_base<true, true, sizeof(trash_obj), false, true, int()> fn;

  fn.assign(trash_obj{}, no_allocate_allocator<trash_obj>{});

  ASSERT_EQ(fn(), 12345);
}

static int call(fu2::function_view<int()> fun) {
  return fun();
}

// https://github.com/Naios/function2/issues/13
TEST(regression_tests, can_convert_nonowning_noncopyable_view) {
  fu2::unique_function<int()> fun = []() mutable { return 12345; };
  int result = call(fun);
  ASSERT_EQ(result, 12345);
}
TEST(regression_tests, can_assign_nonowning_noncopyable_view) {
  fu2::unique_function<int()> fun = []() mutable { return 12345; };
  fu2::function_view<int()> fv;
  fv = fun;
  int result = fv();
  ASSERT_EQ(result, 12345);
}

static fu2::unique_function<void()> issue_14_create() {
  // remove the commented dummy capture to be compilable
  fu2::unique_function<void()>
      func = [i = std::vector<std::vector<std::unique_ptr<int>>>{}
              // ,dummy = std::unique_ptr<int>()
  ](){
          // ...
      };

  return std::move(func);
}

// https://github.com/Naios/function2/issues/14
TEST(regression_tests, issue_14) {
  issue_14_create()();
}
