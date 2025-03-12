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
 * @file IExtFutureWatcher.h
 */

#ifndef SRC_CONCURRENCY_IEXTFUTUREWATCHER_H_
#define SRC_CONCURRENCY_IEXTFUTUREWATCHER_H_

#if 0 // QT6?

#include <QObject>
class KJob;

/**
 * Abstract interface for ExtFuture<T> signals.  Inherit from this to emit these signals.  Unbelievable PITA.
 * https://stackoverflow.com/questions/17943496/declare-abstract-signal-in-interface-class?noredirect=1&lq=1
 * https://stackoverflow.com/questions/39186348/connection-of-pure-virtual-signal-of-interface-class?rq=1
 *
 * @todo Do we need this?
 * @todo Doesn't look like it, but: I don't get what's going on here wrt moc.  It's complaining that this doesn't have a
 * Q_OBJECT in it and signals can't be virtual, but this is the exact pattern used by e.g. KDevelop's IStatus interface.
 * @update ...splitting this into its own file makes the problem go away.  WTH???
 */
class IExtFutureWatcher
{
public:
    virtual ~IExtFutureWatcher();

Q_SIGNALS:
    /// Signal from ExtFuture<T>.
	virtual void SIGNAL_resultsReadyAt(int begin, int end) = 0;

    /// KJob signals.
    virtual void finished(KJob *job) = 0;
    virtual void result(KJob *job) = 0;

    // QObject signals.
//    virtual void destroyed(QObject* obj) = 0;
};
Q_DECLARE_METATYPE(IExtFutureWatcher*)
Q_DECLARE_INTERFACE(IExtFutureWatcher, "io.github.gvansickle.awesomemedialibrarymanager.IExtFutureWatcher")

#endif

#endif /* SRC_CONCURRENCY_IEXTFUTUREWATCHER_H_ */
