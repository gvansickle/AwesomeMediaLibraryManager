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
 * @file WorkerThreadControllerBase.cpp
 */
#include "WorkerThreadControllerBase.h"

// Ours
#include <utils/ConnectHelpers.h>
#include "WorkerThreadBase.h"

namespace ExtAsync
{

WorkerThreadControllerBase::WorkerThreadControllerBase()
{
	WorkerThreadBase *worker = new WorkerThreadBase();
	worker->moveToThread(&m_worker_thread);
	connect_or_die(&m_worker_thread, &QThread::finished, worker, &QObject::deleteLater);
	connect_or_die(this, &WorkerThreadControllerBase::operate, worker, &WorkerThreadBase::doWork);
	connect_or_die(worker, &WorkerThreadBase::resultReady, this, &WorkerThreadControllerBase::SLOT_handle_results);
	m_worker_thread.start();
}

WorkerThreadControllerBase::~WorkerThreadControllerBase()
{
	m_worker_thread.quit();
	m_worker_thread.wait();
}

void WorkerThreadControllerBase::SLOT_handle_results(const QString&)
{

}



} /* namespace ExtAsync */
