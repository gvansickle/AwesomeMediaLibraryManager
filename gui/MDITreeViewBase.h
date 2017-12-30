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
        
signals:
    
    /**
     * Signal is emitted when a row is selected or deselected in the QTreeView.
     */
    void copyAvailable(bool);
        

public:
    explicit MDITreeViewBase(QWidget *parent = Q_NULLPTR);

    ///
    /// Public interface.
    ///

    void newFile();

    virtual bool loadFile(QUrl load_url);
    bool save();
    bool saveAs();
    bool saveFile(QUrl save_url, QString filter);

    /// Returns the current basename of this window's backing file.
    QString userFriendlyCurrentFile() const;

    /// Returns the full URL of this window's backing file.
    QUrl getCurrentUrl() const;

    /// Returns the name to be displayed as this view's windowTitle(), e.g. in tabs.
    /// Default implementation returns userFriendlyCurrentFile().
    virtual QString getDisplayName() const;
    
    /// Return an action for the MainWindow's Window menu.
    QAction* windowMenuAction() const { return m_act_window; };

public slots:
    
    /// @name Edit handlers.
    /// @{
    
    /// View is read-only by default.  Does nothing.
    virtual void onCut() {}
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

    /// Protected function which is used to set the view's filename properties on a save or load.
    /// Called by loadFile() and saveFile().
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

    /**
     * Called by closeEvent().  If document shows as modified, pops up a "Do you want to save?" box,
     * then calls save() or not depending on the user's choice.
     * @return false if file was modified and user cancelled, true otherwise.
     */
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
    
    QAction *m_act_window;

};

#endif // MDITREEVIEWBASE_H
