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

#include "MDITreeViewBase.h"

#include <QGuiApplication>
#include <QApplication>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QHeaderView>
#include <QSaveFile>
#include <logic/LibraryEntryMimeData.h>
#include <logic/LibrarySortFilterProxyModel.h>
#include <logic/proxymodels/ModelHelpers.h>
#include "gui/NetworkAwareFileDialog.h"
#include "utils/ConnectHelpers.h"
#include "utils/DebugHelpers.h"
#include "helpers/Tips.h"
#include "logic/proxymodels/ModelChangeWatcher.h"
#include "logic/proxymodels/ModelHelpers.h"
#include "logic/proxymodels/QPersistentModelIndexVec.h"


MDITreeViewBase::MDITreeViewBase(QWidget* parent) : QTreeView(parent)
{
	// Window menu action.
	m_act_window = new QAction(this);
	m_act_window->setCheckable(true);
	connect_trig(m_act_window, this, &MDITreeViewBase::show);
	connect(m_act_window, SIGNAL(triggered()), this, SLOT(setFocus()));

	// ModelChangeWatcher for keeping "Select All" status updated.
	m_select_all_model_watcher = new ModelChangeWatcher(this);
	connect(m_select_all_model_watcher, &ModelChangeWatcher::modelHasRows, this, &MDITreeViewBase::selectAllAvailable);

	// Full Url to the file backing this view.
	m_current_url = QUrl();

	m_isUntitled = true;

	setAttribute(Qt::WA_DeleteOnClose);

M_WARNING("EXPERIMENTAL");
	auto f = QFont();
	qDebug() << "Original Font size:" << f.pointSize() << "Font:" << f;
	f.setPointSize(10);
//	f = QApplication::font("QTreeView"); /// @note: This comes back as 12 pt., and is what the view uses by default.
	qDebug() << "New Font size:" << f.pointSize() << "Font:" << f;
	setFont(f);

	// Enable sorting for this view.
	setSortingEnabled(true);
	// ...but start unsorted, and don't show the sort indicator.
	header()->setSortIndicator(m_previous_sort_column, m_sort_order);
	header()->setSortIndicatorShown(false);
	setTips(header(), tr("Header Row<br>Click to cycle through column sort modes."),
		 tr("This is the view header."),
		 tr("<h4>Header Row</h4>This is the view's header row.  Click the header of a column to cycle between unsorted, sort ascending, and sort descending modes."));

	// Only allow selection of full rows.
	setSelectionBehavior(QAbstractItemView::SelectRows);

	// Only allow the entire row to show focus.
	setAllColumnsShowFocus(true);

	// All rows are the same height.
	setUniformRowHeights(true);

	// No actual root item.
	setRootIndex(QModelIndex());

	// Item's aren't expandable.
	setItemsExpandable(false);
	setExpandsOnDoubleClick(false);

	// Eliminate the "lines to nowhere" from the nonexistent root item to the top-level entries.
	setRootIsDecorated(false);

	// Set which actions will cause the view to enter edit mode.
	// We want double-click to not cause an entry to edit mode here, since that will be used for the
	// "start playing this entry" action.
	// "EditKeyPressed" appears to mean F2.
	/// @todo Actually, default should probably be no editing.  We also want item activation (Enter) to cause a Playlist item to play,
	/// which is apparently hooked to the activated() signal on QAbstractItemView: "QAbstractItemView::activated(const QModelIndex &index)
	///   This signal is emitted when the item specified by index is activated by the user. How to activate items depends on the platform;
	///   e.g., by single- or double-clicking the item, or by pressing the Return or Enter key when the item is current."
	setEditTriggers(QAbstractItemView::EditKeyPressed);

	setAlternatingRowColors(true);

	// Enable smooth scrolling by default.
	/// @todo Should be a user-settable parameter.
	/// @note per Qt5 docs: "default value comes from the style via the QStyle::SH_ItemView_ScrollMode style hint.".
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	// We'll use the default context menu event mechanism.
	setContextMenuPolicy(Qt::DefaultContextMenu);

	// Connect to the activated() signal, i.e. when the user hits "Enter".
	connect(this, &MDITreeViewBase::activated, this, &MDITreeViewBase::onActivated);

	// Hook things up for our tri-state column-sorting implementation.
	connect(header(), &QHeaderView::sectionClicked, this, &MDITreeViewBase::onSectionClicked);

	// Connect up our custom Header context menu.
	connect(header(), &QHeaderView::customContextMenuRequested, this, &MDITreeViewBase::headerMenu);
}

/**
 * Called by the MainWindow immediately after a new, empty MDI child window is created.
 */
void MDITreeViewBase::newFile()
{
static qint64 sequenceNumber = 0;

	// We don't have an actual title yet.
	m_isUntitled = true;

	// Create a default filename.
	m_current_url = QUrl(getNewFilenameTemplate().arg(sequenceNumber));
	sequenceNumber += 1;

	// Set the window title to the Display Name, which defaults to the filename, plus the Qt "is modified" placeholder.
	setWindowTitle(getDisplayName() + "[*]");
    // Set the name we'll give to the MainWindow's Window menu.
	m_act_window->setText(getDisplayName());

    // CwriteFilenew, empty model.
    setEmptyModel();
}


