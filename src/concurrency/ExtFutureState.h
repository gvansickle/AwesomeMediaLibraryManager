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

// Qt5
#include <QtCore>
#include <QFlags>
#include <QFutureInterface>

// Ours
#include <utils/QtHelpers.h>

class ExtFutureState
{
    Q_GADGET

public:

    enum State
    {
        NoState = QFutureInterfaceBase::NoState,
        Running = QFutureInterfaceBase::Running,
        Started = QFutureInterfaceBase::Started,
        Finished = QFutureInterfaceBase::Finished,
        Canceled = QFutureInterfaceBase::Canceled,
        Paused = QFutureInterfaceBase::Paused,
        Throttled = QFutureInterfaceBase::Throttled
    };
    Q_DECLARE_FLAGS(States, State)
    // Exposes the enum to the Meta-Object System.
    Q_FLAG(States)

    /**
     * Return the combined state flags of a class ultimately derived from QFutureInterfaceBase.
     */
    template<typename T>
    static ExtFutureState::States state(T& qfuture_int_base_derived)
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
        const static QFutureInterfaceBase::State c_states[] = {
            QFutureInterfaceBase::State::NoState,
            QFutureInterfaceBase::State::Running,
            QFutureInterfaceBase::State::Started,
            QFutureInterfaceBase::State::Finished,
            QFutureInterfaceBase::State::Canceled,
            QFutureInterfaceBase::State::Paused,
            QFutureInterfaceBase::State::Throttled
        };


        ExtFutureState::States retval {State::NoState};
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
    static ExtFutureState::States state(QFuture<T>& qfuture_derived)
    {
        return state(qfuture_derived.d);
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ExtFutureState::States)


#endif /* SRC_CONCURRENCY_EXTFUTURESTATE_H_ */
