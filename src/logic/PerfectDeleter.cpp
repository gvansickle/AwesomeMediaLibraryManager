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

PerfectDeleter* PerfectDeleter::s_instance { nullptr };

PerfectDeleter::PerfectDeleter(QObject* parent) : QObject(parent)
{
	// Set the singleton ptr.
	s_instance = this;
}

PerfectDeleter::~PerfectDeleter()
{

}

PerfectDeleter* PerfectDeleter::instance()
{
	return (s_instance != nullptr ) ? s_instance : new PerfectDeleter(qApp);
}

void PerfectDeleter::destroy()
{
	// Delete the singleton.
	// This is intended to be called by the owner's (app class's) destructor.
	if(s_instance != nullptr)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

void PerfectDeleter::cancel_and_wait_for_all()
{
	std::lock_guard lock(m_mutex);

	// Cancel all the QFutures.
	for(QFuture<void>& i : m_watched_qfutures)
	{
		i.cancel();
	}

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
	for(QFuture<void>& i : m_watched_qfutures)
	{
		i.waitForFinished();
	}

	// Wait for the AMLMJobs to finish.
	/// @todo Probably need to keep event loop running here.
//	for(QFuture<void>& i : m_watched_qfutures)
//	{
//		i.waitForFinished();
//	}
}

void PerfectDeleter::addQFuture(QFuture<void> f)
{
	std::lock_guard lock(m_mutex);

	// Connect a QFutureWatcher signal/slot to remove the QFuture<> if it gets deleted.
//	connect_or_die(f, &QFuture<void>::destroyed, this, [=](){
//		qDb() << "QFuture destroyed";
//	});

	m_watched_qfutures.push_back(f);
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
