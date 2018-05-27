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

#ifndef SRC_GUI_ACTIVITYPROGRESSMANAGER_AMLMPROGRESSSTATUSSTRUCT_H_
#define SRC_GUI_ACTIVITYPROGRESSMANAGER_AMLMPROGRESSSTATUSSTRUCT_H_

#include <QObject>

/**
 * Structure used by Trackers to hold the actual KJob-associated tracked information.
 * KWidgetJobTracker puts this kind of info in the KJob's associated widget, which seems wrong.
 * KJobPrivate also has a number of private members we could use directly, if we could get at them.
 * So far I haven't figured out a reasonable way to do so.  KJob itself deos this in kjob.cpp:
 *
 * #include "kjob.h"
 * #include "kjob_p.h"
 *
 * As installed, /usr/include/KF5/KCoreAddons contains kjob.h but not kjob_p.h.
 * Comments seem to indicate that they're intended to be used only by other KF5 classes, not the general public.
 *
 * They do appear to all be exposed one way or another through the KJob public or protected interfaces, or as proprties.
 * So there may not be a real reason to try to get directly at them.
 */
class AMLMProgressStatusStruct: public QObject
{
    Q_OBJECT

public:
    AMLMProgressStatusStruct(QObject* parent = nullptr);
	virtual ~AMLMProgressStatusStruct();

    /**
     * KJobPrivate members:
     *
     *  KJob *q_ptr;
        KJobUiDelegate *uiDelegate;
        QString errorText;
        int error;
        KJob::Unit progressUnit;
        QMap<KJob::Unit, qulonglong> processedAmount;
        QMap<KJob::Unit, qulonglong> totalAmount;
        unsigned long percentage;
        QTimer *speedTimer;
        QEventLoop *eventLoop;
        // eventLoopLocker prevents QCoreApplication from exiting when the last
        // window is closed until the job has finished running
        QEventLoopLocker eventLoopLocker;
        KJob::Capabilities capabilities;
        bool suspended;
        bool isAutoDelete;

        void _k_speedTimeout();

        bool isFinished;

     *
     */
};

#endif /* SRC_GUI_ACTIVITYPROGRESSMANAGER_AMLMPROGRESSSTATUSSTRUCT_H_ */
