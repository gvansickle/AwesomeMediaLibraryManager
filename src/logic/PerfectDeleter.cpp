/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Future Std C++
#include <future/future_algorithms.h> ///< For Uniform Container Erasure.

// Qt5


// Ours
#include <utils/DebugHelpers.h>
#include <concurrency/AMLMJob.h>

/**
 * Some notes on resource management in the bizzaro-world of Qt5:
 *
 * - QObject: What happens if it's deleted while it has outgoing signals pending?
 * @link https://stackoverflow.com/questions/4888189/how-delete-and-deletelater-works-with-regards-to-signals-and-slots-in-qt?rq=1
 * @link https://doc.qt.io/qt-5/qobject.html#deleteLater
 * ... Not clear.  Rumors indicate that the signals will be delivered.
 *
 * - Waiting on futures to complete (ctx: QtConcurrent::run()).
 * @link https://forum.qt.io/topic/53481/how-to-release-the-memory-in-qtconcurrent-the-new-thread-func/2
 * "So the steps are:

    1. Set QGuiApplication::setQuitOnLastWindowClosed( false ) so that to prevent the application from quitting
        immediately after the last of its windows is closed.
    2. Connect a slot to the QGuiApplication::lastWindowClosed() signal.
    3. Create a QFutureWatcher< void >* (using new) for every QFuture< void > you create using QtConcurrent::run and
        keep the instances of the future watchers in the object which receives the last window closed notification.
    4. Hook to the finished() signal of the QFutureWatcher< void > in the object which also tracks whether application
        quit has been requested. Remove the kept instance of the QFutureWatcher
    5. Upon receiving the last window closed notification set a flag in the same object that an application quit has
        been requested - the last window has been closed.
    6. On each QFutureWatcher< void >::finished() and on QGuiApplication::lastWindowClosed() check if there are no
        cached QFutureWatcher's e.g. no futures are running.
 *	"
 *
 */



	// QObjectCleanupHandler does this on add:
//	if (!object)
//	        return 0;
//
//	    connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
//	    cleanupObjects.insert(0, object);


	// remove() does this, but doesn't delete the object:
//	int index;
//	    if ((index = cleanupObjects.indexOf(object)) != -1) {
//	        cleanupObjects.removeAt(index);
//	        disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
//	    }
	// The objectDestroyed() slot just calls remove().

	// Clear actually deletes the object.


class DeletableQObject : public DeletableBase
{
	using BASE_CLASS = DeletableBase;

public:
	DeletableQObject(PerfectDeleter* pd, std::deque<std::shared_ptr<DeletableQObject>>* list, QObject* to_be_deleted)
		: DeletableBase(pd), m_list(list), m_to_be_deleted(to_be_deleted) {};
	~DeletableQObject() override {};

	void cancel() override {  };
	bool poll_wait() override { return true /** @todo */; };
	void remove() override
	{
		// Is object in the list?
		auto it = std::find_if(m_list->begin(), m_list->end(), [&](std::shared_ptr<DeletableQObject> p){ return *p == *this; });
		if(it != m_list->end())
		{
			// Remove the entry.
			// Can't stream name out, it's been deleted.
//			qIn() << "Removing QObject:" << (*it)->object()->objectName();
			std::experimental::erase(*m_list, *it);
			m_to_be_deleted->disconnect(m_pd);
		}
	};

	bool holds_object(const QObject* object) { return m_to_be_deleted == object; }

	virtual QObject* object() const { return m_to_be_deleted; };

protected:

	bool equality_op(const DeletableBase& other) const override
	{
		DeletableQObject const * other_p = dynamic_cast<DeletableQObject const *>(&other);
		if(other_p == nullptr)
		{
			// Not a comparable type.
			return false;
		}
		return m_to_be_deleted == other_p->m_to_be_deleted;
	}

public:
	QObject* m_to_be_deleted;
	std::deque<std::shared_ptr<DeletableQObject>>* m_list;
};

