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

#ifndef ACTIONHELPERS_H
#define ACTIONHELPERS_H

#include <QAction>


static inline QAction* make_action(const QIcon &icon, const QString &text, QObject *parent = nullptr,
                                   QKeySequence shortcut = QKeySequence(), const QString& status_tip = QLatin1String(""))
{
	QAction* retval = new QAction(icon, text, parent);
	retval->setShortcut(shortcut);
	retval->setStatusTip(status_tip);

	return retval;
}

#endif // ACTIONHELPERS_H
