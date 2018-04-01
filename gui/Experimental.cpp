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

#define EX1 1
#define EX2 0

#if EX1 == 1
//#include <KEncodingFileDialog>
//#include <KFileCustomDialog>
#include <KUrlRequesterDialog>

#include <gtkmm.h>
#include <gtkmm/filechooserdialog.h>
#endif

#if EX2 == 1
#include <KConfigDialog>
#include <KConfigSkeleton>
#endif

#include <QDebug>
#include <KIOWidgets/KUrlRequesterDialog>

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

#if EX1 == 1

//	auto dlg = new KFileCustomDialog();

//	QString filePath = QFileDialog::getOpenFileName( this, tr("Test") );

//	KEncodingFileDialog::getOpenFileNamesAndEncoding("file", QUrl(), "All (*)", this, "Experimental open file");

#if 0
	QUrl filePath = QFileDialog::getExistingDirectoryUrl(this, tr("Test - SHOULD BE NATIVE - getExistingDirectoryUrl()"),
															QUrl("/home/gary"), // Start dir
															QFileDialog::ShowDirsOnly, // Options.
															QStringList() << "smb" << "file" << "mtp" << "http" // Supported Schemes.
															);
#endif

//	KUrlRequesterDialog::getUrl();

/// GTKMM
#if 1
//	int argc = 1;
//	char *argv[] = {"myapp", 0};
	auto kit = Gtk::Application::create("myapp");

	Gtk::FileChooserDialog dialog("TEST - GTK FILECHOOSER", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_local_only(false);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("_Open", Gtk::RESPONSE_OK);
	kit->run(dialog);

#endif
#endif

#if EX2 == 1
#endif
}
