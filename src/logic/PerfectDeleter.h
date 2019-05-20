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

#ifndef PERFECTDELETER_H
#define PERFECTDELETER_H

// Std C++
#include <deque>
#include <mutex>
#include <memory>
#include <functional>

// Future Std C++
#include <future/future_algorithms.h> ///< For Uniform Container Erasure.

// Qt5
#include <QObject>
#include <QStringList>
#include <QFuture>
#include <QFutureSynchronizer>
#include <QObjectCleanupHandler>
#include <QUuid>

// KF5
#include <KJob>
class AMLMJob;

// Ours
#include <utils/DebugHelpers.h>


class PerfectDeleter;

class DeletableBase
{
public:
	DeletableBase() = default;
	explicit DeletableBase(PerfectDeleter* pd) : m_uuid(QUuid::createUuid()), m_pd(pd) {};
	virtual ~DeletableBase() = default;

	virtual void cancel() = 0;
	virtual bool poll_wait() = 0;
	virtual void deleted_externally(DeletableBase*) = 0;

	bool operator==(const DeletableBase& other) const
	{
		return (m_uuid == other.m_uuid) && this->equality_op(other);
	}

protected:
	/// The bottom half of derived classes' equality comparison operator.
	/// Override and call this in derived classes.
	virtual bool equality_op(const DeletableBase& other) const = 0;

	// The owning PerfectDeleter.
	PerfectDeleter* m_pd {nullptr};

private:
	/// A UUID representing the instance of this Deletable, so we can avoid the ABA problem
	/// of deleting the wrong object if we relied solely on the pointer value.
	QUuid m_uuid;
};

template <class T, class CancelerType, class WaiterType, class DeletedExternallyCBType>
class Deletable : public DeletableBase
{
	static_assert(std::is_invocable_v<CancelerType, T>);
	static_assert(std::is_invocable_v<WaiterType, T>);

	using BASE_CLASS = DeletableBase;
	using THIS_CLASS = Deletable<T,CancelerType,WaiterType,DeletedExternallyCBType>;

public:
	Deletable(T to_be_deleted, CancelerType canceler, WaiterType waiter, DeletedExternallyCBType deleted_externally_callback)
		: m_to_be_deleted(to_be_deleted), m_canceler(canceler), m_waiter(waiter), m_deleted_externally_callback(deleted_externally_callback) {};
	~Deletable() override {};

	void cancel() override { std::invoke(m_canceler, m_to_be_deleted); };
	bool poll_wait() override { return std::invoke(m_waiter, m_to_be_deleted); };
	void deleted_externally(DeletableBase*) override
	{
		//std::invoke(m_deleted_externally_callback, m_to_be_deleted);
	};

	bool holds_object(const T object) { return m_to_be_deleted == object; }

protected:

	bool equality_op(const DeletableBase& other) const override
	{
		THIS_CLASS const * other_p = dynamic_cast<const THIS_CLASS*>(&other);
		if(other_p == nullptr)
		{
			// Not a comparable type.
			return false;
		}
		return m_to_be_deleted == other_p->m_to_be_deleted;
	}
public:
	T m_to_be_deleted;
	CancelerType m_canceler;
	WaiterType m_waiter;
	DeletedExternallyCBType m_deleted_externally_callback;
};

class DeletableQObject;

inline static auto passthrough = [](AMLMJob*){ return true; };
using DeletableAMLMJob = Deletable<AMLMJob*, decltype(passthrough), decltype(passthrough), decltype(passthrough)>;

template <class T, class CancelerType, class WaiterType, class DeletedExternallyCBType = std::nullptr_t>
static inline std::shared_ptr</*DeletableBase*/Deletable<T, CancelerType, WaiterType, DeletedExternallyCBType>>
make_shared_DeletableBase(T to_be_deleted,
                           CancelerType canceler,
                           WaiterType waiter,
                           DeletedExternallyCBType deleted_externally)
{
	auto deletable = std::make_shared<Deletable<T, CancelerType, WaiterType, DeletedExternallyCBType>>(to_be_deleted,
                                                                                                           canceler,
                                                                                                           waiter,
                                                                                                           deleted_externally);
	return deletable;
}


