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

#include "PerfectDeleter.h"

// Std C++
#include <algorithm>

//PerfectDeleter* PerfectDeleter::s_instance { nullptr };

PerfectDeleter::PerfectDeleter(QObject* parent) : QObject(parent)
{
	m_future_synchronizer.setCancelOnWait(true);
}

PerfectDeleter::~PerfectDeleter()
{

}

//PerfectDeleter* PerfectDeleter::instance()
//{
//	return (s_instance != nullptr ) ? s_instance : new PerfectDeleter(qApp);
//}

void PerfectDeleter::destroy()
{
	// Delete the singleton.
	// This is intended to be called by the owner's (app class's) destructor.
//	if(s_instance != nullptr)
//	{
//		delete s_instance;
//		s_instance = nullptr;
//	}
}

void PerfectDeleter::cancel_and_wait_for_all()
{
	std::lock_guard lock(m_mutex);

	// Cancel all registered AMLMJobs.
	for(AMLMJob* i : m_watched_AMLMJobs)
	{
		if(i != nullptr)
		{
			// Killing them softly is probably the right way to go here.
			i->kill(KJob::KillVerbosity::Quietly);
		}
	}

	// Wait for the QFutures to finish.
	/// @todo Need to keep event loop running here?
	m_future_synchronizer.waitForFinished();

	// Wait for the AMLMJobs to finish.
	/// @todo Probably need to keep event loop running here.
//	for(QFuture<void>& i : m_watched_qfutures)
//	{
//		i.waitForFinished();
//	}
}

void PerfectDeleter::addQFuture(QFuture<void> f)
{
//	Q_ASSERT(this == s_instance);
	qDb() << "Adding QFuture";
	std::lock_guard lock(m_mutex);
	qDb() << "Locked";

	m_future_synchronizer.addFuture(f);
	qDb() << "Added QFuture";

	/// @todo Do we need to periodically purge completed futures?
}

void PerfectDeleter::addKJob(KJob* kjob)
{
	std::lock_guard lock(m_mutex);
	// Connect a signal/slot to remove the QFuture<> if it gets deleted.
	/*QObject::*/connect_or_die(kjob, &QObject::destroyed, [=](QObject* obj) {
		qDb() << "KJob destroyed";
		std::lock_guard lock(m_mutex);
		m_watched_KJobs.erase(std::remove(m_watched_KJobs.begin(), m_watched_KJobs.end(), obj),
				m_watched_KJobs.end());
	});
	m_watched_KJobs.push_back(kjob);
}

void PerfectDeleter::addAMLMJob(AMLMJob* amlmjob)
{
	std::lock_guard lock(m_mutex);
	/*QObject::*/connect_or_die(amlmjob, &QObject::destroyed, [=](QObject* obj) {
		qDb() << "AMLMJob destroyed";
		std::lock_guard lock(m_mutex);
		m_watched_AMLMJobs.erase(std::remove(m_watched_AMLMJobs.begin(), m_watched_AMLMJobs.end(), obj),
				m_watched_AMLMJobs.end());
	});
	m_watched_AMLMJobs.push_back(amlmjob);
}

std::vector<std::tuple<QString, long> > PerfectDeleter::stats() const
{
	std::lock_guard lock(m_mutex);

	// QFutureSynchronizer<> stats.
	int num_futures = m_future_synchronizer.futures().size();

	return {{tr("Watched QFuture<void>s"), num_futures}};

}
