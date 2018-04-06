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
//#include <KUrlRequesterDialog>
//#include <KBuildSycocaProgressDialog>

#include <gtkmm.h>
#include <gtkmm/filechooserdialog.h>
#endif

#if EX2 == 1
#include <KConfigDialog>
#include <KConfigSkeleton>
#endif

#include <QDebug>
#include <utils/DebugHelpers.h>

#include <QApplication>
#include <QFileDialog>

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

/// GTKMM
#if 1
//	int argc = 1;
//	char *argv[] = {"myapp", 0};

	// Per https://developer.gnome.org/gtkmm/stable/classGtk_1_1Main.html#details
	// "Deprecated:	Use Gtk::Application instead."
	///Gtk::Main kit;

	// Per https://developer.gnome.org/gtkmm/stable/classGtk_1_1Application.html#details
	// "the application ID must be valid. See g_application_id_is_valid()."
	// https://developer.gnome.org/gio/stable/GApplication.html#g-application-id-is-valid
	// GIO::ApplicationFlags: https://developer.gnome.org/gio/stable/GApplication.html#GApplicationFlags
	//    Not clear that any of the flags are applicable.
	auto app = Gtk::Application::create(tostdstr(QApplication::desktopFileName()).c_str());
	std::string chosen_path;

	Gtk::FileChooserDialog dialog("TEST - GTK FILECHOOSER", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	//dialog.set_transient_for(kit);
	dialog.set_local_only(false);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("_Open", Gtk::RESPONSE_OK);
	int result = dialog.run();
//	int result = app->run(dialog);

	switch(result)
	{
	case Gtk::RESPONSE_OK:
	{
		chosen_path = dialog.get_filename();
		qDebug() << "Diretory selected:" << chosen_path;
		break;
	}
	case Gtk::RESPONSE_CANCEL:
	{
		qDebug() << "User cancelled";
		break;
	}
	default:
	{
		qDebug() << "Unknown result:" << result;
		break;
	}
	}

#endif

#endif

#if EX2 == 1
#endif
}
