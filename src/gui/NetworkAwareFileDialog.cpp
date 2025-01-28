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

#include <config.h>

#include "NetworkAwareFileDialog.h"

// Std C++
#include <memory>
#include <set>

// Qt5
#include <QApplication>
#include <QRegularExpression>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QWindow>

#if HAVE_GTKMM01 == 1
#include "helpers/NetAwareFileDialogGtk3.h"
#include <QtX11Extras/QX11Info>
//#include <glib-object.h>
#include <gtkmm.h>
//#include <gtkmm/filechooserdialog.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/x11/gdkx11window.h>
#endif // HAVE_GTKMM01 == 1


// Ours
#include <utils/in.h>
#include <utils/DebugHelpers.h>
#include <utils/ConnectHelpers.h>
#include "AMLMSettings.h"

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

/**
 * @brief NetworkAwareFileDialog::NetworkAwareFileDialog
 * @param parent
 * @param caption   This is set as the QDialog's windowTitle() via setWindowTitle().
 * @param directory
 * @param filter
 * @param state_key
 */
NetworkAwareFileDialog::NetworkAwareFileDialog(QWidget *parent, const QString& caption, const QUrl& directory,
											   const QString& filter, const QString& state_key)
    : QWidget(parent), m_parent_widget(parent)
{
	QString dir_as_str;
    setObjectName("NetAwareFileDialogProxy");

#if HAVE_GTKMM01 == 1
    m_xcb_connection = QX11Info::connection();
#endif // HAVE_GTKMM01 == 1

    // Create the underlying QFileDialog.
    m_the_qfiledialog = QSharedPointer<QFileDialog>::create(parent, caption, directory.toLocalFile(), filter);

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

//	setViewMode(QFileDialog::Detail);
	if(!use_native_dlg())
	{
		setOptions(QFileDialog::DontUseNativeDialog);
	}

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

    // We need to track the filter the user has selected (i.e. "Text files (*.txt)") via this signal.
    // For QFileDialog there doesn't appear to be any better way to get this information after the exec() call returns.
	connect_or_die(m_the_qfiledialog.data(), &QFileDialog::filterSelected, this, &NetworkAwareFileDialog::onFilterSelected);
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

    auto &dlg = nafdlg; //->m_the_qfiledialog;

	// User must select a directory.
	dlg->setFileMode(QFileDialog::Directory);
	if(options)
	{
		// Set options after doing setFileMode(), or we'll get a popup saying "Can't determine file extension".
		dlg->setOptions(options);
	}
	// List all directories and drives.  System is to allow KDE dialogs to see gvfs mounts.
	dlg->setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Drives | QDir::System);
	dlg->setAcceptMode(QFileDialog::AcceptOpen);
	if(!supportedSchemes.empty())
	{
		dlg->setSupportedSchemes(supportedSchemes);
	}

	if(!nafdlg->exec())
	{
		return {QUrl(), ""};
	}

    return std::make_pair(dlg->selectedUrls()[0], dlg->selectedNameFilter());
}

void NetworkAwareFileDialog::setSupportedSchemes(const QStringList &schemes)
{
    m_the_qfiledialog->setSupportedSchemes(schemes);
}

void NetworkAwareFileDialog::setOptions(QFileDialog::Options options)
{
    m_the_qfiledialog->setOptions(options);
}

QList<QUrl> NetworkAwareFileDialog::selectedUrls() const
{
M_WARNING("TODO");
    return m_the_qfiledialog->selectedUrls();
}

QString NetworkAwareFileDialog::selectedNameFilter() const
{
M_MESSAGE("TODO");
    return m_the_qfiledialog->selectedNameFilter();
}

void NetworkAwareFileDialog::setFilter(QDir::Filters filters)
{
    m_the_qfiledialog->setFilter(filters);
}

void NetworkAwareFileDialog::setFileMode(QFileDialog::FileMode mode)
{
    m_the_qfiledialog->setFileMode(mode);
}

