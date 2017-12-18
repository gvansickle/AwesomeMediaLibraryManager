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

#include "SettingsDialogSideWidget.h"

#include <QDebug>

SettingsDialogSideWidget::SettingsDialogSideWidget(QWidget* parent) : QListWidget(parent)
{
	// Set to IconMode view mode.  This sets l-to-r layout/Large size/Free movement.
	setViewMode(QListView::IconMode);
	// We want top-to-bottom layout.
	setFlow(QListView::Flow::TopToBottom);
	// We don't want Free movement.  Items can't be moved by user.
    setMovement(QListView::Static);
	// Don't know what this defaults to, but we don't want batched mode.
	setLayoutMode(QListView::SinglePass);
	// Re-layout the items when the view is resized.  This isn't the default.
	setResizeMode(QListView::Adjust);

	// Unclear if this makes any difference.
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

//	setIconSize(QSize(96, 84));
//	setMaximumWidth(128);
//    setSpacing(12);

	// Only single-selection make sense.
	setSelectionMode(QAbstractItemView::SingleSelection);
}

void SettingsDialogSideWidget::addPageEntry(const QString &label_text, const QIcon& icon,
                                            const QString& tooltip_str,
											const QString& statustip_str,
											const QString& whatsthis_str)
{
    QListWidgetItem *item = new QListWidgetItem(this); // passing 'this' adds the item to the QListWidget.
    item->setIcon(icon);
    item->setText(label_text);
    item->setToolTip(tooltip_str);
    item->setStatusTip(statustip_str);
	item->setWhatsThis(whatsthis_str);

    // Set various properties.
    item->setTextAlignment(Qt::AlignHCenter);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

	// Set a size hint with a width wider than the widest entry.  This
	// is to center us in the containing QListView.
	//auto oldsize = item->sizeHint();
//	item->setSizeHint(QSize(maximumWidth(), 84));
}

int SettingsDialogSideWidget::sizeHintForColumn(int column) const
{
	return QListWidget::sizeHintForColumn(column);
}

QSize SettingsDialogSideWidget::sizeHint() const
{
	///return QAbstractScrollArea::sizeHint();

	// Attempt to get the width exactly right, so we don't have a horizontal scrollbar.
	auto s = QSize();
	s.setHeight(QListWidget::sizeHint().height());
	auto sizehint0 = sizeHintForColumn(0);
	qDebug() << "sizehint0:" << sizehint0;
    s.setWidth(sizehint0/*+30*/); ///@todo Need to add something here, this isn't quite correct.

	return s;
}
