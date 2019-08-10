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

#define TYPE_2 1

// The following is a hack that allows to use shared_from_this in the case of a multiple inheritance.
// Credit: https://stackoverflow.com/questions/14939190/boost-shared-from-this-and-multiple-inheritance
template <typename T>
struct enable_shared_from_this_virtual;

class enable_shared_from_this_virtual_base : public std::enable_shared_from_this<enable_shared_from_this_virtual_base>
{
	typedef std::enable_shared_from_this<enable_shared_from_this_virtual_base> base_type;
    template <typename T>
    friend struct enable_shared_from_this_virtual;

public:
	virtual ~enable_shared_from_this_virtual_base() {};

    std::shared_ptr<enable_shared_from_this_virtual_base> shared_from_this()
    {
    	return base_type::shared_from_this();
    }
    std::shared_ptr<enable_shared_from_this_virtual_base const> shared_from_this() const
    {
    	return base_type::shared_from_this();
    }

	std::weak_ptr<enable_shared_from_this_virtual_base> weak_from_this() noexcept
	{
		return base_type::weak_from_this();
	};
	std::weak_ptr<enable_shared_from_this_virtual_base const> weak_from_this() const noexcept
	{
		return base_type::weak_from_this();
	};

	void* m_void_ptr;
};

template <typename T>
struct enable_shared_from_this_virtual : virtual enable_shared_from_this_virtual_base
{
	typedef enable_shared_from_this_virtual_base base_type;

public:
    std::shared_ptr<T> shared_from_this()
    {
#ifdef TYPE_2
		return std::dynamic_pointer_cast<T>(enable_shared_from_this_virtual_base::shared_from_this());
#else
		std::shared_ptr<T> result(base_type::shared_from_this(), static_cast<T *>(this));
        return result;
#endif
    }

    std::shared_ptr<T const> shared_from_this() const
    {
#ifdef TYPE_2
		return std::dynamic_pointer_cast<T const>(enable_shared_from_this_virtual_base::shared_from_this());
#else
        std::shared_ptr<T const> result(base_type::shared_from_this(), static_cast<T const *>(this));
        return result;
#endif
	}

	std::weak_ptr<T> weak_from_this() noexcept
	{
		std::weak_ptr<T> result(base_type::weak_from_this(), static_cast<T*>(this));
		return result;
	};
	std::weak_ptr<T const> weak_from_this() const noexcept
	{
		std::weak_ptr<T const> result(base_type::weak_from_this(), static_cast<T const*>(this));
	};

	/**
	 * Utility method to easily downcast.
	 * Useful when a child doesn't inherit directly from enable_shared_from_this
	 * but wants to use the feature.
	 * From SO:
	 * @link https://stackoverflow.com/a/16083526
	 */
	template <class Down>
	std::shared_ptr<Down> downcasted_shared_from_this()
	{
		return std::dynamic_pointer_cast<Down>(enable_shared_from_this_virtual_base::shared_from_this());
	}
};

#endif //AWESOMEMEDIALIBRARYMANAGER_ENABLE_SHARED_FROM_THIS_VIRTUAL_H
