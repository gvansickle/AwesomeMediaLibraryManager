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

#define EX1 0
#define EX2 1

#if EX1 == 1
#include <KEncodingFileDialog>
//#include <KFileCustomDialog>
#endif

#if EX2 == 1
#include <KConfigDialog>
#include <KConfigSkeleton>
#endif

#include <QDebug>

Experimental::Experimental(QWidget *parent) : QWidget(parent)
{

}

void Experimental::DoExperiment()
{
	qDebug() << "Starting DoExperiment()";

	/**
	 * @todo
	 * KFileCopyToMenu
	 *
	 */

#if 0

	KEncodingFileDialog::getOpenFileNamesAndEncoding("file", QUrl(), "All (*)", this, "Experimental open file");
#endif

#if 1
	//"QT += KConfigCore KConfigGui"

	if(KConfigDialog::showDialog("settings"))
	  return;
	KConfigDialog *dialog = new KConfigDialog(this, "settings", MySettings::self());
	dialog->setFaceType(KPageDialog::List);
	dialog->addPage(new General(0, "General"), i18n("General") );
	dialog->addPage(new Appearance(0, "Style"), i18n("Appearance") );
	connect(dialog, SIGNAL(settingsChanged(const QString&)), mainWidget, SLOT(loadSettings()));
	connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));
	dialog->show();
#endif
}
