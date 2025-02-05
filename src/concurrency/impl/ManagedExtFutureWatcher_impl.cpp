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
 * @file ManagedExtFutureWatcher_impl.cpp
 */

#if 0 // !QT6

#include "ManagedExtFutureWatcher_impl.h"

// Qt5
#include <QThread>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/ConnectHelpers.h>

namespace ManagedExtFutureWatcher_detail
{

QThread* BackpropThreadManager::get_backprop_qthread()
{
	auto retval = priv_instance();
	return retval;
}

FutureWatcherParent* BackpropThreadManager::getFutureWatcherParent()
{
	/// This is semi-gross, it's the QObject which will be the parent of all managed future watchers.
	static FutureWatcherParent* ms_the_managed_fw_parent = new FutureWatcherParent();
	return ms_the_managed_fw_parent;
}

// Static
QThread* BackpropThreadManager::priv_instance()
{
	static QThread* backprop_thread = []{
		QThread* new_thread = new QThread;
		new_thread->setObjectName("ExtFutureBackpropThread");
		PerfectDeleter::instance().addQThread(new_thread);
#if 0
				, [](QThread* the_qthread){
			// Call exit(0) on the QThread.  We use Qt's invokeMethod() here.
			ExtAsync::detail::run_in_event_loop(the_qthread, [the_qthread](){
				qDb() << "Calling quit()+wait() on managed FutureWatcher QThread, FWParent has num children:" << ms_the_managed_fw_parent->children().size();
				the_qthread->quit();
				the_qthread->wait();
				qDb() << "Finished quit()+wait() on managed FutureWatcher QThread";
			});
		});
#endif
		// No parent, this will be eventually deleted by the signal below.
		FutureWatcherParent* the_managed_fw_parent = get_future_watcher_parent();
		Q_ASSERT(the_managed_fw_parent != nullptr);
		// Create and push the future watcher parent object into the new thread.
		the_managed_fw_parent->moveToThread(new_thread);
		// Connect QThread::finished to QObject::deleteLater.
		connect_or_die(new_thread, &QThread::finished, the_managed_fw_parent, &QObject::deleteLater);

		// Start the thread.
		new_thread->start();
		return new_thread;
	}();

	return backprop_thread;
}


BackpropThreadManager* get_backprop_thread_manager()
{
	static BackpropThreadManager* btm = new BackpropThreadManager();
	return btm;
}

/// Returns the pointer to the QThread which is to be used for running QFutureWatchers which
/// implement inter-future status propagation (cancellation and exceptions).
QThread* get_backprop_qthread()
{
	BackpropThreadManager* btm = get_backprop_thread_manager();
	return btm->get_backprop_qthread();
}

FutureWatcherParent* get_future_watcher_parent()
{
	BackpropThreadManager* btm = get_backprop_thread_manager();
	return btm->getFutureWatcherParent();
}

}; // END ns

#endif // !QT6
