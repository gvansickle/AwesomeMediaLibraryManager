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

// Qt5
#include <QObject>
#include <QFuture>

// KF5
#include <KJob>

// Ours
#include <src/concurrency/AMLMJob.h>

struct Deletable
{
//	template <class T, class Canceler, class Waiter>
	QVariant m_to_be_deleted;
};

/**
 * Class for managing the lifecycle of various deferred-delete objects.
 */
class PerfectDeleter : public QObject
{
public:
    /**
     * Default constructor
     */
	explicit PerfectDeleter(QObject* parent);

    /**
     * Destructor
     */
    ~PerfectDeleter();

	/// Returns a pointer to the singleton.
    static PerfectDeleter* instance();
    static void destroy();

	void cancel_and_wait_for_all();

	/**
	 * For adding QFuture<void>'s to be canceled/waited on.
	 */
    void addQFuture(QFuture<void> f);

    void addKJob(KJob* kjob);

    void addAMLMJob(AMLMJob* amlmjob);

private:

	/// The singleton.
    static PerfectDeleter* s_instance;

	// Mutex for synchronizing state, e.g. watch lists below.
    std::mutex m_mutex;

    std::deque<QFuture<void>> m_watched_qfutures;

    std::deque<QPointer<KJob>> m_watched_KJobs;
    std::deque<QPointer<AMLMJob>> m_watched_AMLMJobs;

    /// Private member functions.


};

#endif // PERFECTDELETER_H