class DeletableAMLMJob : public DeletableQObject
{
	using BASE_CLASS = DeletableQObject;

public:
	DeletableAMLMJob(PerfectDeleter* pd, std::deque<std::shared_ptr<DeletableQObject>>* list, AMLMJob* to_be_deleted)
		: DeletableQObject(pd, list, to_be_deleted) {};
	~DeletableAMLMJob() override {};

	void cancel() override
	{
		// Killing them softly is probably the right way to go here.
		object()->kill(KJob::KillVerbosity::Quietly);
	};
	bool poll_wait() override { return true /** @todo */; };

	AMLMJob* object() const override { return dynamic_cast<AMLMJob*>(m_to_be_deleted); };
};

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
	auto stats_text = stats_internal();
	for(const auto& line : qAsConst(stats_text))
	{
		qIno() << line;
	}

	// Cancel all registered AMLMJobs.
	qIno() << "Killing" << m_watched_AMLMJobs.size() << "AMLMJobs";
	for(std::shared_ptr<DeletableBase> it : m_watched_AMLMJobs)
	{
		it->cancel();
//		auto it2 = std::dynamic_pointer_cast<DeletableAMLMJob>(it);
//		if(!it2)
//		{
//			continue;
//		}
//		AMLMJob* i = it2->object();//m_to_be_deleted;
//		if(i != nullptr)
//		{
//			// Killing them softly is probably the right way to go here.
//			i->kill(KJob::KillVerbosity::Quietly);
//		}
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
	m_future_synchronizer.clearFutures();
	qIno() << "QFuture<void> wait complete.";

	// Wait for the AMLMJobs to finish.
	/// @todo Probably need to keep event loop running here.
	waitForAMLMJobsFinished(true);

	// Cancel all "deletables".
	qIno() << "Canceling" << m_watched_deletables.size() << "Deletables";
	for(auto& d : m_watched_deletables)
	{
		d->cancel();
	}
	// Wait for them to be canceled and deleted.
}

bool PerfectDeleter::empty() const
{
	return (size() == 0);
}

size_t PerfectDeleter::size() const
{
	std::scoped_lock sl(m_mutex);
	auto total_deletables = m_watched_KJobs.size() + m_watched_AMLMJobs.size()
			+ m_watched_QThreads.size() + m_watched_deletables.size();
	return total_deletables;
}


template <class QObjectDerivedType, class ListType>
void addQObjectDerivedType(PerfectDeleter* pd, QObjectDerivedType* object, ListType* list, std::mutex& the_mutex)
{
	if(object == nullptr)
	{
		// Shouldn't have been called, warn.
		qWr() << "addQObject() called with nullptr, ignoring";
		return;
	}

	auto deletable_qobject = make_shared_DeletableBase(object,
			// Canceler
			                                           [](QObjectDerivedType* kjob) {},
			// Waiter
			                                           [](QObjectDerivedType* kjob) { /** @todo */ return true; },
			// External remover.
													   [pd, list, object](DeletableBase* deletable_base) {
			                                        	   // Is object in the list?
														   auto it = std::find_if(list->begin(), list->end(), [&](std::shared_ptr<DeletableBase> p){ return *p == *deletable_base; });
			                                        	   if(it != list->end())
			                                        	   {
			                                        		   // Remove the entry.
			                                        		   std::experimental::erase(*list, *it);
															   object->disconnect(pd);
			                                        	   }
			                                           });

	// Connect up the QObject's destroyed signal.
	connect_or_die(object, &QObject::destroyed, pd, [=, &the_mutex](QObject* object) mutable {
		std::scoped_lock lock(the_mutex);
		// Let's make sure deletable_qobject really contains object.
		auto typed_object_p = dynamic_cast<QObjectDerivedType*>(object);
		Q_ASSERT(deletable_qobject->holds_object(typed_object_p));
		// Remove this Deletable QObject's entry.
		auto before_size = list->size();
		qIn() << "Destroying QObject:" << object->objectName();
		std::experimental::erase_if(*list,
		                            [deletable_qobject](const auto& stored_deleter){ return stored_deleter == deletable_qobject; });
		auto after_size = list->size();
		if(before_size != after_size)
		{
			// We must have erased an entry in the deque.
			// Disconnect the signal that got us here.
			object->disconnect(pd);
			//			disconnect(object, &QObject::destroyed, this, objectDestroyed(QObject*)));
		}
	});

	// Add it to the watch list.
	list->emplace_back(deletable_qobject);
}


