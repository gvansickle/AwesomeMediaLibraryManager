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
 * @file
 * An extended QFutureInterface<T> class.
 */

#ifndef UTILS_CONCURRENCY_EXTFUTURE_H_
#define UTILS_CONCURRENCY_EXTFUTURE_H_

#include <QFutureInterface>

template <typename T>
class ExtFuture : public QFutureInterface<T>
{
	using BASE_CLASS = QFutureInterface<T>;

public:
	ExtFuture(QFutureInterfaceBase::State initialState = QFutureInterfaceBase::Started) : QFutureInterface<T>(initialState)
	{
		// Nothing.
	}

	ExtFuture(const QFutureInterface<T> &other)
		: QFutureInterface<T>(other)
	{
	}

	/**
	 * Attaches a continuation to this ExtFuture.
	 * @return A new future for containing the return value of @a continuation_function.
	 */
	ExtFuture<void> then(std::function<ExtFuture<void>()> continuation_function)
	{
		m_continuation_function = std::move(continuation_function);
		return m_continuation_function();
	}

protected:

	std::function<ExtFuture<void>()> m_continuation_function {nullptr};

};


#endif /* UTILS_CONCURRENCY_EXTFUTURE_H_ */
