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


PerfectDeleter::PerfectDeleter(QObject* parent) : QObject(parent)
{
	m_future_synchronizer.setCancelOnWait(true);
}

PerfectDeleter::~PerfectDeleter()
{

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
	scan_and_purge_futures();
	qIno() << "Waiting for" << m_future_synchronizer.futures().size() << "canceled QFuture<void>'s to finish...";
	m_future_synchronizer.waitForFinished();
	qIno() << "Wait complete.";

	// Wait for the AMLMJobs to finish.
	/// @todo Probably need to keep event loop running here.
	waitForAMLMJobsFinished(true);
}

void PerfectDeleter::addQFuture(QFuture<void> f)
{
	std::lock_guard lock(m_mutex);

	m_num_qfutures_added_since_last_purge++;
	m_total_num_added_qfutures++;

	m_future_synchronizer.addFuture(f);

	if(m_total_num_added_qfutures % 16 == 0)
	{
		qIno() << "Total added QFutures:" << m_total_num_added_qfutures;
		qIno() << "Total unfinished QFutures:" << m_future_synchronizer.futures().size();
		qIno() << "Number of QFutures added since last purge:" << m_num_qfutures_added_since_last_purge;
	}

	// Periodically purge completed futures.
	if(m_num_qfutures_added_since_last_purge > m_purge_futures_count)
	{
		qIno() << "Purging QFutures...";
		scan_and_purge_futures();
		m_num_qfutures_added_since_last_purge = 0;
	}
}

void PerfectDeleter::addKJob(KJob* kjob)
{
	std::lock_guard lock(m_mutex);

	auto remover_lambda = [=](QObject* obj) {
			qDb() << "KJob destroyed";
			std::lock_guard lock(m_mutex);
			m_watched_KJobs.erase(std::remove(m_watched_KJobs.begin(), m_watched_KJobs.end(), obj),
					m_watched_KJobs.end());
		};

	// Connect a signal/slot to remove the Kjob* if it gets deleted.
	connect_or_die(kjob, &QObject::destroyed, this, remover_lambda);
	connect_or_die(kjob, &KJob::finished, this, remover_lambda);

	m_watched_KJobs.push_back(kjob);
}

void PerfectDeleter::addAMLMJob(AMLMJob* amlmjob)
{
	std::lock_guard lock(m_mutex);

	// Remover lambda and connections, for when the AMLMJob is deleted out from under us.
	// This is a bit of a mess due to this whole "let's delete ourself" thing that permeates Qt5/KF5.
	// Need to hook into two signals which don't really give us the info we need to un-crashably remove the
	// registered pointers from the storage here.

	auto remover_lambda = [=](QObject* obj) {
		qDb() << "AMLMJob destroyed";
		std::lock_guard lock(m_mutex);
		m_watched_AMLMJobs.erase(std::remove(m_watched_AMLMJobs.begin(), m_watched_AMLMJobs.end(), obj),
				m_watched_AMLMJobs.end());
	};

	connect_or_die(amlmjob, &QObject::destroyed, this, remover_lambda);
	connect_or_die(amlmjob, &AMLMJob::finished, this, remover_lambda);

	m_watched_AMLMJobs.push_back(amlmjob);
}

std::vector<std::tuple<QString, long> > PerfectDeleter::stats() const
{
	std::lock_guard lock(m_mutex);

	// QFutureSynchronizer<> stats.
	int num_futures = m_future_synchronizer.futures().size();

	return {{tr("Watched QFuture<void>s"), num_futures}};

}

bool PerfectDeleter::waitForAMLMJobsFinished(bool spin)
{
	long remaining_amlmjobs = 0;

	do
	{
		// One pass through the AMLMJob list.
		for(AMLMJob* i : m_watched_AMLMJobs)
		{
			// Do nothing, the connections should take care of the deletion.
			remaining_amlmjobs++;
		}

		qIno() << "Remaining AMLMJobs:" << remaining_amlmjobs;

		if(remaining_amlmjobs > 0)
		{
			return false;
		}
		else
		{
			return true;
		}

	} while(spin);
}

void PerfectDeleter::scan_and_purge_futures()
{
	/// @note Requires m_mutex to already be held.

	QList<QFuture<void>> futures = m_future_synchronizer.futures();
	auto num_starting_qfutures = futures.size();

	QList<QFuture<void>> unfinished_futures =
			QtConcurrent::blockingFiltered(futures, [](const QFuture<void>& f) -> bool {
		// Keep by returning true.
				return (!(f.isCanceled() || f.isFinished()));
			});

	qDb() << "Clearing futures:" << m_future_synchronizer.futures().size();
	m_future_synchronizer.clearFutures();
	qDb() << "Cleared futures:" << m_future_synchronizer.futures().size();

	// Add back the unfinished futures.
	for(QFuture<void>& f : unfinished_futures)
	{
		m_future_synchronizer.addFuture(f);
	}

	auto num_removed_futures = num_starting_qfutures - m_future_synchronizer.futures().size();
	auto num_remaining_futures = m_future_synchronizer.futures().size();

	qIno() << "Purged" << num_removed_futures << "canceled/finished futures, "
		   << num_remaining_futures << "==" << m_future_synchronizer.futures().size() << "remaining";

}
