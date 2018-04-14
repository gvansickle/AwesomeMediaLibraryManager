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

#include "NetworkAwareFileDialog.h"

#include <QApplication>
#include <QRegularExpression>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

#include <memory>
#include <set>

#include <utils/in.h>
#include <utils/DebugHelpers.h>

/**
 * @note Notes
 * - Per an old post on SO: https://stackoverflow.com/a/2609618, only the static QFileDialog factory functions use native dialogs.
 *   This was 7 years ago, not certain if that's still the case.  I can confirm that with KF5, the dialog you get from a static
 *   factory function doesn't seem to crash as much and looks slightly different.
 *
 * - Per http://doc.qt.io/qt-5/qfiledialog.html:
 *   "QFileDialog::DontUseNativeDialog  Don't use the native file dialog. By default, the native file dialog is used unless
 *	 you use a subclass of QFileDialog that contains the Q_OBJECT macro, or the platform does not have a native dialog
 *   of the type that you require."
 *
 * - Per https://stackoverflow.com/questions/42997657/pyqt5-filedialog-show-network-folders, "the gtk3 file dialog hides non-local files by default".
 *   Can confirm that both gtk3 and gtk2 themes show a file chooser with no network or VFS browsing support.
 *
 * - Stated nowhere, but sidebarUrls() only work with non-native dialogs.
 */

NetworkAwareFileDialog::NetworkAwareFileDialog(QWidget *parent, const QString& caption, const QUrl& directory,
											   const QString& filter, const QString& state_key)
    : QWidget(parent), m_parent_widget(parent), m_the_qfiledialog(new QFileDialog(parent, caption, directory.toLocalFile(), filter))
{
	QString dir_as_str;

	if(!directory.isEmpty())
	{
		qDebug() << "dir not empty";
		dir_as_str = directory.toString();
	}
	else
	{
		qDebug() << "dir empty, setting to \"\"";
		dir_as_str = "";
	}
#if 0
	setObjectName("nafiledialog");
	setViewMode(QFileDialog::Detail);
	if(!use_native_dlg())
	{
		setOptions(QFileDialog::DontUseNativeDialog);
	}
#else
	/**
	 *
		QFileDialog::DontUseNativeDialog
		0x00000010
		Don't use the native file dialog.
 By default, the native file dialog is used ***unless you use a subclass of QFileDialog that contains the Q_OBJECT macro***,
 or the platform does not have a native dialog of the type that you require.
	 */
	m_the_qfiledialog->setOptions(QFileDialog::DontUseNativeDialog);
#endif
	m_the_qfiledialog->setFileMode(QFileDialog::AnyFile);
	m_the_qfiledialog->setAcceptMode(QFileDialog::AcceptSave);
//	m_the_qfiledialog->setSupportedSchemes({"file", "smb", "gvfs"});
#if 0
	if(state_key.length() > 0)
	{
		// Persist the last state to/from this QSettings key.
		m_settings_state_key = "file_dialogs/" + state_key;
	}
#endif
	connect(m_the_qfiledialog.data(), &QFileDialog::filterSelected, this, &NetworkAwareFileDialog::onFilterSelected);
}

NetworkAwareFileDialog::~NetworkAwareFileDialog()
{

}

/**
 * Static member for creating a "Save File" dialog.
 */
std::pair<QUrl, QString> NetworkAwareFileDialog::getSaveFileUrl(QWidget* parent, const QString& caption, const QUrl& dir, const QString& filter,
																const QString& state_key, QFileDialog::Options options,
																const QStringList& supportedSchemes)
{
	std::unique_ptr<NetworkAwareFileDialog> nafdlg = std::make_unique<NetworkAwareFileDialog>(parent, caption, dir, filter, state_key);

	auto dlg = nafdlg->m_the_qfiledialog;

	options |= QFileDialog::DontUseNativeDialog;
	if(options)
	{
		dlg->setOptions(options);
	}

	dlg->setAcceptMode(QFileDialog::AcceptSave);
	if(!supportedSchemes.empty())
	{
		dlg->setSupportedSchemes(supportedSchemes);
	}

	qWarning() << "is_dlg_native:" << nafdlg->is_dlg_native();

	if(!nafdlg->exec())
	{
		return {QUrl(), ""};
	}

	return std::make_pair(dlg->selectedUrls()[0], dlg->selectedNameFilter());
}


/**
 * Static member for creating a "Open Existing Dir" dialog.
 */
