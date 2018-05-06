/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "Experimental.h"

#if EX2 == 1
#include <KConfigDialog>
#include <KConfigSkeleton>
#endif

#include <QDebug>
#include <utils/DebugHelpers.h>

#include <QApplication>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QFileDialog>
#include <QDockWidget>
#include <QScrollArea>
#include <QStorageInfo>

#define EX1 1
#define EX2 0

#if EX1 == 1

#include <KIO/Job>
#include <KIO/CopyJob>
#include <KIO/ListJob>
#include <KIO/DirectorySizeJob>
#include <KJobUiDelegate>

#include <KMessageWidget>

#include <ThreadWeaver/DebuggingAids>

#include "concurrency/ActivityManager.h"

#include "MainWindow.h"

#include "activityprogressmanager/ActivityProgressStatusBarWidget.h"
#include "activityprogressmanager/BaseActivityProgressWidget.h"
#include "activityprogressmanager/ActivityProgressWidget.h"
#include "activityprogressmanager/ActivityProgressDialog.h"
#include <concurrency/DirectoryScanJob.h>

#endif



Experimental::Experimental(QWidget *parent) : QWidget(parent)
{
//	setAttribute(Qt::WA_NativeWindow);
}

void Experimental::DoExperiment()
{
	qDebug() << "Starting DoExperiment()";

	/**
	 * @todo
	 * KFileCopyToMenu
	 *
	 */

#if EX1 == 1

//	auto dlg = new KFileCustomDialog();

//	QString filePath = QFileDialog::getOpenFileName( this, tr("Test") );

//	KEncodingFileDialog::getOpenFileNamesAndEncoding("file", QUrl(), "All (*)", this, "Experimental open file");

//	QUrl url = KDirSelectDialog::selectDirectory(QUrl("file://home/gary"), false, this, "KDE4 Dir Select Dialog");

#if 0
	QUrl filePath = QFileDialog::getExistingDirectoryUrl(this, tr("Test - SHOULD BE NATIVE - getExistingDirectoryUrl()"),
															QUrl("/home/gary"), // Start dir
															QFileDialog::ShowDirsOnly, // Options.
															QStringList()  //<< "gvfs" << "network" << "smb" << "file" << "mtp" << "http" // Supported Schemes.
															);
#endif

#if 1
    auto mwin = MainWindow::instance();
//    auto dock = new QDockWidget(tr("Test dock"), mwin);
//    auto stack_wdgt = new QWidget(dock);
//    stack_wdgt->setLayout(new QVBoxLayout(dock));
//    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//    dock->setWidget(stack_wdgt);
//    mwin->addDockWidget(Qt::LeftDockWidgetArea, dock);
//    auto kmsg_wdgt = new KMessageWidget(tr("KMessageWidget test"), stack_wdgt);
//    auto kmsg_wdgt2 = new KMessageWidget(tr("Second KMessageWidget test"), stack_wdgt);
//    stack_wdgt->layout()->addWidget(kmsg_wdgt);
//    stack_wdgt->layout()->addWidget(kmsg_wdgt2);

//    stack_wdgt->show();

//    kmsg_wdgt->animatedShow();
//    kmsg_wdgt2->animatedShow();



    ThreadWeaver::setDebugLevel(true, 10);

//    QUrl dir_url("smb://storey.local/music/");
    QUrl dir_url("file:///run/user/1000/gvfs/smb-share:server=storey.local,share=music");
    KIO::DirectorySizeJob* dirsizejob = KIO::directorySize(dir_url);
    qDb() << "DirSizeJob:"
          << M_NAME_VAL(dirsizejob->detailedErrorStrings())
          << M_NAME_VAL(dirsizejob->capabilities());
    connect(dirsizejob, &KIO::DirectorySizeJob::result, [=](KJob* kjob){
        if(kjob->error())
        {
            kjob->uiDelegate()->showErrorMessage();
        }
    });
    connect(dirsizejob, &KIO::DirectorySizeJob::description, [=](KJob *job,
            const QString &  	title,
            const QPair< QString, QString > &  	field1,
            const QPair< QString, QString > &  	field2){
        qIn() << "Title:" << title;});

    dirsizejob->start();


    AMLMJobPtr dsj = DirectoryScannerAMLMJob::make_shared(this, dir_url,
                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QUrl dir_url2("file:///home/gary");
    AMLMJobPtr dsj2 = DirectoryScannerAMLMJob::make_shared(this, dir_url2,
                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    qDb() << M_NAME_VAL(dsj->capabilities());
    qDb() << M_NAME_VAL(dsj2->capabilities());

    auto* queue = ThreadWeaver::Queue::instance(); //ThreadWeaver::stream();

//    ActivityManager::instance()->addActivity(dsj);//.dynamicCast<ThreadWeaver::JobInterface>());
//    ActivityManager::instance()->addActivity(dsj2);
//    MainWindow::getInstance()->m_activity_progress_widget->addActivity(dsj);

    /// @todo Does this transfer ownership/parentage?
//    MainWindow::instance()->registerJob(dirsizejob);
    MainWindow::instance()->registerJob(dsj);
    MainWindow::instance()->registerJob(dsj2);

    qIn() << "QUEUE STATE:" << queue->state()->stateName();

    qDb() << M_NAME_VAL(dsj);
    qDb() << M_NAME_VAL(dsj2);

    // enqueue takes JobPointers (QSharedPtr<>).
    queue->enqueue(dsj->asTWJobPointer());
    queue->enqueue(dsj2->asTWJobPointer());
//    queue << dsj << dsj2;
    qIn() << "QUEUE STATE:" << queue->state()->stateName();

#endif

//	KUrlRequesterDialog::getUrl();

#if 0

    // Get dir url.
//    QUrl dir_url = QFileDialog::getExistingDirectoryUrl(this, tr("EXPERIMENTAL - getExistingDirectoryUrl()"),
//                                                            QUrl("/home/gary"), // Start dir
//                                                            QFileDialog::ShowDirsOnly, // Options.
//                                                            QStringList()  //<< "gvfs" << "network" << "smb" << "file" << "mtp" << "http" // Supported Schemes.
//                                                            );
    QUrl dir_url("smb://storey.local/music/");

//    auto dlg = new ExpDialog(this);
//    dlg->show();

    // Try to use KIO to list the tree.
    KIO::ListJob* list_job = KIO::listRecursive(dir_url, KIO::JobFlag::DefaultFlags, false /*no hidden dirs*/);
    KIO::DirectorySizeJob* ds_job = KIO::directorySize(dir_url);
//    KIO::CopyJob* cp_job = KIO::copyAs(dir_url, QUrl("file://home/gary/deletme"));

    qDebug() << M_NAME_VAL(list_job);

    connect(list_job, &KIO::ListJob::entries, this, &Experimental::onDirEntries);

    qDebug() << "REGISTERING LIST JOB";
//    KIO::getJobTracker()->registerJob(list_job);

//    KIO::Job* total_job = new KIO::Job;
//    KCompositeJob* total_job = new KCompositeJob(this);
//    KIO::SimpleJob* total_job = new KIO::SimpleJob(this->parent());
//    ds_job->setParentJob(total_job);
//    list_job->setParentJob(ds_job);
    list_job->setObjectName("ListJob");


    MainWindow::getInstance()->registerJob(list_job);
    MainWindow::getInstance()->registerJob(ds_job);
//    MainWindow::getInstance()->registerJob(cp_job);

    qDebug() << "STARTING LIST JOB";
    list_job->start();
    qDebug() << "STARTING DS JOB";
    ds_job->start();
//    cp_job->start();

//    dlg->TrackJob(list_job);

//    dlg->exec();

//    KWidgetJobTracker* job_tracker_widget = new KWidgetJobTracker(this);
//    job_tracker_widget->registerJob(list_job);

//    connect(job, &KIO::ListJob::)

#endif

#endif

#if EX2 == 1
#endif
}

void Experimental::onDirEntries(KIO::Job *job, const KIO::UDSEntryList &list)
{
    for(auto e : list)
    {
        qInfo() << "GOT ENTRY:" << e.stringValue( KIO::UDSEntry::UDS_NAME );
    }
}
