/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file cloneable.h
 *
 * Support infrastructure for covariant std::unique<> and in particular a covariant clone() interface.
 * Adapted from @link https://www.fluentcpp.com/2017/09/12/how-to-return-a-smart-pointer-and-use-covariance/.
 * Well, not really adapted since I can't get this to work.  Trying a single template function clone() way instead.
 */

#ifndef AWESOMEMEDIALIBRARYMANAGER_CLONEABLE_H
#define AWESOMEMEDIALIBRARYMANAGER_CLONEABLE_H

#include <memory>

/**
 * Clone free function template.
 * Only requires T to have a publicly accessible copy constructor.
 */
template <class T, class BaseType>
BaseType* clone(T derived_object)
{

}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class abstract_method
{
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class virtual_inherit_from : virtual public T
{
	// Bring all the base classes' constructors into the current namespace.
	using T::T;
};

///////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename ... Bases>
class clone_inherit : public Bases...
{
public:
	 ~clone_inherit() override = default;

	std::unique_ptr<Derived> clone() const
	{
		return std::unique_ptr<Derived>(static_cast<Derived *>(this->clone_impl()));
	}

protected:
	//         desirable, but impossible in C++17
	//         see: http://cplusplus.github.io/EWG/ewg-active.html#102
	// using typename... Bases::Bases;

private:
	clone_inherit* clone_impl() const override
	{
		return new Derived(static_cast<const Derived & >(*this));
	}
};

///////////////////////////////////////////////////////////////////////////////

template <typename Derived, typename ... Bases>
class clone_inherit<abstract_method<Derived>, Bases...> : public Bases...
{
public:
	 ~clone_inherit() override = default;

	std::unique_ptr<Derived> clone() const
	{
		return std::unique_ptr<Derived>(static_cast<Derived *>(this->clone_impl()));
	}

protected:
	//         desirable, but impossible in C++17
	//         see: http://cplusplus.github.io/EWG/ewg-active.html#102
	// using typename... Bases::Bases;

private:
	 clone_inherit * clone_impl() const override = 0;
};

///////////////////////////////////////////////////////////////////////////////

template <typename Derived>
class clone_inherit<Derived>
{
public:
	virtual ~clone_inherit() = default;

	std::unique_ptr<Derived> clone() const
	{
		return std::unique_ptr<Derived>(static_cast<Derived *>(this->clone_impl()));
	}

private:
	clone_inherit * clone_impl() const override
	{
		return new Derived(static_cast<const Derived & >(*this));
	}
};

///////////////////////////////////////////////////////////////////////////////

template <typename Derived>
class clone_inherit<abstract_method<Derived>>
{
public:
	virtual ~clone_inherit() = default;

	std::unique_ptr<Derived> clone() const
	{
		return std::unique_ptr<Derived>(static_cast<Derived *>(this->clone_impl()));
	}

private:
	virtual clone_inherit * clone_impl() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// @name Example User Code

//class cloneable : public clone_inherit<abstract_method<cloneable>>
//{
//};

//class abstracted : public clone_inherit<abstract_method<abstracted>, cloneable>
//{
//};

class cloneable : public clone_inherit<abstract_method<cloneable>>
{
};

///////////////////////////////////////////////////////////////////////////////

class foo
   : public clone_inherit<abstract_method<foo>, virtual_inherit_from<cloneable>>
{
};

///////////////////////////////////////////////////////////////////////////////

class bar
   : public clone_inherit<abstract_method<bar>, virtual_inherit_from<cloneable>>
{
};

///////////////////////////////////////////////////////////////////////////////

class concrete
   : public clone_inherit<concrete, foo, bar>
{
};

#endif //AWESOMEMEDIALIBRARYMANAGER_CLONEABLE_H
