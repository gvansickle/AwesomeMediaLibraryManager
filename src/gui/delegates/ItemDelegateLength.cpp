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

#include "ItemDelegateLength.h"

#include <QTime>
#include <utils/Fraction.h>

ItemDelegateLength::ItemDelegateLength(QObject* parent) : QStyledItemDelegate(parent)
{

}

QString ItemDelegateLength::displayText(const QVariant& value, const QLocale& /*locale*/) const
{
	// Delegate for displaying track length as (HH:)?MM:SS

	// Convert from QVariant to Fraction.
	Fraction frac = value.value<Fraction>();
	double total_seconds = qint64(frac);
	QTime as_qtime = QTime::fromMSecsSinceStartOfDay(total_seconds * 1000);
	return as_qtime.toString("mm:ss");
}
