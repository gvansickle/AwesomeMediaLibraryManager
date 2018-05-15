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

#ifndef SRC_CONCURRENCY_AMLMCOMPOSITEJOB_H_
#define SRC_CONCURRENCY_AMLMCOMPOSITEJOB_H_

/// Qt5
#include <QObject>
#include <QPointer>

#include <KJob>
#include <KCompositeJob>
#include <ThreadWeaver/Collection>

#include "AMLMJob.h"

/// Use the AMLMCompositeJobPtr alias to pass around refs to AMLMJob-derived jobs.
class AMLMCompositeJob;
using AMLMCompositeJobPtr = QPointer<AMLMCompositeJob>;

/*
 *
 */
class AMLMCompositeJob: /*public AMLMJob,*/ public KCompositeJob, public ThreadWeaver::Collection, public QEnableSharedFromThis<AMLMCompositeJob>
{
    Q_OBJECT

public:
    AMLMCompositeJob(QObject* parent);
     ~AMLMCompositeJob() override;

    Q_SCRIPTABLE void start() override {};
};

#endif /* SRC_CONCURRENCY_AMLMCOMPOSITEJOB_H_ */