void PerfectDeleter::addQObject(QObject* object)
{
	std::scoped_lock lock(m_mutex);

	if(object == nullptr)
	{
		// Shouldn't have been called, warn.
		qWr() << "addQObject() called with nullptr, ignoring";
		return;
	}

	std::shared_ptr<DeletableQObject> deletable_qobject = std::make_shared<DeletableQObject>(this, &m_watched_QObjects, object);

	// This connection is a bit odd.  We need to accept the QObject*, turn that into a DeletableQObject*, and remove that.
	connect_or_die(object, &QObject::destroyed, this, [deletable_qobject](QObject* deleted_object){
		// Is this the same QObject that we put in?
		Q_ASSERT(deletable_qobject->holds_object(deleted_object) == true);
		// Erase the Deletable from the list.
		deletable_qobject->remove();
	});

	// Add it to the watch list.
	m_watched_QObjects.emplace_back(deletable_qobject);
}


void PerfectDeleter::addQFuture(QFuture<void> f)
{
	std::lock_guard lock(m_mutex);

	m_num_qfutures_added_since_last_purge++;
	m_total_num_added_qfutures++;

	m_future_synchronizer.addFuture(f);

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

	QPointer<KJob> qpkjob = kjob;

	auto remover_lambda = [=, qpkjob=qpkjob]() {
			std::lock_guard lock(m_mutex);
			qIn() << "Destroying KJob:" << qpkjob;
			if(qpkjob.isNull())
			{
				qWr() << "KJob was null";
			}
			else
			{
				std::experimental::erase(m_watched_KJobs, qpkjob.data());
            }
		};

	auto deletable = make_shared_DeletableBase(kjob,
			// Canceler
			                                   [qpkjob = qpkjob](KJob* kjob) {},
			// Waiter
			                                   [qpkjob = qpkjob](KJob* kjob) { return true; /** @todo */},
			// External remover.
			                                   [qpkjob = qpkjob](KJob* kjob) {}
	);

	// Connect a signal/slot to remove the Kjob* if it gets deleted.
	connect_or_die(kjob, &QObject::destroyed, this, remover_lambda);
//	connect_or_die(kjob, &KJob::finished, this, remover_lambda);

	m_watched_KJobs.push_back(kjob);
}

