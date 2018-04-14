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

#ifndef NETWORKAWAREFILEDIALOG_H
#define NETWORKAWAREFILEDIALOG_H

#include <QFileDialog>
#include <QUrl>
#include <QString>

#include <utility> // for std::pair<>.

/**
 * A sisyphean attempt to be all things to all people for file chooser dialogs.
 * The simplest things....
 */
class NetworkAwareFileDialog : public QWidget
{
	Q_OBJECT

	/// @note We do not inherit from QFileDialog here because:
	///       - Per an old post on SO: https://stackoverflow.com/a/2609618, only the static QFileDialog factory
	///         functions can use native dialogs.
	///       - Per http://doc.qt.io/qt-5/qfiledialog.html:
	///         "QFileDialog::DontUseNativeDialog  Don't use the native file dialog. By default, the native file dialog is used unless
	///          you use a subclass of QFileDialog that contains the Q_OBJECT macro, or the platform does not have a native dialog
	///          of the type that you require."
	using BASE_CLASS = QObject;

Q_SIGNALS:
	/// @note No signals yet.

public:
	explicit NetworkAwareFileDialog(QWidget *parent = Q_NULLPTR, const QString &caption = QString(), const QUrl &directory = QUrl(),
									const QString &filter = QString(), const QString &state_key = QString());
	~NetworkAwareFileDialog() override;


	static std::pair<QUrl, QString> getSaveFileUrl(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
												   const QUrl &dir = QUrl(),
												   const QString &filter = QString(),
												   const QString &state_key = QString(),
												   QFileDialog::Options options = QFileDialog::Options(),
												   const QStringList &supportedSchemes = QStringList());

	static std::pair<QUrl, QString> getExistingDirectoryUrl(QWidget *parent = Q_NULLPTR, const QString &caption = QString(),
															const QUrl &dir = QUrl(),
															const QString &state_key = QString(),
															QFileDialog::Options options = QFileDialog::ShowDirsOnly,
															const QStringList &supportedSchemes = QStringList());

	bool is_dlg_native();

public Q_SLOTS:
	virtual void onFilterSelected(const QString& filter);

	/// Workalike to QFileDialog's exec().
	virtual int exec();

	void onFinished(int result);

private:
	Q_DISABLE_COPY(NetworkAwareFileDialog)

	QString filter_to_suffix(const QString &filter);

	/**
	 * Native vs. Non-native file dialogs.
	 * (As of Qt 5.10, KF5 5.45)
	 *
	 * The situation:
	 * - Per the Internet and the scraps of documentation I've been able to find, QFileDialog() should
	 *   be the One True File Dialog Class on all platforms.  Anything platform specific should be getting
	 *   handled at the Qt Platform Abstraction layer (QPA, http://doc.qt.io/qt-5/qpa.html).  This is
	 *   partially true; if you only care about the local filesystem, QFileDialog seems to handle it.
	 *
	 * - Under Windows, as long as QFileDialog is using native file dialogs, again things seem to work as
	 *   expected, with the addition of a fully functional "Network Neighborhood" etc.  Remote files/dirs
	 *   seem to be handled seamlessly.
	 *
	 * - Gnome/gvfs/gio: If you're on Gnome and you want access to e.g. a Samba share under Gnome GVFS/GIO, which
	 *   in the real Gtk+ file dialog you can browse to and double click to automount under /run/user/.../gvfs/,
	 *   you're SOL: QFileDialog (native and non-native) will give you neither a "Network Neighborhood" or any
	 *   other means to do that.  The only QFileDIalog-based partial workaround is to add /run/user/.../gvfs/
	 *   to the sidebar URLs, and have the user use e.g. Nemo to automount a dir, then have him go
	 *   into your app's QFileDialog and select the dir/file under the sidebar "gvfs" path.
	 *   Yeah, pretty horrible.  Appears to be a deliberate decision at the QPA level.
	 *
	 * - KDE/KF5/KIO QPA under Gnome, QFileDialog using "native": The QPA will show you a "Network Neighborhood",
	 *   and let you browse it,
	 *   but as soon as you touch a Samba share, you get a segfault and your app is done.  This is
	 *   a bug in KIO and/or the KDE QPA.  The last debug message is something to the effect of:
	 *   "ERROR: QUrl("smb:///") found but QUrl("smb://") wasn't in the model, segfaulting!"
	 *   This bug appears to have been there since at least 2010, so don't hold your breath.
	 *   So yeah, worse than the Gnome situation.
	 *
	 */

    /// Returns the final decision on whether we should use the native file dialog or not.
	bool use_native_dlg() const;

    /// @todo Returns the user-settable preference on whether to use native dialogs or not.
	bool user_pref_native_file_dialog() const { return true; }

	bool isDirSelectDialog() const;
	void setDefaultSidebarUrls();

	int exec_();

	/// "Overload" because these have base class equivalents of otherwise the same name.
	void saveStateOverload();
	void restoreStateOverload();

	/// Persist the last state to/from this QSettings key.
	QString m_settings_state_key;

	QWidget *m_parent_widget;
	QSharedPointer<QFileDialog> m_the_qfiledialog;

};

#endif // NETWORKAWAREFILEDIALOG_H