void NetworkAwareFileDialog::setAcceptMode(QFileDialog::AcceptMode mode)
{
    m_the_qfiledialog->setAcceptMode(mode);
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

bool NetworkAwareFileDialog::use_qfiledialog() const
{
    // Get the user's preference.
    AMLMSettings::FileDialogMode user_pref_mode = AMLMSettings::fileDialogModeComboBox();

    if(user_pref_mode == AMLMSettings::FileDialogMode::QFDNative
            || user_pref_mode == AMLMSettings::FileDialogMode::QFDNonNative)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool NetworkAwareFileDialog::use_native_dlg() const
{
    if(user_pref_native_file_dialog() ||
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

bool NetworkAwareFileDialog::isDirSelectDialog() const
{
	return in(std::set<QFileDialog::FileMode>({QFileDialog::Directory, QFileDialog::DirectoryOnly}), m_the_qfiledialog->fileMode());
}

void NetworkAwareFileDialog::setDefaultSidebarUrls()
{
	// This doesn't appear to do anything on Windows when using the system file dialog.
    if(!use_native_dlg())
	{
		QList<QUrl> urls;
		urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)[0])
			<< QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0])
			/// @todo if linux && gvfs
			<< QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation)[0] + "/gvfs");
		for(const auto& url : std::as_const(urls))
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

    QDialog::DialogCode retval = QDialog::DialogCode::Rejected;

    if(use_qfiledialog())
    {
        retval = exec_qfiledialog();
    }

#if HAVE_GTKMM01 == 1
    else if(use_native_dlg() /* && GTK3+ available */)
    {
        retval = exec_gtk3plus();
    }
#endif // HAVE_GTKMM01 == 1

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



QDialog::DialogCode NetworkAwareFileDialog::exec_qfiledialog()
{
    setDefaultSidebarUrls();
    // On Windows at least, we don't have to do this for a native file dialog.
    if(!isDirSelectDialog() && !use_native_dlg())
    {
        QString snf = m_the_qfiledialog->selectedNameFilter();
        qDebug() << QString("Initial selected name filter:") << snf;
        m_the_qfiledialog->setDefaultSuffix(filter_to_suffix(m_the_qfiledialog->selectedNameFilter()));
    }

    // Force non-native dialog if that's what we want.
    if(!use_native_dlg())
    {
        m_the_qfiledialog->setOption(QFileDialog::DontUseNativeDialog, true);
    }

	qInfo() << "Using QFileDialog, is_dlg_native?:" << is_dlg_native();

    QDialog::DialogCode retval = static_cast<QDialog::DialogCode>(m_the_qfiledialog->exec());
    return retval;
}

#if HAVE_GTKMM01 == 1
static Gtk::FileChooserAction map_to_Gtk_FileChooserAction(QFileDialog::FileMode filemode)
{
    // See GTKMM: https://developer.gnome.org/gtkmm/stable/group__gtkmmEnums.html#ga0d6076e7637ec501f26296e65fee2212
    // and QFileDialog: http://doc.qt.io/qt-5/qfiledialog.html#QFileDialog-1

    // Two don't cleanly map:
    // Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER
    //    Indicates a mode for creating a new folder.
    //    The file chooser will let the user name an existing or new folder.
    // QFileDialog::ExistingFiles:
    //    "The names of zero or more existing files."

    switch(filemode)
    {
    case QFileDialog::AnyFile:
        // "The name of a file, whether it exists or not."
        // == "The file chooser will let the user pick an existing file, or type in a new filename. "
        return Gtk::FILE_CHOOSER_ACTION_SAVE;
    case QFileDialog::ExistingFile:
        // "The name of a single existing file."
        // == "The file chooser will only let the user pick an existing file."
        return Gtk::FILE_CHOOSER_ACTION_OPEN;
    case QFileDialog::Directory:
        // "The name of a directory. Both files and directories are displayed. However, the native Windows file dialog does not support displaying files in the directory chooser."
        // == "Indicates an Open mode for selecting folders.  The file chooser will let the user pick an existing folder."
        return Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER;
    case QFileDialog::ExistingFiles:
        // "The names of zero or more existing files."
        // == "???"
		return Gtk::FILE_CHOOSER_ACTION_OPEN; // For "control may reach end of nonvoid function".
        Q_ASSERT(0);
    default:
		return Gtk::FILE_CHOOSER_ACTION_OPEN; // For "control may reach end of nonvoid function".
        Q_ASSERT(0);
    }
}
#endif // HAVE_GTKMM01


