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

#ifndef ACTIONBUNDLE_H
#define ACTIONBUNDLE_H

#include <QActionGroup>

class QMenu;
class QToolBar;
class QString;
class QIcon;

class ActionBundle : public QActionGroup
{
	Q_OBJECT
	
public:
	explicit ActionBundle(QObject *parent);
	
	/**
	 * Adds a QAction separator with the given @a text to the bundle.
	 * ActionBundle takes ownership of the created QAction.
	 */
	QAction* addSection(const QString &text);

	/**
	 * Adds a QAction separator with the given @a icon and @a text to the bundle.
	 * ActionBundle takes ownership of the created QAction.
	 */
	QAction* addSection(const QIcon &icon, const QString &text);

	/**
	 * Does not transfer ownership of the QActions to @a menu.
	 */
	void appendToMenu(QMenu* menu, bool elide_separators = false);

	/**
	 * Does not transfer ownership of the QActions to @a menu.
	 */
	void prependToMenu(QMenu* menu, bool elide_separators = false);

	/**
	 * Does not transfer ownership of the QActions to @a toolbar.
	 */
	void appendToToolBar(QToolBar* toolbar, bool elide_separators = false);

private:
	Q_DISABLE_COPY(ActionBundle)

};

#endif /* ACTIONBUNDLE_H */

