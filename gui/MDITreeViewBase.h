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

#ifndef MDITREEVIEWBASE_H
#define MDITREEVIEWBASE_H

#include <QContextMenuEvent>
#include <QTreeView>
#include <QUrl>

class QMdiSubWindow;
class QFileDevice;

class MDITreeViewBase : public QTreeView
{
	Q_OBJECT

public:
	MDITreeViewBase(QWidget *parent = Q_NULLPTR);

	///
	/// Public interface.
	///

	void newFile();

	virtual bool loadFile(QUrl load_url);
	bool save();
	bool saveAs();
	bool saveFile(QUrl save_url, QString filter);

	QString userFriendlyCurrentFile() const;
	QUrl getCurrentUrl() const;

protected:

	QUrl m_current_url;
	QString m_current_filter;
	bool m_isUntitled = true;

	/// Protected function which is used to set the view's filename properties on a save or load.
	void setCurrentFile(QUrl url);

	virtual void closeEvent(QCloseEvent* event) override;

	///
	/// Pure virtual functions.
	///

	/// Return a template for use in naming instances of this class when doing a "File->New".
	/// E.g. "document%1.txt".  The %1 will be filled in with a unique identifier.
	virtual QString getNewFilenameTemplate() const = 0;

	/// Override to return a string usable by QFileDialog in setNameFilter().
	virtual QString defaultNameFilter() = 0;

	/// Override to return whether or not the underlying data has been modified.
	virtual bool isModified() const = 0;

	virtual void serializeDocument(QFileDevice& file) const = 0;
	virtual void deserializeDocument(QFileDevice& file) = 0;

	///
	/// Not pure-virtual, but designed-to-be-overridden functions.
	///

protected slots:
	virtual void documentWasModified();

	virtual void headerMenu(QPoint pos);

	virtual void onSectionClicked(int logicalIndex);

protected:

	virtual bool viewportEvent(QEvent *event) override;

	/// Return a string suitable for use as a key in the QSettings file.  Used
	/// to save and restore the state of the "Save As" dialog.
	/// Default is none, settings won't be saved.
	virtual QString getSaveAsDialogKey() const { return QString(); }

	/// Return True if you handle it, False if you don't.
	virtual bool onBlankAreaToolTip(QHelpEvent* event);

	virtual bool maybeSave();

	/**
	 * Returns the QMdiSubwindow instance holding this MDITreeViewBase-derived instance.
	 */
	QMdiSubWindow* getQMdiSubWindow() const;

	/// Helper function to convert from incoming proxy QModelIndexes to actual underlying model indexes.
	virtual QModelIndex to_underlying_qmodelindex(const QModelIndex &proxy_index) = 0;

	/// Helper function to convert from underlying model indexes to proxy QModelIndexes.
	virtual QModelIndex from_underlying_qmodelindex(const QModelIndex& underlying_index) = 0;

private:
	Q_DISABLE_COPY(MDITreeViewBase)

	/// The column which we last saw was being sorted.
	/// This is used to enable tri-state column sort functionality.
	int m_previous_sort_column {-1};

	Qt::SortOrder m_sort_order { Qt::AscendingOrder };

};

#endif // MDITREEVIEWBASE_H
