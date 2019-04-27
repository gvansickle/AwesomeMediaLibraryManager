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
#include <QTextCursor>
#include <QTextList>
#include <algorithm>

// Future Std C++
#include <future/future_algorithms.h> ///< For Uniform Container Erasure.

// Ours
#include <concurrency/ExtFuturePropagationHandler.h>


PerfectDeleter::PerfectDeleter(QObject* parent) : QObject(parent)
{
	setObjectName("ThePerfectDeleter");

	m_future_synchronizer.setCancelOnWait(true);
}

PerfectDeleter::~PerfectDeleter()
{
	qDb() << objectName() << "destroyed.";
}

PerfectDeleter& PerfectDeleter::instance(QObject* parent)
{
	// First caller must specify a parent, or we'll assert here.
	static PerfectDeleter* m_the_instance = (Q_ASSERT(parent != nullptr), new PerfectDeleter(parent));

	return *m_the_instance;
}

void PerfectDeleter::cancel_and_wait_for_all()
{
	std::lock_guard lock(m_mutex);

	// Ok, we're going down, so try to do so with as little leakage as possible.

	// First print some stats.
	qIno() << "END OF PROGRAM SUMMARY OF OPEN RESOURCES";
	auto stats_text_doc = stats();
	for(const auto& line : qAsConst(stats_text_doc))
	{
		qIno() << line;
	}

	// Cancel all registered AMLMJobs.
	qIno() << "Killing" << m_watched_AMLMJobs.size() << "AMLMJobs";
	for(AMLMJob* i : m_watched_AMLMJobs)
	{
		if(i != nullptr)
		{			
			// Killing them softly is probably the right way to go here.
			i->kill(KJob::KillVerbosity::Quietly);
		}
	}

	// Cancel all registered KJobs.
	qIno() << "Killing" << m_watched_KJobs.size() << "KJobs";
	for(KJob* i : m_watched_KJobs)
	{
		if(i != nullptr)
		{
			// Killing them softly is probably the right way to go here.
			i->kill(KJob::KillVerbosity::Quietly);
		}
	}

	// Wait for the QFutures to finish.
	/// @todo Need to keep event loop running here?
//	scan_and_purge_futures();
	qIno() << "Canceling" << m_future_synchronizer.futures().size() << "QFuture<void>'s and waiting for them to finish...";
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
	/// @todo This is the most wrong part, this will cancel futures which may still have consumers.
//	if(m_num_qfutures_added_since_last_purge > m_purge_futures_count)
//	{
//		qIno() << "Purging QFutures...";
//		scan_and_purge_futures();
//		m_num_qfutures_added_since_last_purge = 0;
//	}
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
		qIn() << "AMLMJob destroyed:" << obj->objectName();
		std::lock_guard lock(m_mutex);
		m_watched_AMLMJobs.erase(std::remove(m_watched_AMLMJobs.begin(), m_watched_AMLMJobs.end(), obj),
				m_watched_AMLMJobs.end());
	};

M_WARNING("These both want to remove the same amlmjob, maybe ok?");
	connect_or_die(amlmjob, &QObject::destroyed, this, remover_lambda);
	connect_or_die(amlmjob, &AMLMJob::finished, this, remover_lambda);

	m_watched_AMLMJobs.push_back(amlmjob);
}

void PerfectDeleter::addQThread(QThread* qthread)
{
	std::lock_guard lock(m_mutex);

	// QThread is a QObject which will ordinarily have a parent.  However, per the docs, QThread::create() is
	// somewhat different: the caller is suppose to take ownership.  So that's why this exists.

	// First let's check if we can immediately delete it.
	if(qthread->isFinished())
	{
		qthread->deleteLater();
	}
	else
	{
		// From Qt docs:
		// "When this signal is emitted, the event loop has already stopped running. No more events will be processed in the thread,
		// except for deferred deletion events. This signal can be connected to QObject::deleteLater(), to free objects in that thread."
		connect_or_die(qthread, &QThread::finished, this, [=](){
			qIn() << "Deleting QThread:" << qthread;
			/// @todo Uniquify, don't rely on pointer.
			std::experimental::erase(m_watched_QThreads, qthread);
			qthread->deleteLater();
			});
		m_watched_QThreads.push_back(qthread);
	}
}

//template <class T>
//QTextTable& operator<<(QTextTable& table, T NextColumnText)
//{
//	if constexpr(NextColumnText == std::endl)
//	{
//		// EOL, insert a new row.

//	}
//	else if(std::is_convertible_v<T, const QString&>)
//	{

//	}
//	else
//	{
//		static_assert(dependent_false<T>, "")
//	}
//}

#if 0
QTextDocument* PerfectDeleter::stats() const
{
	/// @todo This should really be a Threadsafe Interface pattern.
//	std::lock_guard lock(m_mutex);

	// Generate and return a stats object.
	QTextDocument* retval = new QTextDocument();
	QTextCursor cursor(retval);
	cursor.movePosition(QTextCursor::Start);
	QTextList* list = cursor.insertList(QTextListFormat::ListDisc);

	cursor.insertText(tr("Num QThreads: %1").arg(m_watched_QThreads.size()));

	// QFutureSynchronizer<> stats.
	int num_futures = m_future_synchronizer.futures().size();

	cursor.insertText(tr("Total added QFutures: %1").arg(m_total_num_added_qfutures));
	cursor.insertText(tr("Total unfinished QFutures: %1").arg(m_future_synchronizer.futures().size()));
	cursor.insertText(tr("Number of QFutures added since last purge: %1").arg(m_num_qfutures_added_since_last_purge));

	cursor.insertText(tr("Watched QFuture<void>s: %1").arg(num_futures));

	return retval;
}
#endif
QStringList PerfectDeleter::stats() const
{
	/// @todo This should really be a Threadsafe Interface pattern.
//	std::lock_guard lock(m_mutex);

	// Generate and return a stats object.
	QStringList retval;

	retval << tr("Num QThreads: %1").arg(m_watched_QThreads.size());

	// QFutureSynchronizer<> stats.
	int num_futures = m_future_synchronizer.futures().size();

	retval << tr("Total added QFutures: %1").arg(m_total_num_added_qfutures);
	retval << tr("Total unfinished QFutures: %1").arg(m_future_synchronizer.futures().size());
	retval << tr("Number of QFutures added since last purge: %1").arg(m_num_qfutures_added_since_last_purge);

	retval << tr("Watched QFuture<void>s: %1").arg(num_futures);

	return retval;
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

#if 0
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
#endif
