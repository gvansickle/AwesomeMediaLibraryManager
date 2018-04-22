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
#include <QFileDialog>

#define EX1 1
#define EX2 0

#if EX1 == 1

#include <KIO/ListJob>
#include <KWidgetJobTracker>

#include "MainWindow.h"
#include "expdialog.h"

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

#if 0

	QFileDialog* fd = new QFileDialog(this, "TEST - IS THIS NATIVE?", "file://home/gary");
	fd->setFileMode(QFileDialog::Directory);
	fd->setOptions(QFileDialog::ShowDirsOnly);
	fd->setFilter(QDir::AllDirs);
	fd->setAcceptMode(QFileDialog::AcceptOpen);


	qInfo() << "layout:" << fd->layout();

	fd->exec();

#endif

//	KUrlRequesterDialog::getUrl();

#if 1

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

    qDebug() << M_NAME_VAL(list_job);

//    KIO::getJobTracker()->registerJob(list_job);
    qDebug() << "REGISTERING LIST JOB";
    MainWindow::getInstance()->registerJob(list_job);
    qDebug() << "STARTING LIST JOB";
    list_job->start();

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
