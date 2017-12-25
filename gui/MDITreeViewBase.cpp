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
#include <QHeaderView>
#include <QSaveFile>
#include <logic/LibrarySortFilterProxyModel.h>

#include "gui/NetworkAwareFileDialog.h"

MDITreeViewBase::MDITreeViewBase(QWidget* parent) : QTreeView(parent)
{
	// Full Url to the file backing this view.
	m_current_url = QUrl();

	m_isUntitled = true;

	setAttribute(Qt::WA_DeleteOnClose);

	// Enable sorting for this view.
	setSortingEnabled(true);
	// ...but start unsorted, and don't show the sort indicator.
	header()->setSortIndicator(m_previous_sort_column, m_sort_order);
	header()->setSortIndicatorShown(false);

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
	setEditTriggers(QAbstractItemView::EditKeyPressed);

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

	/// @todo Connect a contentsChanged signal to a docWasModified slot here?
}

bool MDITreeViewBase::loadFile(QUrl load_url)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QFile file(load_url.toLocalFile());
	if(!file.open(QFile::ReadOnly | QFile::Text))
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, qApp->applicationDisplayName(),
							QString("Cannot read file %1:\n%2.").arg(load_url.toString()).arg(file.errorString()));
		return false;
	}

	// Call the overridden function to serialize the doc.
	deserializeDocument(file);

	QApplication::restoreOverrideCursor();

	setCurrentFile(load_url);

	/// @todo Connect to docWasModified.
	//self.document().contentsChanged.connect(self.documentWasModified)

	return true;
}

bool MDITreeViewBase::save()
{
	if(m_isUntitled)
	{
		return saveAs();
	}
	else
	{
		return saveFile(m_current_url, m_current_filter);
	}
}

bool MDITreeViewBase::saveAs()
{
	QString state_key = getSaveAsDialogKey();
	auto retval = NetworkAwareFileDialog::getSaveFileUrl(this, "Save As", m_current_url, defaultNameFilter(), state_key);

	QUrl file_url = retval.first;
	QString filter = retval.second;

	if(file_url.isEmpty())
	{
		return false;
	}

	/// @todo Don't need this?  At least here?
	/// mo = re.search(r"\.([^.]*)$", file_url.toString())
	/// playlist_type = mo[1]

	return saveFile(file_url, filter);
}

bool MDITreeViewBase::saveFile(QUrl save_url, QString filter)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	QSaveFile savefile(save_url.toLocalFile());

	if(!savefile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, qApp->applicationDisplayName(),
							QString("Cannot write file ") + save_url.toString() + ": " + savefile.errorString());
		return false;
	}
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
	setWindowModified(false);
	setWindowTitle(getDisplayName() + "[*]");
}

QString MDITreeViewBase::getDisplayName() const
{
	return userFriendlyCurrentFile();
}

void MDITreeViewBase::closeEvent(QCloseEvent* event)
{
	if(maybeSave())
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MDITreeViewBase::documentWasModified()
{
	setWindowModified(isModified());
}

void MDITreeViewBase::headerMenu(QPoint pos)
{
	auto globalPos = mapToGlobal(pos);
	auto menu = new QMenu("Show/Hide Columns");
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

#if 0
void MDITreeViewBase::paintEvent(QPaintEvent* event)
{
	QTreeView::paintEvent(event);
	// Take over for the base class' call to paintDropIndicator().

}
#endif

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

bool MDITreeViewBase::maybeSave()
{
	/// Default implementation to give the user one last chance to save a modified document when he's closing it.
	if(isWindowModified())
	{
		auto ret = QMessageBox::warning(this, qGuiApp->applicationDisplayName(),
								  QString("'%1' has been modified.\nDo you want to save your changes?").arg(userFriendlyCurrentFile()),
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
