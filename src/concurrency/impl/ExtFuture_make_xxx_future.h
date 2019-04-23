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

/**
 * @file ExtFuture_make_xxx_future.h
 */
#ifndef SRC_CONCURRENCY_IMPL_EXTFUTURE_MAKE_XXX_FUTURE_H_
#define SRC_CONCURRENCY_IMPL_EXTFUTURE_MAKE_XXX_FUTURE_H_

// Std C++
#include <type_traits>
#include <atomic>

// Qt5
#include <QFutureInterface>
#include <QException>

template <class T>
class ExtFuture;

namespace ExtAsync
{
namespace detail
{
	inline static std::atomic_uint64_t get_next_id()
	{
		static std::atomic_uint64_t f_future_id_ctr {2};
		return f_future_id_ctr.fetch_add(1);
	}
}

/// @name ExtFuture<T> make_xxx_future() functions.
/// @{

/**
 * Creates a completed future containing the value @a value.
 *
 * @param value
 * @return  A ready ExtFuture<deduced_type_t<T>>();
 */
/**
 * Create and return a finished future of type ExtFuture<T>.
 *
 * Intended to be a mostly-work-alike to std::experimental::make_ready_future.
 * @see http://en.cppreference.com/w/cpp/experimental/make_ready_future
 *
 * @todo Specialize for void, or use Unit.
 * @todo return type decay rules when decay<T> is ref wrapper.
 *
 * @param value
 * @return
 */
template<typename T>
auto make_ready_future(T&& value) -> ExtFuture<std::decay_t<T>>
{
	QFutureInterface<T> qfi;

	qfi.reportStarted();
	qfi.reportResult(std::forward<T>(value));
	qfi.reportFinished();

	return 	ExtFuture<T>(&qfi, detail::get_next_id());
}

/**
 * Same as above, but with a QList<T> from an upstream ExtFuture<T>.
 */
template<typename T>
auto make_ready_future_from_qlist(QList<T>&& value) -> ExtFuture<std::decay_t<T>>
{
	QFutureInterface<T> qfi;

	qfi.reportStarted();
	qfi.reportResults(QVector<T>::fromList(value));
	qfi.reportFinished();

	return 	ExtFuture<T>(&qfi, detail::get_next_id());
}

template <class T, class E,
		  REQUIRES(!is_ExtFuture_v<T>
		  && std::is_base_of_v<QException, E>)>
auto make_exceptional_future(const E & exception) -> ExtFuture<std::decay_t<T>>
{
	QFutureInterface<T> qfi;

	qfi.reportStarted();
	qfi.reportException(exception);
	qfi.reportFinished();

	return ExtFuture<T>(&qfi, detail::get_next_id());
}

/**
 * Helper which returns a (Started) ExtFuture<T>.
 */
template <typename T>
auto make_started_only_future() -> ExtFuture<std::decay_t<T>>
{
	static_assert(!is_ExtFuture_v<T>, "ExtFuture<T>: T cannot be a nested ExtFuture");
	// QFutureInterface<T> starts out with a state of NoState.
	QFutureInterface<T> fi;
	fi.reportStarted();
//	Q_ASSERT(ExtFutureState::state(fi) == ExtFutureState::Started) << state(fi);
	return ExtFuture<T>(&fi, detail::get_next_id());
}

} // END ExtAsync



#endif /* SRC_CONCURRENCY_IMPL_EXTFUTURE_MAKE_XXX_FUTURE_H_ */
