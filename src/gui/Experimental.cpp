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
#include <logic/dbmodels/CollectionDatabaseModel.h>
#include <utils/DebugHelpers.h>

#include <QApplication>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QFileDialog>
#include <QDockWidget>
#include <QScrollArea>
#include <QStorageInfo>
#include <KDialogJobUiDelegate>

#define EX1 0
#define EX2 0

#if EX1 == 1

#include <KIO/Job>
#include <KIO/CopyJob>
#include <KIO/ListJob>
#include <KIO/DirectorySizeJob>
#include <KIO/JobUiDelegate>
#include <KJobUiDelegate>

#include <KMessageWidget>
#include <KUiServerJobTracker>
#include <KJobWidgets>

#include "MainWindow.h"

#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>
#include <concurrency/DirectoryScanJob.h>

#endif


enum /*__attribute__((enum_extensibility(closed), flag_enum))*/ BITS
{
    A = 0x01,
    B = 0x02
};

//static BITS a = A;
//static BITS b = B;
//static BITS c = a|b;
//static BITS d = c+1;


Experimental::Experimental(QWidget *parent) : QWidget(parent)
{
//	setAttribute(Qt::WA_NativeWindow);
}

void Experimental::DoExperiment()
{
	qDebug() << "Starting DoExperiment()";

    /// @todo Experiments
//    auto m_cdb_model = new CollectionDatabaseModel(this);
//    m_cdb_model->InitDb(QUrl("dummyfile.sqlite3"));
//    auto rel_table_model = m_cdb_model->make_reltable_model(this);
//    m_cdb_model->SLOT_addDirScanResult(QUrl("http://gbsfjdhg"));
//    m_cdb_model->SLOT_addDirScanResult(QUrl("http://the_next_one"));
//    /// @end

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

    auto master_job_tracker = MainWindow::master_tracker_instance();
    Q_CHECK_PTR(master_job_tracker);

    // Set the global KIO job tracker.
    // If we do this, the jobs will add themselves to the given tracker if they don't have HideProgressInfo set.
    KIO::setJobTracker(master_job_tracker);

//    QUrl dir_url("smb://storey.local/music/");
    QUrl dir_url("file:///run/user/1000/gvfs/smb-share:server=storey,share=music");
    KIO::DirectorySizeJob* dirsizejob = KIO::directorySize(dir_url);
    qDb() << "DirSizeJob:"
          << M_NAME_VAL(dirsizejob->detailedErrorStrings())
          << M_NAME_VAL(dirsizejob->capabilities());
    dirsizejob->setObjectName("DIRSIZEJOB");

    /// Two AMLMJobs
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(nullptr, dir_url,
                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QUrl dir_url2("file:///home/gary");
    DirectoryScannerAMLMJobPtr dsj2 = DirectoryScannerAMLMJob::make_job(nullptr, dir_url2,
                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    /// Another KF5 KIO Job.
    KIO::ListJob* kio_list_kiojob = KIO::listRecursive(dir_url, /*KIO::DefaultFlags*/ KIO::HideProgressInfo, /*includeHidden=*/false);
    connect_or_die(kio_list_kiojob, &KJob::result, this, [=](KJob* kjob){
        qIn() << "KIO::ListJob emitted result" << kjob;
//        AMLMJob::dump_job_info(kjob);
        ;});
//    connect_or_die(kio_list_kiojob, &KIO::ListJob::entries, this, [this](KIO::Job *job, const KIO::UDSEntryList &list){
//        static long num_entries = 0;
//        num_entries += list.size();
//        qDb() << "ENTRIES:" << num_entries;
//    });
    kio_list_kiojob->setObjectName("LISTRECURSIVEJOB");

    /// And one last KF5 KIO job.
    /// "emits the data through the data() signal."
    QUrl web_src_url(QStringLiteral("http://releases.ubuntu.com/18.04/ubuntu-18.04-desktop-amd64.iso?_ga=2.204957456.1400403342.1527338037-878124677.1491681087"));
//    QUrl local_dest_url(QStringLiteral("file://home/gary/testfile.html"));
    KIO::TransferJob* inet_get_job = KIO::get(web_src_url, KIO::LoadType::Reload, KIO::HideProgressInfo);
    inet_get_job->setObjectName("INET_GET_JOB");

    master_job_tracker->registerJob(dirsizejob);

    master_job_tracker->registerJob(dsj);
    master_job_tracker->setAutoDelete(dsj, true);
    master_job_tracker->setStopOnClose(dsj, false);

    master_job_tracker->registerJob(dsj2);
    master_job_tracker->setAutoDelete(dsj2, true);
    master_job_tracker->setStopOnClose(dsj2, false);

    master_job_tracker->registerJob(kio_list_kiojob);

    master_job_tracker->registerJob(inet_get_job);

    // Shows prog and other signals hooked up to the tracker.
//    dump_qobject(kio_list_kiojob);

//    auto test_job = inet_get_job;
//    KUiServerJobTracker *tracker3 = new KUiServerJobTracker(MainWindow::instance());
//    tracker3->registerJob(test_job);
//    KJobWidgets::setWindow(test_job, MainWindow::instance());
////    test_job->setUiDelegate(new KDialogJobUiDelegate());
//    test_job->setUiDelegate(new KIO::JobUiDelegate());
//    dump_qobject(inet_get_job);

    qDb() << M_NAME_VAL(dsj);
    qDb() << M_NAME_VAL(dsj2);

    // Start all the jobs.
    dirsizejob->start();
    dsj->start();
    dsj2->start();
    kio_list_kiojob->start();
    inet_get_job->start();

#endif

#endif

#if EX2 == 1
#endif
}

void Experimental::onDirEntries(KIO::Job *job, const KIO::UDSEntryList &list)
{
	for(auto& e : list)
    {
        qInfo() << "GOT ENTRY:" << e.stringValue( KIO::UDSEntry::UDS_NAME );
    }
}
