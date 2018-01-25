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

#include <QTreeView>
#include <QUrl>

#include "mdi/MDIModelViewPair.h"
//#include "logic/proxymodels/ModelHelpers.h"

class QMdiSubWindow;
class QContextMenuEvent;
class QFileDevice;
class ModelChangeWatcher;
class QPersistentModelIndexVec;
class LibraryEntryMimeData;

/**
 * Base class for the various tree views in the app.
 */
class MDITreeViewBase : public QTreeView
{
    Q_OBJECT

    using BASE_CLASS = QTreeView;

Q_SIGNALS:

	/**
	 * Signal emitted just before the QCloseEvent is accepted.
	 * @note view_that_is_closing can not be consided to be dereferenceable.  The view may have
	 * already been deleted before the signal is delivered.
	 */
	void closing(MDITreeViewBase* view_that_is_closing, QAbstractItemModel* modelptr);

    /**
     * Signal is emitted when a selection is available for copying in the QTreeView.
     */
    void copyAvailable(bool);

    /**
     * Signal emitted when the "cutability" status changes.  Read-only views will only send false.
     */
    void cutAvailable(bool);

    /**
     * Signal emitted when there is a change in the "Select All" status.
     */
    void selectAllAvailable(bool);


public:
    explicit MDITreeViewBase(QWidget *parent = Q_NULLPTR);

    ///
    /// Public interface.
    ///

	/**
	 * Called by the MainWindow immediately after a new, empty MDI child window is created.
	 * Gives the view a dummy name and sets the window title.
	 */
    void newFile();

    bool save();
    bool saveAs();

    /// @name Open functions.
    /// Create factory functions like these in each derived class.
    /// These static "open" functions would be good candidates for the virtual static methods which don't exist in C++.
    /// These are just dummied out here for demonstration purposes.
    /// @{
#if 0
    /**
     * Pop up an 'Open file" dialog and open a new View on the file specified by the user.
     */
	static MDIModelViewPair open(QWidget* parent) { return nullptr; }

    /**
     * Open the specified QUrl.  Called by open(QWidget*).
	 * Among other things, this function is responsible for calling setCurrentFilename().
     */
	static MDIModelViewPair openFile(QUrl open_url, QWidget* parent) { return nullptr; }

    /**
     * Open a new view on the given model.
	 * This is largely analogous to the openFile() call, though @a model here must be an existing, valid model.
	 * Among other things, this function is responsible for calling setCurrentFilename().
     */
	static MDIModelViewPair openModel(QSharedPointer<QAbstractItemModel> model, QWidget* parent) { return nullptr; }
    /// @}
#endif

    /// Returns the current basename of this window's backing file.
    QString userFriendlyCurrentFile() const;

    /// Returns the full URL of this window's backing file.
    QUrl getCurrentUrl() const;

    /// Returns the name to be displayed as this view's windowTitle(), e.g. in tabs.
    /// Default implementation returns userFriendlyCurrentFile().
    virtual QString getDisplayName() const;

    /// Return an action for the MainWindow's Window menu.
	QAction* windowMenuAction() const { return m_act_window; }

    /// Override if derived classes are not read-only.
	virtual bool isReadOnly() const { return true; }

	/**
	 * Returns the QMdiSubwindow instance holding this MDITreeViewBase-derived instance.
	 */
	QMdiSubWindow* getQMdiSubWindow() const;

    //
    // Base class overrides.
    //

	// Overridden from QTreeView.
	void setModel(QAbstractItemModel *model) override;

	virtual QAbstractItemModel* underlyingModel() const = 0;


public Q_SLOTS:

    /// @name Edit handlers.
    /// @{

    /// View is read-only by default.  Does nothing.
    virtual void onCut() {}
	/// Copy selection to clipboard.
    virtual void onCopy();
    /// View is read-only by default.  Does nothing.
    virtual void onPaste() {}
    virtual void onSelectAll();
    /// View is read-only by default.  Does nothing.
    virtual void onDelete() {}
    /// @}

protected:

    QUrl m_current_url;
    QString m_current_filter;
    bool m_isUntitled = true;

    /// Override in derived classes to set an empty model.
    /// Used when newFile() is called.
    virtual void setEmptyModel() = 0;