bool MDITreeViewBase::save()
{
	if(m_isUntitled)
	{
		return saveAs();
	}
	else
	{
        return writeFile(m_current_url, m_current_filter);
	}
}

bool MDITreeViewBase::saveAs()
{
	QString state_key = getSaveAsDialogKey();
	auto retval = NetworkAwareFileDialog::getSaveFileUrl(this, tr("Save As"), m_current_url, defaultNameFilter(), state_key);

	QUrl file_url = retval.first;
	QString filter = retval.second;

	if(file_url.isEmpty())
	{
		return false;
	}

    return writeFile(file_url, filter);
}

bool MDITreeViewBase::readFile(QUrl load_url)
{
	QFile file(load_url.toLocalFile());
	if(!file.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::warning(this, qApp->applicationDisplayName(),
							QString("Cannot read file %1:\n%2.").arg(load_url.toString()).arg(file.errorString()));
		return false;
	}

    QApplication::setOverrideCursor(Qt::WaitCursor);

	// Call the overridden function to serialize the doc.
	deserializeDocument(file);

	QApplication::restoreOverrideCursor();

	setCurrentFile(load_url);

	/// @todo Connect to docWasModified.
	//self.document().contentsChanged.connect(self.documentWasModified)

	return true;
}

bool MDITreeViewBase::writeFile(QUrl save_url, QString filter)
{
    QSaveFile savefile(save_url.toLocalFile());

    if(!savefile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, qApp->applicationDisplayName(),
                            QString("Cannot write file ") + save_url.toString() + ": " + savefile.errorString());
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    serializeDocument(savefile);
    savefile.commit();

    QApplication::restoreOverrideCursor();

    setCurrentFile(save_url);
    return true;
}

QString MDITreeViewBase::userFriendlyCurrentFile() const
{
	return m_current_url.fileName();
}

QUrl MDITreeViewBase::getCurrentUrl() const
{
	return m_current_url;
}

void MDITreeViewBase::setCurrentFile(QUrl url)
{
	// Protected function which is used to set the view's filename properties on a save or load.
	m_current_url = url;
	m_isUntitled = false;
	setWindowFilePath(url.toString());
    qDebug() << "windowFilePath()" << windowFilePath();
	setWindowModified(false);
	setWindowTitle(getDisplayName() + "[*]");
	m_act_window->setText(getDisplayName());
}

QString MDITreeViewBase::getDisplayName() const
{
    return userFriendlyCurrentFile();
}

//
// Base class overrides.
//

void MDITreeViewBase::setModel(QAbstractItemModel* model)
{
    qDebug() << "BASE SETTING MODEL:" << model;

    m_select_all_model_watcher->disconnectFromCurrentModel();
    this->BASE_CLASS::setModel(model);
    m_select_all_model_watcher->setModelToWatch(model);
}

//
// Public slots.
//

void MDITreeViewBase::onCopy()
{
    // Get the current selection.
    QModelIndexList mil = selectionModel()->selectedRows();

	LibraryEntryMimeData* copied_rows = selectedRowsToMimeData(mil);

	if(copied_rows == nullptr)
	{
		qWarning() << "Couldn't get a QMimeData object for selection's QModelIndexList:" << mil;
		return;
	}

    // Copy the rows to the clipboard.
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setMimeData(copied_rows, QClipboard::Clipboard);
}

void MDITreeViewBase::onSelectAll()
{
    selectAll();
}


