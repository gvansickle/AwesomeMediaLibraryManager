/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
 * @file MimeTypeDelegate.cpp
 */
#include "MimeTypeDelegate.h"

#include <QMimeType>
#include <QPainter>

#include <AMLMApp.h>
#include <gui/Theme.h>
#include <utils/DebugHelpers.h>

MimeTypeDelegate::MimeTypeDelegate(QObject *parent) : BASE_CLASS(parent)
{
}

MimeTypeDelegate::~MimeTypeDelegate()
{
}

void MimeTypeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    painter->save();

    // Draw the background.
    if (option.state & QStyle::State_Selected)
    {
            painter->fillRect(option.rect, option.palette.highlight());
    }

    // Create a copy of the incoming QStyleOptionViewItem that we can modify.
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Get the mime type.
    QMimeType mime_type = index.model()->data(index, Qt::DisplayRole).value<QMimeType>();

    // Get the mime type icon and set it in the copy of the QStyleOptionViewItem.
    opt.icon = Theme::iconFromTheme(mime_type);

    qDbo() << M_NAME_VAL(option.decorationSize);
    qDbo() << M_NAME_VAL(option.decorationPosition);
    qDbo() << M_NAME_VAL(option.showDecorationSelected);

    auto qstyle = AMLMApp::style();

    // Draw the modified CE_ItemViewItem.
    qstyle->drawControl(QStyle::CE_ItemViewItem, &opt, painter, nullptr);

    painter->restore();
}

QString MimeTypeDelegate::displayText(const QVariant &value, const QLocale &) const
{
    QMimeType mt = value.value<QMimeType>();

    return mt.name();
}

