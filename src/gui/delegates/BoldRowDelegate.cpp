/*
 * Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "BoldRowDelegate.h"

#include <QStyledItemDelegate>


BoldRowDelegate::BoldRowDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
}

BoldRowDelegate::~BoldRowDelegate() = default;

void BoldRowDelegate::setRow(ssize_t row)
{
    m_row = row;
	// Track which rows have been bolded.
	m_bolded_rows.insert(row);
    clearAllButOne();
}

void BoldRowDelegate::clearAll()
{
	if (!m_bolded_rows.empty())
	{
		m_bolded_rows.clear();
		Q_EMIT updateRequested();
	}
}

void BoldRowDelegate::clearAllButOne()
{
    if (!m_bolded_rows.empty())
    {
        m_bolded_rows.clear();
        m_bolded_rows.insert(m_row);
        Q_EMIT updateRequested();
    }
}

void BoldRowDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
	QStyledItemDelegate::initStyleOption(option, index);

	// Make m_row bold.
	if (index.row() == m_row)
	{
		option->font.setBold(true);
	}
}
