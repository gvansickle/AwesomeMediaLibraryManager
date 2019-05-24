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

#include "ManagedExtFutureWatcher_impl.h"

// Qt5
#include <QThread>

// Ours
#include <utils/DebugHelpers.h>
#include <utils/ConnectHelpers.h>



QThread* ManagedExtFutureWatcher_detail::BackpropThreadManager::get_backprop_qthread()
{
	return priv_instance();
}

QThread* ManagedExtFutureWatcher_detail::BackpropThreadManager::priv_instance()
{
	static QThread* backprop_thread = []{
		QThread* new_thread = new QThread;
		new_thread->setObjectName("ExtFutureBackpropThread");
		PerfectDeleter::instance().addQThread(new_thread);
#if 0
				, [](QThread* the_qthread){
			// Call exit(0) on the QThread.  We use Qt's invokeMethod() here.
			ExtAsync::detail::run_in_event_loop(the_qthread, [the_qthread](){
				qDb() << "Calling quit()+wait() on managed FutureWatcher QThread, FWParent has num children:" << f_the_managed_fw_parent->children().size();
				the_qthread->quit();
				the_qthread->wait();
				qDb() << "Finished quit()+wait() on managed FutureWatcher QThread";
			});
		});
#endif
		// No parent, this will be eventually deleted by the signal below.
		f_the_managed_fw_parent = new FutureWatcherParent();
		// Create and push the future watcher parent object into the new thread.
		f_the_managed_fw_parent->moveToThread(new_thread);
		// Connect QThread::finished to QObject::deleteLater.
		connect_or_die(new_thread, &QThread::finished, f_the_managed_fw_parent, &QObject::deleteLater);

		// Start the thread.
		new_thread->start();
		return new_thread;
	}();

	return backprop_thread;
}
