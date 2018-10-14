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

#ifndef SRC_CONCURRENCY_EXTFUTURESTATE_H_
#define SRC_CONCURRENCY_EXTFUTURESTATE_H_

#include <config.h>

// Std C++ backfill
#include <future/cpp14_concepts.hpp>

// Qt5
#include <QtCore>
#include <QFlags>
#include <QFutureInterface>

// Ours
#include <utils/QtHelpers.h>
#include <utils/StringHelpers.h>
#include "ExtAsync_traits.h"

class ExtFutureState
{
    Q_GADGET

public:

	/// Current ExtFuture state flags.
    enum StateFlag
    {
        NoState = QFutureInterfaceBase::NoState,
        Running = QFutureInterfaceBase::Running,
        Started = QFutureInterfaceBase::Started,
        Finished = QFutureInterfaceBase::Finished,
        Canceled = QFutureInterfaceBase::Canceled,
        Paused = QFutureInterfaceBase::Paused,
        Throttled = QFutureInterfaceBase::Throttled
    };
    Q_DECLARE_FLAGS(State, StateFlag)
    // Exposes the enum to the Meta-Object System.
    Q_FLAG(State)

    /**
     * Return the combined state flags of a class ultimately derived from QFutureInterfaceBase.
     */
	template<typename T, REQUIRES(!is_ExtFuture_v<T>)>
    static State state(const T& qfuture_int_base_derived)
    {
        QMutexLocker lock(qfuture_int_base_derived.mutex());
        // States from QFutureInterfaceBase.
        /// @note The actual state variable is a public member of QFutureInterfaceBasePrivate (in qfutureinterface_p.h),
        ///       but an instance of that class is a private member of QFutureInterfaceBase, i.e.:
        ///			#ifndef QFUTURE_TEST
        ///			private:
        ///			#endif
        ///				QFutureInterfaceBasePrivate *d;
        /// So we pretty much have to use this queryState() loop here, which is unfortunate since state is
        /// actually a QAtomicInt, so we'd already be thread-safe here without the mutex if we could get at it.

    //    std::vector<std::pair<QFutureInterfaceBase::State, const char*>> list = {
    //        {QFutureInterfaceBase::NoState, "NoState"},
    //        {QFutureInterfaceBase::Running, "Running"},
    //        {QFutureInterfaceBase::Started,  "Started"},
    //        {QFutureInterfaceBase::Finished,  "Finished"},
    //        {QFutureInterfaceBase::Canceled,  "Canceled"},
    //        {QFutureInterfaceBase::Paused,   "Paused"},
    //        {QFutureInterfaceBase::Throttled, "Throttled"}
    //    };
        constexpr static QFutureInterfaceBase::State c_states[] = {
            QFutureInterfaceBase::State::NoState,
            QFutureInterfaceBase::State::Running,
            QFutureInterfaceBase::State::Started,
            QFutureInterfaceBase::State::Finished,
            QFutureInterfaceBase::State::Canceled,
            QFutureInterfaceBase::State::Paused,
            QFutureInterfaceBase::State::Throttled
        };


        ExtFutureState::State retval {StateFlag::NoState};
        for(QFutureInterfaceBase::State i : c_states)
        {
            if(qfuture_int_base_derived.queryState(i))
            {
                auto bitval = QFlag(i);
                retval |= bitval;
            }
        }

        return retval;
    }

    /**
     * Return the combined state flags of a class ultimately derived from QFuture<T>.
     */
    template<typename T>
	static State state(const QFuture<T>& qfuture_derived)
    {
        return state(qfuture_derived.d);
    }

//	/// Free inline friend streaming operator to QDebug.
//	friend QDebug operator<<(QDebug dbg, const ExtFutureState::State& state)
//	{
//	    QDebugStateSaver saver(dbg);
//
//		dbg << toqstr<ExtFutureState::State>(state);
//
//	    return dbg;
//	}

};

Q_DECLARE_OPERATORS_FOR_FLAGS(ExtFutureState::State)

/// Free streaming operator to let Google Test print ExtFutureState's in text format.
static std::ostream& operator<<(std::ostream& os, const ExtFutureState::State& state)
{
	return os << "(" << toqstr(state).toStdString() << ")";

	// Or if we want to get the same output that qDebug() would get, we could use something like this:
//	QString str;
//	QDebug dbg(&str);
//	dbg << state;
//	TCOUT << "qDb():" << str;

}

/**
 * QDebug stream operator for QFutureInterface<T>'s.
 */
template <typename T>
QDebug operator<<(QDebug dbg, const QFutureInterface<T> &qfi)
{
    QDebugStateSaver saver(dbg);

    dbg << "QFutureInterface<T>(" << ExtFutureState::state(qfi) << ")";

    return dbg;
}

/**
 * QDebug stream operator for QFutureInterfaceBase's.
 */
template <typename T>
QDebug operator<<(QDebug dbg, const QFutureInterfaceBase &qfi)
{
    QDebugStateSaver saver(dbg);

    dbg << "QFutureInterfaceBase(" << ExtFutureState::state(qfi) << ")";

    return dbg;
}

#endif /* SRC_CONCURRENCY_EXTFUTURESTATE_H_ */
