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
 * @file ExtAsyncExceptions.h
 */
#ifndef SRC_CONCURRENCY_EXTASYNCEXCEPTIONS_H_
#define SRC_CONCURRENCY_EXTASYNCEXCEPTIONS_H_

#include <config.h>

// Std C++
#include <exception>
#include <iostream>

// Qt
#include <QException>

/**
 * @link https://www.boost.org/doc/libs/1_68_0/libs/exception/doc/using_virtual_inheritance_in_exception_types.html
 * "Exception types should use virtual inheritance when deriving from other exception types. [...]
 * Using virtual inheritance prevents ambiguity problems in the exception handler"
 * Except QException just public derives from std::exception, and std::exception
 */
class ExtAsyncCancelException : public QException
{
public:
//	ExtAsyncCancelException() : {  }
    void raise() const override { throw *this; }
    ExtAsyncCancelException *clone() const override { return new ExtAsyncCancelException(*this); }
};

enum class ExtFutureErrc
{
    broken_promise,
    future_already_retrieved,
	promise_already_satisfied,
    no_state
};

/**
 * Qt5ish std::future_error.
 * @see @link https://en.cppreference.com/w/cpp/thread/future_error
 */
class ExtFutureError : public QException
{
public:
	explicit ExtFutureError(ExtFutureErrc errc, const char* msg = nullptr) : m_code(errc) {};
	ExtFutureErrc code() const { return m_code; };
    void raise() const override { throw *this; }
	ExtFutureError *clone() const override { return new ExtFutureError(*this); }
private:
    ExtFutureErrc m_code {};
};

inline void print_exception(const std::exception& e, int level =  0)
{
	std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
	try
	{
		std::rethrow_if_nested(e);
	}
	catch(const std::exception& e)
	{
		print_exception(e, level+1);
	}
	catch(...)
	{}
}


#endif /* SRC_CONCURRENCY_EXTASYNCEXCEPTIONS_H_ */