#if HAVE_GTKMM01 == 1
QDialog::DialogCode NetworkAwareFileDialog::exec_gtk3plus()
{
    // Use the GTK File chooser.  This gives us access to the gvfs virtual folders and the network.

    QDialog::DialogCode retval = QDialog::DialogCode::Accepted;
#if 0

    // I think this is the equivalent of:
    // m_dlgHelper = static_cast<QPlatformFileDialogHelper*>(QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FileDialog));
    auto dialog = new NetAwareFileDialogGtk3();

//    dialog->setOptions(m_the_qfiledialog->options());
    dialog->setFilter(); // ApplyOptions
    dialog->exec();

#elif HAVE_GTKMM01
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

//    QWindow* parent_qwindow = this->windowHandle();
//    qDebug() << "parent_qwindow:" << parent_qwindow;
//    WId xcb_parent_window_id = this->winId();
//    qDebug() << "Parent WId:" << xcb_parent_window_id;

    // Create the Gtk file dialog exactly how we want it, bypassing the QPA.
    // Gtk::FileChooserDialog docs:
    // https://developer.gnome.org/gtkmm/stable/classGtk_1_1FileChooser.html
    // https://developer.gnome.org/gtkmm/stable/classGtk_1_1FileChooserDialog.html#adc98a1e747613c9b6cb66c238f6f8da6

    Gtk::FileChooserDialog dialog(toustring(m_the_qfiledialog->windowTitle()), map_to_Gtk_FileChooserAction(m_the_qfiledialog->fileMode()));

    dialog.set_local_only(false);

    dialog.set_current_folder("/home/grvs");
    dialog.set_action(Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    dialog.add_button(GTK_STOCK_OK, GTK_RESPONSE_OK);
    dialog.add_button(GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

//    dialog.show();
    auto wrappeddlg = new Gtk3DialogHelper(GTK_WIDGET(dialog.gobj()));

//    wrappeddlg->setParent(m_parent_widget->windowHandle());

//    dialog.set_transient_for(parent_qwindow->winId());
//    setTransientParent_xcb();

    if(false) /// @todo We're on gnome/xcb.
    {
        // On Linux/xcb, Qt5's WId is really an xcb_window_t.
        // See https://gist.github.com/torarnv/c5dfe2d2bc0c089910ce.

        // So what we want to do here is set the Gtk3 dialog's (transient?)parent to be our Qt5 parent window.
        // Per the link above, that means something like this:
        // nativeWindow = GetXcbWindowOf???(dialog);
        // qtWindow = this;
        // QWindow::fromWinId(nativeWindow)->setParent(qtWindow);


        QWindow* parent_qwindow = m_parent_widget->windowHandle();
        qDebug() << "parent_qwindow:" << parent_qwindow;


        Glib::RefPtr<Gdk::Window> dialog_gdkpp_win = dialog.get_window();
        qDebug() << "dialog_gdkpp_win:" << dialog_gdkpp_win.operator bool();
        if(dialog_gdkpp_win)
        {
            GdkWindow* dialog_gdk_win = dialog_gdkpp_win->gobj();

            qDebug() << "GDK_IS_X11_WINDOW(dialog_gdk_win):" << GDK_IS_X11_WINDOW(dialog_gdk_win);
            // Window == X11 window, ~= xcb_window_t == XCB window.
            Window xcb_dialog_window_id = GDK_WINDOW_XID(dialog_gdk_win);
            qDebug() << "xcb_dialog_window_id:" << xcb_dialog_window_id;
            m_xcb_file_dlg_win = xcb_dialog_window_id;

//            QWindow* file_dlg_qwindow_wrapper = QWindow::fromWinId(xcb_dialog_window_id);
    //		GdkWindow * gdk_parent_win = gdk_window_foreign_new(x11_parent_window_id);
    //        GdkDisplay* display = gdk_display_get_default();
//            qDebug() << "file_dlg_qwindow_wrapper:" << file_dlg_qwindow_wrapper;
    //        GdkWindow * gdk_parent_win = gdk_x11_window_lookup_for_display(display, xcb_parent_window_id);
    //        qDebug() << "gdk_parent_win:" << gdk_parent_win;

            if(xcb_dialog_window_id !=0 && parent_qwindow != nullptr)
            {
                qDebug() << "SETTING TRANSIENT PARENT" << xcb_dialog_window_id << parent_qwindow;
                QWindow* qw = QWindow::fromWinId(xcb_dialog_window_id);
                qDebug() << "qw:" << qw;
                qw->setTransientParent(parent_qwindow);
                qw->setFlags(Qt::Dialog);
                qw->dumpObjectInfo();
                qw->dumpObjectTree();
            }
        }

//        auto gtk_parent_win = Glib::wrap(gdk_parent_win);
//        qDebug() << "Parent WId as GdkWindow:" << gdk_parent_win;
//		dialog.set_transient_for(*gtk_parent_win.operator->());
//        dialog.set_parent_window(gtk_parent_win);
    }



    wrappeddlg->show(Qt::Dialog, Qt::ApplicationModal, m_parent_widget->windowHandle());
    wrappeddlg->exec();

//    int result = dialog.run();
    int result = 987;

    switch(result)
    {
    case Gtk::RESPONSE_OK:
    {
#warning "TODO: Get the string out"
        chosen_path = dialog.get_filename();
        qDebug() << "Diretory selected:" << chosen_path;
        retval = QDialog::DialogCode::Accepted;
        break;
    }
    case Gtk::RESPONSE_CANCEL:
    {
        qDebug() << "User cancelled";
        retval = QDialog::DialogCode::Rejected;
        break;
    }
    default:
    {
        qDebug() << "Unknown result:" << result;
        retval = QDialog::DialogCode::Rejected;
        break;
    }
    }

    return retval;
#else
    // Default to QFileDialog/Native if we ever get here.
    return exec_qfiledialog();
#endif // HAVE_GTKMM01
}
#endif // HAVE_GTKMM01 == 1

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

#if HAVE_GTKMM01
void NetworkAwareFileDialog::setTransientParent_xcb()
{
    QWindow* parent_qwindow = m_parent_widget->windowHandle();
    auto m_transientParent = parent_qwindow;

    qDebug() << M_NAME_VAL(m_transientParent);

	if(m_transientParent != nullptr)
    {
        xcb_window_t tp_xcb_win = m_transientParent->winId();
        xcb_void_cookie_t cookie = xcb_change_property_checked(m_xcb_connection, XCB_PROP_MODE_REPLACE,
                                                               m_xcb_file_dlg_win, XCB_ATOM_WM_TRANSIENT_FOR,
                                                               XCB_ATOM_WINDOW, 32, 1, &tp_xcb_win);
        xcb_request_check(m_xcb_connection, cookie);
        xcb_flush(m_xcb_connection);
    }
}
#endif // HAVE_GTKMM01

bool NetworkAwareFileDialog::user_pref_native_file_dialog() const
{
    // Get the user's preference.
    AMLMSettings::FileDialogMode user_pref_mode = AMLMSettings::fileDialogModeComboBox();

    if(user_pref_mode == AMLMSettings::FileDialogMode::QFDNative
            || user_pref_mode == AMLMSettings::FileDialogMode::GTK3Direct)
    {
        return true;
    }
    else
    {
        return false;
    }
}