void MDITreeViewBase::closeEvent(QCloseEvent* event)
{
	if(okToClose())
	{
		emit closing(this, underlyingModel());
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

LibraryEntryMimeData* MDITreeViewBase::selectedRowsToMimeData(const QModelIndexList& row_indexes)
{
	auto mil = row_indexes;

	if(mil.isEmpty())
	{
		// Nothing to copy.
		qWarning() << "EMPTY MODELINDEXLIST, RETURNING NULLPTR" <<  row_indexes;
		return nullptr;
	}

	auto m = model();
	LibraryEntryMimeData* copied_rows = qobject_cast<LibraryEntryMimeData*>(m->mimeData(mil));

	return copied_rows;
}

void MDITreeViewBase::documentWasModified()
{
	setWindowModified(isModified());
}

void MDITreeViewBase::headerMenu(QPoint pos)
{
	auto globalPos = mapToGlobal(pos);
	auto menu = new QMenu(tr("Show/Hide Columns"));
	for(qint64 col = 0;  col < model()->columnCount(); ++col)
	{
		auto action = new QAction(menu);
		action->setText(model()->headerData(col, Qt::Horizontal).toString());
		action->setCheckable(true);
		action->setChecked(!header()->isSectionHidden(col));
		action->setData(col);
		menu->addAction(action);
	}
	auto selectedItem = menu->exec(globalPos);
	if(selectedItem)
	{
		qDebug() << QString("Selected:") << selectedItem->data() << selectedItem->text();
		int section = selectedItem->data().toInt();
		header()->setSectionHidden(section, !header()->isSectionHidden(section));
	}
}

/**
 * Overloaded to handle tri-state column header sort clicking.
 * May have been better to derive a new QHeaderView, but this works and there's not much code to it.
 */
void MDITreeViewBase::onSectionClicked(int logicalIndex)
{
	qDebug() << "Section Header Clicked:" << logicalIndex;

	if(logicalIndex != m_previous_sort_column)
	{
		// It's a new column.
		qDebug() << "First click, New column: " << m_previous_sort_column << "->" << logicalIndex;
		m_sort_order = Qt::AscendingOrder;
		m_previous_sort_column = logicalIndex;
		header()->setSortIndicatorShown(true);
	}
	else
	{
		// Same column as last time.
		if(m_sort_order == Qt::DescendingOrder)
		{
			// This is the third click.  Go back to the unsorted state.
			qDebug() << "Third click, same column, going back to unsorted. " << m_previous_sort_column << "->" << logicalIndex;
			m_previous_sort_column = -1;
			// Set up for the next click to sort ascending.
			logicalIndex = -1;
		}
		else
		{
			// It's the second click.
			qDebug() << "Second click, same column: " << m_previous_sort_column << "->" << logicalIndex;
			m_sort_order = Qt::DescendingOrder;
		}
	}

	header()->setSortIndicator(logicalIndex, m_sort_order);
}

void MDITreeViewBase::contextMenuEvent(QContextMenuEvent* event)
{
	// Check if the click was on an item or in the blank area, and dispatch accordingly.
	// Note that indexAt() takes a point in viewport coordinates.
	QModelIndex index = indexAt(viewport()->mapFromGlobal(event->globalPos()));

	if(index.isValid())
	{
		// Open context menu for the current selection.
		qDebug() << "MODEL INDEX:" << index;

		// This item should be in the current selection.
		auto selected_row_indexes = selectionModel()->selectedRows(0);
		auto selected_row_pindexes = QPersistentModelIndexVec(selected_row_indexes);

		if(selected_row_pindexes.size() == 0)
		{
			qWarning() << "Should have more than one selected row, got 0";
		}

		onContextMenuSelectedRows(event, selected_row_pindexes);
	}
	else
	{
		// Open the blank area context menu.
		qDebug() << "NO VALID INDEX";

		onContextMenuViewport(event);
	}
}

void MDITreeViewBase::onActivated(const QModelIndex& index)
{
	qDebug() << "Base class ignoring activated signal at index:" << index;
}

void MDITreeViewBase::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	this->QTreeView::selectionChanged(selected, deselected);

	if(!selected.empty())
	{
		emit copyAvailable(true);
		emit cutAvailable(!isReadOnly());
	}
	else
	{
		emit copyAvailable(false);
		emit cutAvailable(false);
	}
}

bool MDITreeViewBase::viewportEvent(QEvent* event)
{
	if(event->type() == QEvent::ToolTip)
	{
		QHelpEvent* helpevent = dynamic_cast<QHelpEvent*>(event);
		if(!indexAt(helpevent->pos()).isValid())
		{
//			qDebug() << "No valid item at tooltip pos";
			bool retval = onBlankAreaToolTip(helpevent);
			if(retval == false)
			{
				// Didn't handle it.  Send it to the base class.
				return QTreeView::viewportEvent(event);
			}
			else
			{
				// We handled it.  Return true "to indicate to the event system that the event has been handled, and needs no further processing".
				return true;
			}
		}
	}
	return QTreeView::viewportEvent(event);
}

bool MDITreeViewBase::onBlankAreaToolTip(QHelpEvent* event)
{
	// By default, we don't handle the blank area tooltip.
	(void)event;
	return false;
}

bool MDITreeViewBase::okToClose()
{
	/// Default implementation to give the user one last chance to save a modified document when he's closing it
	/// or the whole application.

	/// @note Not using QWidget::isWindowModified() here.  Idea is that the view may want to not show the
	///       modified state, even when it needs saving.  Not sure that makes sense, but that's my thinking here.
	if(isModified())
	{
		auto ret = QMessageBox::warning(this, qGuiApp->applicationDisplayName(),
								  tr("'%1' has been modified.\nDo you want to save your changes?").arg(userFriendlyCurrentFile()),
									  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
								  );
		if(ret == QMessageBox::Save)
		{
			return save();
		}

		if(ret == QMessageBox::Cancel)
		{
			return false;
		}
	}

	return true;
}

QMdiSubWindow* MDITreeViewBase::getQMdiSubWindow() const
{
	return qobject_cast<QMdiSubWindow*>(parent());
}
