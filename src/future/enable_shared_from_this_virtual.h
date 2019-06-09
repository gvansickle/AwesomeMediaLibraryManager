/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AWESOMEMEDIALIBRARYMANAGER_ENABLE_SHARED_FROM_THIS_VIRTUAL_H
#define AWESOMEMEDIALIBRARYMANAGER_ENABLE_SHARED_FROM_THIS_VIRTUAL_H

/// @file enable_shared_from_this_virtual.h

#include <memory>

// From KDenLive, which got it from SO here:
// https://stackoverflow.com/questions/14939190/boost-shared-from-this-and-multiple-inheritance

// The following is a hack that allows to use shared_from_this in the case of a multiple inheritance.
// Credit: https://stackoverflow.com/questions/14939190/boost-shared-from-this-and-multiple-inheritance
template <typename T> struct enable_shared_from_this_virtual;

class enable_shared_from_this_virtual_base : public std::enable_shared_from_this<enable_shared_from_this_virtual_base>
{
    using base_type = std::enable_shared_from_this<enable_shared_from_this_virtual_base>;
    template <typename T> friend struct enable_shared_from_this_virtual;

    std::shared_ptr<enable_shared_from_this_virtual_base> shared_from_this() { return base_type::shared_from_this(); }
    std::shared_ptr<enable_shared_from_this_virtual_base const> shared_from_this() const { return base_type::shared_from_this(); }
};

template <typename T> struct enable_shared_from_this_virtual : virtual enable_shared_from_this_virtual_base
{
    using base_type = enable_shared_from_this_virtual_base;

public:
    std::shared_ptr<T> shared_from_this()
    {
        std::shared_ptr<T> result(base_type::shared_from_this(), static_cast<T *>(this));
        return result;
    }

    std::shared_ptr<T const> shared_from_this() const
    {
        std::shared_ptr<T const> result(base_type::shared_from_this(), static_cast<T const *>(this));
        return result;
    }
};

#endif //AWESOMEMEDIALIBRARYMANAGER_ENABLE_SHARED_FROM_THIS_VIRTUAL_H
