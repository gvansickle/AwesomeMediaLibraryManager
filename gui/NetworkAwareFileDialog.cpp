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

NetworkAwareFileDialog::NetworkAwareFileDialog(QWidget *parent, const QString& caption, const QUrl& directory, const QString& filter, const QString& state_key)
	: QFileDialog(parent, caption, directory.toLocalFile(), filter)
{
	//assert(directory is None or isinstance(directory, QUrl))
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

	setObjectName("nafiledialog");
	setViewMode(QFileDialog::Detail);
	if(!use_native_dlg())
	{
		setOptions(QFileDialog::DontUseNativeDialog);
	}
	setFileMode(QFileDialog::AnyFile);
	setAcceptMode(QFileDialog::AcceptSave);
	setSupportedSchemes({"smb", "gvfs"});

	///gtk3_dlg = None

	if(state_key.length() > 0)
	{
		// Persist the last state to/from this QSettings key.
		m_settings_state_key = "file_dialogs/" + state_key;
	}

	connect(this, &QFileDialog::filterSelected, this, &NetworkAwareFileDialog::onFilterSelected);
}

/**
 * Static member for creating a "Save File" dialog.
 */
std::pair<QUrl, QString> NetworkAwareFileDialog::getSaveFileUrl(QWidget* parent, const QString& caption, const QUrl& dir, const QString& filter,
																const QString& state_key, QFileDialog::Options options,
																const QStringList& supportedSchemes)
{
	std::unique_ptr<NetworkAwareFileDialog> dlg = std::make_unique<NetworkAwareFileDialog>(parent, caption, dir, filter, state_key);

	if(options)
	{
		dlg->setOptions(options);
	}
	dlg->setAcceptMode(QFileDialog::AcceptSave);
	if(!supportedSchemes.empty())
	{
		dlg->setSupportedSchemes(supportedSchemes);
	}

	if(!dlg->exec())
	{
		return {QUrl(), ""};
	}

	return std::make_pair(dlg->selectedUrls()[0], dlg->selectedNameFilter());
}


/**
 * Static member for creating a "Open Existing Dir" dialog.
 */
std::pair<QUrl, QString> NetworkAwareFileDialog::getExistingDirectoryUrl(QWidget* parent, const QString& caption, const QUrl& dir, const QString& state_key,
																		 QFileDialog::Options options, const QStringList& supportedSchemes)
{
	std::unique_ptr<NetworkAwareFileDialog> dlg = std::make_unique<NetworkAwareFileDialog>(parent, caption, dir, QString(), state_key);
	if(options)
	{
		dlg->setOptions(options);
	}
	dlg->setFileMode(QFileDialog::Directory);
	dlg->setAcceptMode(QFileDialog::AcceptOpen);
	if(!supportedSchemes.empty())
	{
		dlg->setSupportedSchemes(supportedSchemes);
	}

	if(!dlg->exec())
	{
		return {QUrl(), ""};
	}

	return std::make_pair(dlg->selectedUrls()[0], dlg->selectedNameFilter());
}

QString NetworkAwareFileDialog::filter_to_suffix(const QString &filter)
{
    qDebug() << "Filter:" << filter;
    QRegularExpression re(R"((\.([[:alnum:]_]*)))");
	QRegularExpressionMatch mo = re.match(filter);
	if(!mo.hasMatch())
	{
		QMessageBox::critical(this, QApplication::applicationDisplayName(),
							 "Can't determine file extension");
	}
	auto savefile_ext = mo.captured(1);
	qDebug() << "Filename extension:" << savefile_ext;
	return savefile_ext;
}

bool NetworkAwareFileDialog::isDirSelectDialog() const
{
	return in(std::set<FileMode>({QFileDialog::Directory, QFileDialog::DirectoryOnly}), fileMode());
}

void NetworkAwareFileDialog::setDefaultSidebarUrls()
{
	// This doesn't appear to do anything on Windows when using the system file dialog.
	if(!use_native_dlg())
	{
		QList<QUrl> urls =
		{
			QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0]),
			QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]),
			/// @todo if linux
			QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation)[0] + "/gvfs")
		};
		for(auto url : urls)
		{
			qDebug() << "Adding Sidebar URL:" << url;
		}
		setSidebarUrls(urls);
	}
}

void NetworkAwareFileDialog::onFilterSelected(const QString& filter)
{
	if(fileMode() != QFileDialog::Directory && fileMode() != QFileDialog::DirectoryOnly)
	{
		qDebug() << "Filter selected:" << filter;
		setDefaultSuffix(filter_to_suffix(filter));
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


int NetworkAwareFileDialog::exec_()
{
	if(true) //!use_gtk_dialog)
	{
		setDefaultSidebarUrls();
		// On Windows at least, we don't have to do this for a native file dialog.
		if(!isDirSelectDialog() && !use_native_dlg())
		{
			QString snf = selectedNameFilter();
			qDebug() << QString("Initial selected name filter:") << snf;
			setDefaultSuffix(filter_to_suffix(selectedNameFilter()));
		}
		int retval = QFileDialog::exec();
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

	QByteArray new_state = saveState();
	qDebug() << "Saving file dialog settings to settings key:" << m_settings_state_key;
	settings.setValue(m_settings_state_key + "/qfd_state", QVariant::fromValue(new_state));
	qDebug() << "Saving last dir URL: " << directoryUrl();
	settings.setValue(m_settings_state_key + "/dir_url", QVariant::fromValue(directoryUrl()));

	QString selected_mime_type_filter = selectedMimeTypeFilter();
	qDebug() << "Saving last selected_mime_type_filter: " << selected_mime_type_filter;
	settings.setValue(m_settings_state_key + "/mime_type_filter", QVariant::fromValue(selected_mime_type_filter));

	QString selected_name_filter = selectedNameFilter();
	qDebug() << "Saving last selected_name_filter: " << selected_name_filter;
	settings.setValue(m_settings_state_key + "/name_filter", QVariant::fromValue(selected_name_filter));

	// Detail or List view.
	QFileDialog::ViewMode view_mode  = viewMode();
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
			state_restored = restoreState(saved_state);

			/// @todo For reasons unknown, the above QFileDialog::restoreState() doesn't seem to work correctly,
			/// at least on Linux.
			/// At a minimum, it does not restore the selected directory; it behaves as though there's
			/// a single m_state_key that all instances are sharing (which isn't the case).
			/// So, we save/restore the directoryUrl manually.
			qDebug() << "Restoring last dir URL to:" << last_dir_url;
			setDirectoryUrl(last_dir_url);
			qDebug() << "Restoring selected_mime_type_filter:" << selected_mime_type_filter;
			selectMimeTypeFilter(selected_mime_type_filter);
			qDebug() << "Restoring selected_name_filter:" << selected_name_filter;
			selectNameFilter(selected_name_filter);
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