std::pair<QUrl, QString> NetworkAwareFileDialog::getExistingDirectoryUrl(QWidget* parent, const QString& caption, const QUrl& dir,
																		 const QString& state_key,
																		 QFileDialog::Options options,
																		 const QStringList& supportedSchemes)
{
	std::unique_ptr<NetworkAwareFileDialog> nafdlg = std::make_unique<NetworkAwareFileDialog>(parent, caption, dir, QString(), state_key);

	auto dlg = nafdlg->m_the_qfiledialog;

	options |= QFileDialog::DontUseNativeDialog;
	if(options)
	{
		dlg->setOptions(options);
	}

	// User must select a directory.
	dlg->setFileMode(QFileDialog::Directory);
	// List all directories and drives.  System is to allow KDE dialogs to see gvfs mounts.
	dlg->setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Drives | QDir::System);
	dlg->setAcceptMode(QFileDialog::AcceptOpen);
	if(!supportedSchemes.empty())
	{
		dlg->setSupportedSchemes(supportedSchemes);
	}

	qWarning() << "is_dlg_native:" << nafdlg->is_dlg_native();

	if(!nafdlg->exec())
	{
		return {QUrl(), ""};
	}

	return std::make_pair(dlg->selectedUrls()[0], dlg->selectedNameFilter());
}

bool NetworkAwareFileDialog::is_dlg_native()
{
	// Check if the dialog is native by seeing if we can get a non-null layout.
	if(m_the_qfiledialog->layout() == nullptr)
	{
		return true;
	}
	else
	{
		return false;
	}
}

QString NetworkAwareFileDialog::filter_to_suffix(const QString &filter)
{
    qDebug() << "Filter:" << filter;
    QRegularExpression re(R"((\.([[:alnum:]_]*)))");
	QRegularExpressionMatch mo = re.match(filter);
	if(!mo.hasMatch())
	{
		QMessageBox::critical(m_parent_widget, QApplication::applicationDisplayName(),
							 "Can't determine file extension");
	}
	auto savefile_ext = mo.captured(1);
	qDebug() << "Filename extension:" << savefile_ext;
	return savefile_ext;
}

bool NetworkAwareFileDialog::isDirSelectDialog() const
{
	return in(std::set<QFileDialog::FileMode>({QFileDialog::Directory, QFileDialog::DirectoryOnly}), m_the_qfiledialog->fileMode());
}

void NetworkAwareFileDialog::setDefaultSidebarUrls()
{
	// This doesn't appear to do anything on Windows when using the system file dialog.
	if(true)//!use_native_dlg())
	{
		QList<QUrl> urls;
		urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0])
			<< QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0])
			/// @todo if linux && gvfs
			<< QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation)[0] + "/gvfs");
		for(auto url : urls)
		{
			qDebug() << "Adding Sidebar URL:" << url;
		}
		m_the_qfiledialog->setSidebarUrls(urls);
	}
}

void NetworkAwareFileDialog::onFilterSelected(const QString& filter)
{
	if(m_the_qfiledialog->fileMode() != QFileDialog::Directory && m_the_qfiledialog->fileMode() != QFileDialog::DirectoryOnly)
	{
		qDebug() << "Filter selected:" << filter;
		m_the_qfiledialog->setDefaultSuffix(filter_to_suffix(filter));
	}
}

int NetworkAwareFileDialog::exec()
{
	restoreStateOverload();

	int retval = exec_();

	if(retval && m_settings_state_key.length() > 0)
	{
		// Save the state for next time.
		saveStateOverload();
	}

	return retval;
}

void NetworkAwareFileDialog::onFinished(int result)
{
	auto sender = QObject::sender();
	qDebug() << "sender:" << sender;

	auto fd = qobject_cast<QFileDialog*>(sender);
	if(fd == nullptr)
	{
		qWarning() << "fd == nullptr";
	}
	else
	{
		qInfo() << "layout():" << fd->layout();
	}
}



int NetworkAwareFileDialog::exec_()
{
	if(true) //!use_gtk_dialog)
	{
		setDefaultSidebarUrls();
		// On Windows at least, we don't have to do this for a native file dialog.
		if(!isDirSelectDialog() && !use_native_dlg())
		{
			QString snf = m_the_qfiledialog->selectedNameFilter();
			qDebug() << QString("Initial selected name filter:") << snf;
			m_the_qfiledialog->setDefaultSuffix(filter_to_suffix(m_the_qfiledialog->selectedNameFilter()));
		}
		int retval = m_the_qfiledialog->exec();
		return retval;
	}
	else
	{
#if 0
		// Use the GTK File chooser.  This gives us access to the gvfs virtual folders and the network.
		self.gtk3_dlg = Gtk.FileChooserDialog("Please choose a folder", None,
									   Gtk.FileChooserAction.SELECT_FOLDER,
									   (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
										"Select", Gtk.ResponseType.OK))
		self.gtk3_dlg.set_local_only(False)
		response = self.gtk3_dlg.run()
		if response == Gtk.ResponseType.OK:
			print("Select clicked")
			print("Folder selected: " + self.gtk3_dlg.get_filename())
			response = QFileDialog.Accepted
		elif response == Gtk.ResponseType.CANCEL:
			print("Cancel clicked")
			response = QFileDialog.Rejected
		//self.gtk3_dlg.destroy()
		return response
#endif
		Q_ASSERT(0);
	}
}

