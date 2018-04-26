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

#ifndef SRC_CONCURRENCY_AMLMJOB_H_
#define SRC_CONCURRENCY_AMLMJOB_H_

#include <QObject>
#include <KJob>
#include <ThreadWeaver/IdDecorator>
#include <ThreadWeaver/QObjectDecorator>

/**
 * Class which bridges the hard-to-understand gap between a ThreadWeaver {QObject,Id}Decorator and a KIO::Job-derived class.
 * Note multiple inheritance in effect here.  Ok since IdDecoration doesn't inherit from QObject.
 */
class AMLMJob: public KJob, public ThreadWeaver::IdDecorator
{
    /// IdDecorator signals:
    /*
    *  // This signal is emitted when this job is being processed by a thread.
    *  void started(ThreadWeaver::JobPointer);
    *  // This signal is emitted when the job has been finished (no matter if it succeeded or not).
    *  void done(ThreadWeaver::JobPointer);
    *  // This signal is emitted when success() returns false after the job is executed.
    *  void failed(ThreadWeaver::JobPointer);
    */
public:
    AMLMJob(JobInterface *decoratee, bool autoDelete, QObject *parent = nullptr);
    ~AMLMJob();

    static AMLMJob* make_amlmjob(ThreadWeaver::IdDecorator* tw_job);

private:
    ThreadWeaver::QObjectDecorator* m_tw_job;
};

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
