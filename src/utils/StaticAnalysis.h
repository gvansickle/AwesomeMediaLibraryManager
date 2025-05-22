//
// Created by gary on 11/30/18.
//

#ifndef AWESOMEMEDIALIBRARYMANAGER_STATICANALYSIS_H
#define AWESOMEMEDIALIBRARYMANAGER_STATICANALYSIS_H

/// @file

#include <type_traits>

/**
 * GSL template for marking an owning pointer.
 * @see @link http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Ri-raw
 * @link https://releases.llvm.org/7.0.0/tools/clang/tools/extra/docs/clang-tidy/checks/cppcoreguidelines-owning-memory.html
 */
namespace gsl
{
template <class T, class = std::enable_if_t<std::is_pointer<T>::value>>
using owner = T;
}

#endif //AWESOMEMEDIALIBRARYMANAGER_STATICANALYSIS_H