/**
 * Class for managing the lifecycle of various deferred-delete or self-deleting objects.
 */
class PerfectDeleter : public QObject
{
	Q_OBJECT

public:
    /**
     * Default constructor
     */
	explicit PerfectDeleter(QObject* parent);

    /**
     * Destructor
     */
	~PerfectDeleter() override;

	/**
	 * Singleton accessor.
	 * @param parent  Must be specified once by the first caller (usually the QApplication-derived singleton).
	 */
	static PerfectDeleter& instance(QObject* parent = nullptr);


	void cancel_and_wait_for_all();

	bool empty() const;
	size_t size() const;

	void addQObject(QObject* object);

	/**
	 * For adding QFuture<void>'s to be canceled/waited on.
	 * @warning This is kind of wrong.  Ext/QFutures are ~std::shared_future<>'s, so while we can tell the promise-side
	 * status (i.e. isFinished() etc.), there's not a public way of determining when the future is no longer being
	 * looked at by consumers, and hence can be destroyed.  Since it's a value-type, it may not even make sense to try
	 * to track finished-ness, but we should have a way to do a global cancel() of all outstanding futures.
	 */
	void addQFuture(QFuture<void> f);

    void addKJob(KJob* kjob);

    void addAMLMJob(AMLMJob* amlmjob);

	void addQThread(QThread* qthread);

	template <class DestroyerCallbackType>
	void addQThread(QThread* qthread, DestroyerCallbackType&& destroyer_cb)
	{
		/*std::shared_ptr<Deletable>*/
//		auto deletable = std::make_shared<Deletable<QThread*,DestroyerCallbackType,void(*)(QThread*)>>(qthread, destroyer_cb, [](QThread*){});
		auto deletable = make_shared_DeletableBase(qthread, destroyer_cb, [](QThread*) {/** @todo */ return true;}, [](QThread*){  });
		m_watched_deletables.push_back(deletable);
	}

	/// The Threadsafe Interface for stats_internal().
	QStringList stats() const;

public Q_SLOTS:
	void SLOT_DeletableBaseWasDestroyed(std::shared_ptr<DeletableBase> deletable_base);

protected:
	/// No mutex.
	QStringList stats_internal() const;

private:

	/// Private member functions.

	bool waitForAMLMJobsFinished(bool spin);

	/// Private helper for clearing out completed futures.
	void scan_and_purge_futures();

	/// @name Private data members
	/// @{

	// Mutex for synchronizing state, e.g. watch lists below.
	mutable std::mutex m_mutex;

	/// QFutureSynchronizer<void> for watching/canceling all QFuture<>s.
	QFutureSynchronizer<void> m_future_synchronizer;
	long m_total_num_added_qfutures {0};
	long m_num_qfutures_added_since_last_purge {0};
	/// When the number of submitted futures exceeds this value,
	/// run a GC sweep and remove any canceled/finished futures.
	const long m_purge_futures_count {64};

	std::deque<std::shared_ptr<DeletableQObject>> m_watched_QObjects;
    std::deque<QPointer<KJob>> m_watched_KJobs;
//    std::deque<QPointer<AMLMJob>> m_watched_AMLMJobs;
//	std::deque<std::shared_ptr<DeletableAMLMJob>> m_watched_AMLMJobs;
	std::deque<std::shared_ptr<DeletableBase>> m_watched_AMLMJobs;
	std::deque<QPointer<QThread>> m_watched_QThreads;
	std::deque<std::shared_ptr<DeletableBase>> m_watched_deletables;
	QObjectCleanupHandler m_qobj_cleanup_handler;

	/// @}

};

#endif // PERFECTDELETER_H
