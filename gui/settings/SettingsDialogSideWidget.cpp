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

SettingsDialogSideWidget::SettingsDialogSideWidget(QWidget* parent) : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setIconSize(QSize(96, 84));
    setMovement(QListView::Static);
    setMaximumWidth(128);
    setSpacing(12);
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
}