void NetworkAwareFileDialog::saveStateOverload()
{
	QSettings settings;

	QByteArray new_state = m_the_qfiledialog->saveState();
	qDebug() << "Saving file dialog settings to settings key:" << m_settings_state_key;
	settings.setValue(m_settings_state_key + "/qfd_state", QVariant::fromValue(new_state));
	qDebug() << "Saving last dir URL: " << m_the_qfiledialog->directoryUrl();
	settings.setValue(m_settings_state_key + "/dir_url", QVariant::fromValue(m_the_qfiledialog->directoryUrl()));

	QString selected_name_filter = m_the_qfiledialog->selectedNameFilter();
	qDebug() << "Saving last selected_name_filter: " << selected_name_filter;
	settings.setValue(m_settings_state_key + "/name_filter", QVariant::fromValue(selected_name_filter));

	QString selected_mime_type_filter = m_the_qfiledialog->selectedMimeTypeFilter();
	qDebug() << "Saving last selected_mime_type_filter: " << selected_mime_type_filter;
	settings.setValue(m_settings_state_key + "/mime_type_filter", QVariant::fromValue(selected_mime_type_filter));

	// Detail or List view.
	QFileDialog::ViewMode view_mode  = m_the_qfiledialog->viewMode();
	qDebug() << "Saving last view_mode: " << view_mode;
	settings.setValue(m_settings_state_key + "/view_mode", QVariant::fromValue(view_mode));

	settings.sync();
	qDebug() << "Saved settings with error status:" << settings.status();
}

void NetworkAwareFileDialog::restoreStateOverload()
{
	QSettings settings;

	// Do we have a state_key key?
	if(m_settings_state_key.length() > 0)
	{
		bool state_restored = false;

		QByteArray saved_state = settings.value(m_settings_state_key + "/qfd_state").toByteArray();
		QUrl last_dir_url = settings.value(m_settings_state_key + "/dir_url").toUrl();
		QString selected_mime_type_filter = settings.value(m_settings_state_key + "/mime_type_filter").toString();
		QString selected_name_filter = settings.value(m_settings_state_key + "/name_filter").toString();
		if(saved_state.size() > 0)
		{
			state_restored = m_the_qfiledialog->restoreState(saved_state);

			/// @todo For reasons unknown, the above QFileDialog::restoreState() doesn't seem to work correctly,
			/// at least on Linux.
			/// At a minimum, it does not restore the selected directory; it behaves as though there's
			/// a single m_state_key that all instances are sharing (which isn't the case).
			/// So, we save/restore the directoryUrl manually.
			qDebug() << "Restoring last dir URL to:" << last_dir_url;
			m_the_qfiledialog->setDirectoryUrl(last_dir_url);
			qDebug() << "Restoring selected_name_filter:" << selected_name_filter;
			m_the_qfiledialog->selectNameFilter(selected_name_filter);
			// Note: MimeTypeFilters override NameFilters.
			qDebug() << "Restoring selected_mime_type_filter:" << selected_mime_type_filter;
			m_the_qfiledialog->selectMimeTypeFilter(selected_mime_type_filter);
		}

		if(state_restored == false)
		{
			qWarning() << "File dialog state failed to restore for m_state_key '" << m_settings_state_key << "'";
		}
		else
		{
			qDebug() << "File dialog state restored successfully for m_state_key '" << m_settings_state_key << "'";
			//qDebug() << "Dir is" << directory() << directoryUrl();
		}
	}
}

bool NetworkAwareFileDialog::use_native_dlg() const
{
M_WARNING("TODO");
	return false;

    if(/** @todo user_pref_native_file_dialog() | */
        ((QSysInfo::kernelType() == "winnt") && (QSysInfo::windowsVersion() & QSysInfo::WV_NT_based)) )
    {
        // Use the native file dialogs.
        return true;
    }
    else
    {
        return false;
    }
}
