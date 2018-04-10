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
 * A sysyphean attempt to be all things to all people for file chooser dialogs.
 * The simplest things....
 */
class NetworkAwareFileDialog : public QObject
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

    // Returns the final decision on whether we should use the native file dialog or not.
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