	bool saveFile(const QUrl& filename, const QString& filter);

    /// Protected function which is used to set the view's filename properties on a read or write.
    /// Called by readFile() and writeFile().
    void setCurrentFile(QUrl url);

	/**
	 * The function that does the actual reading of the file.
	 * Called by openFile().
	 */
    virtual bool readFile(QUrl load_url);

	/**
	 * The function that finally does the actual saving of the file.
	 * Called by saveFile().
	 */
    virtual bool writeFile(QUrl save_url, QString filter);

	void closeEvent(QCloseEvent* event) override;

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

	virtual void serializeDocument(QFileDevice& file) = 0;
    virtual void deserializeDocument(QFileDevice& file) = 0;

    ///
    /// Not pure-virtual, but designed-to-be-overridden functions.
    ///

	virtual QModelIndexList selectedRowIndexes() const;

	virtual QPersistentModelIndexVec selectedRowPindexes() const;

	/// Creates a LibraryEntryMimeData object containing copies of the given rows.
	virtual LibraryEntryMimeData* selectedRowsToMimeData(const QModelIndexList& row_indexes);

protected Q_SLOTS:

	/**
	 * Connect this slot to any model signals which indicate there are unsaved changes.
	 * By default, calls setWindowModified(isModified()).
	 */
    virtual void documentWasModified();

    virtual void headerMenu(QPoint pos);

    virtual void onSectionClicked(int logicalIndex);

	/**
	 * Context menu handler.  Base class implementation in QWidget ignores the event.
	 * This override dispatches the event to either onContextMenuIndex() or onContextMenuViewport()
	 * as appropriate.  Derived classes shouldn't need to override this.
	 */
	void contextMenuEvent(QContextMenuEvent* event) override;

	/**
	 * Override to implement context menu handler for @a index.  @a index is guaranteed to be valid.
	 * @param event
	 * @param index
	 */
	virtual void onContextMenuSelectedRows(QContextMenuEvent* event, const QPersistentModelIndexVec& row_indexes) { Q_UNUSED(event); Q_UNUSED(row_indexes); }

	/**
	 * Override to implement context menu handler for the viewport (blank area of treeview).
	 * @param event
	 * @param index
	 */
	virtual void onContextMenuViewport(QContextMenuEvent* event) { Q_UNUSED(event); }

	/**
     * Slot called when the user activates (hits Enter or double-clicks) on an item.
	 * @param index
	 */
	virtual void onActivated(const QModelIndex& index);

protected:

    /**
     * Called whenever the view's selection changes.  We override it here to emit the copyAvailable() signal.
     */
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    bool viewportEvent(QEvent *event) override;

    /// Return a string suitable for use as a key in the QSettings file.  Used
    /// to save and restore the state of the "Save As" dialog.
    /// Default is none, settings won't be saved.
    virtual QString getSaveAsDialogKey() const { return QString(); }

    /// Return True if you handle it, False if you don't.
    virtual bool onBlankAreaToolTip(QHelpEvent* event);

    /**
     * Called by closeEvent().  If document shows as modified, pops up a "Do you want to save?" box,
     * then calls save() or not depending on the user's choice.
     * @return false if file was modified and user cancelled, true otherwise.
     */
	virtual bool okToClose();

    /// Helper function to convert from incoming proxy QModelIndexes to actual underlying model indexes.
    virtual QModelIndex to_underlying_qmodelindex(const QModelIndex &proxy_index) = 0;

    /// Helper function to convert from underlying model indexes to proxy QModelIndexes.
    virtual QModelIndex from_underlying_qmodelindex(const QModelIndex& underlying_index) = 0;

	/// The QAction we'll give to the MainWindow for inclusion in the Window menu.
	QAction *m_act_window;

private:
    Q_DISABLE_COPY(MDITreeViewBase)

    /// ModelChangeWatcher object for keeping "Select All" enable status correct.
    ModelChangeWatcher* m_select_all_model_watcher;

    /// The column which we last saw was being sorted.
    /// This is used to enable tri-state column sort functionality.
    int m_previous_sort_column {-1};

    Qt::SortOrder m_sort_order { Qt::AscendingOrder };

};

#endif // MDITREEVIEWBASE_H
