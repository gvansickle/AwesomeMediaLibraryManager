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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_CUMULATIVEAMLMJOB_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_CUMULATIVEAMLMJOB_H_

// Qt
#include <QMetaType>

// Ours
#include <ExtFuture.h>
#include <concurrency/AMLMJob.h>
#include <utils/TheSimplestThings.h>

class CumulativeAMLMJob;
using CumulativeAMLMJobPtr = QPointer<CumulativeAMLMJob>;

// Experiment to see if we can create a special-purpose "CumulativeKJob", so that we can treat
// the summary job/widget the same as the sub-jobs/widgets.
class CumulativeAMLMJob : public AMLMJob, public UniqueIDMixin<CumulativeAMLMJob>
{
    Q_OBJECT

    using BASE_CLASS = AMLMJob;

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<CumulativeAMLMJob>::uniqueQObjectName;

public:

    using ExtFutureType = ExtFuture<Unit>;

	explicit CumulativeAMLMJob(QObject* parent);
    ~CumulativeAMLMJob() override;

    static CumulativeAMLMJobPtr make_job(QObject *parent);

    /// Nothing to start, this is more of a placeholder. Or maybe?????
    Q_SCRIPTABLE void start() override {}

protected:

//    CumulativeAMLMJob* asDerivedTypePtr() override { return this; }

    void runFunctor() override {}

private:

    ExtFutureType m_ext_future;

};

Q_DECLARE_METATYPE(CumulativeAMLMJob);
Q_DECLARE_METATYPE(CumulativeAMLMJob*);


#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_CUMULATIVEAMLMJOB_H_ */