void PerfectDeleter::addAMLMJob(AMLMJob* amlmjob)
{
#if 1 // NEW
	std::lock_guard lock(m_mutex);

	std::shared_ptr<DeletableAMLMJob> deletable_amlmjob = std::make_shared<DeletableAMLMJob>(this, &m_watched_AMLMJobs, amlmjob);

	M_WARNING("These both want to remove the same amlmjob, maybe ok?");
	connect_or_die(amlmjob, &QObject::destroyed, this, [=](){
		std::scoped_lock lock(m_mutex);
		deletable_amlmjob->remove();
	});
/// @todo Should we hook up finished here?
//	connect_or_die(amlmjob, &AMLMJob::finished, this, remover_lambda);

	m_watched_AMLMJobs.emplace_back(deletable_amlmjob);


#else
	std::lock_guard lock(m_mutex);

	QPointer<AMLMJob> qpamlmjob = amlmjob;

	// Remover lambda and connections, for when the AMLMJob is deleted out from under us.
	// This is a bit of a mess due to this whole "let's delete ourself" thing that permeates Qt5/KF5.
	// Need to hook into two signals which don't really give us the info we need to un-crashably remove the
	// registered pointers from the storage here.

	auto remover_lambda = [=, qpamlmjob=qpamlmjob]() {
		std::lock_guard lock(m_mutex);
		if(qpamlmjob.isNull())
		{
			qWr() << "AMLMJob was null";
		}
		else
		{
			qIn() << "Destroying AMLMJob:" << qpamlmjob->objectName();
			std::experimental::erase(m_watched_AMLMJobs, qpamlmjob.data());
		}
	};

	auto deletable = make_shared_DeletableBase(amlmjob,
			// Canceler
			                                   [qpamlmjob = qpamlmjob](AMLMJob* amlmjob) {},
			// Waiter
			                                   [qpamlmjob = qpamlmjob](AMLMJob* amlmjob) {},
			// External remover.
			                                   [qpamlmjob = qpamlmjob](AMLMJob* amlmjob) {}
	);

M_WARNING("These both want to remove the same amlmjob, maybe ok?");
	connect_or_die(amlmjob, &QObject::destroyed, this, remover_lambda);
//	connect_or_die(amlmjob, &AMLMJob::finished, this, remover_lambda);

	m_watched_AMLMJobs.push_back(amlmjob);
#endif
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
			std::lock_guard lock(m_mutex);
			/// @todo Uniquify, don't rely on pointer.
			std::experimental::erase(m_watched_QThreads, qthread);
			qthread->deleteLater();
			});
		m_watched_QThreads.push_back(qthread);
	}
}


QStringList PerfectDeleter::stats() const
{
	/// @todo This should really be a Threadsafe Interface pattern.
	std::lock_guard lock(m_mutex);

	return stats_internal();
}

void PerfectDeleter::SLOT_DeletableBaseWasDestroyed(std::shared_ptr<DeletableBase> deletable_base)
{
	// This slot is invoked by a QObject deleted() signal.  All we have to do is remove the right one from the right list.
	deletable_base->remove(/*deletable_base.get()*/);
}

QStringList PerfectDeleter::stats_internal() const
{
	/// @note Acquires no mutex, see Threadsafe Interface stats().
	// Generate and return a stats object.
	QStringList retval;

	int num_futures = m_future_synchronizer.futures().size();

	retval << tr("Watched Deletables: %1").arg(m_watched_deletables.size());
	retval << tr("Watched QObjects: %1").arg(m_watched_QObjects.size());
	retval << tr("Watched QThreads: %1").arg(m_watched_QThreads.size());
	retval << tr("Watched QFuture<void>s: %1").arg(num_futures);
	retval << tr("Watched KJobs: %1").arg(m_watched_KJobs.size());
	retval << tr("Watched AMLMJobs: %1").arg(m_watched_AMLMJobs.size());

	// QFutureSynchronizer<> stats.
	retval << tr("Total added QFutures: %1").arg(m_total_num_added_qfutures);
	retval << tr("Number of QFutures added since last purge: %1").arg(m_num_qfutures_added_since_last_purge);

	return retval;
}

bool PerfectDeleter::waitForAMLMJobsFinished(bool spin)
{
M_TODO("This doesn't look like it's livelocking on exit, but it seems like we need to keep an event loop running.");
	long remaining_amlmjobs = 0;

	do
	{
		// One pass through the AMLMJob list.
		for(const std::shared_ptr<DeletableBase> i : m_watched_AMLMJobs)
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

	// Get a list of all futures we're currently watching.
	QList<QFuture<void>> futures = m_future_synchronizer.futures();
	auto num_starting_qfutures = futures.size();

	// Filter out the futures which report both canceled and finished.
	QList<QFuture<void>> unfinished_futures =
			QtConcurrent::blockingFiltered(futures, [](const QFuture<void>& f) -> bool {
		// Keep by returning true.
				return (!f.isCanceled() && !f.isFinished());
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
