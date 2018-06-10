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

#ifndef THEME_H
#define THEME_H

#include <QWidget>
#include <QIcon>
#include <QStringList>
#include <QActionGroup>

class MainWindow;

class Theme : public QWidget
{
    Q_OBJECT

public:
    explicit Theme(QWidget *parent = nullptr);

    static void initialize();

	/**
	 * Get a "Widget Styles" QActionGroup.
	 */
	static QActionGroup *getStylesActionGroup(MainWindow *main_window);

	static QString getUserDefaultStyle(const char* fallback = nullptr);

    /// @name Icon Theme interface.
    /// @{
    static QStringList GetIconThemeNames();

    static bool setIconThemeName(const QString& name);

    static QIcon iconFromTheme(const QString& icon_name);
    static QIcon iconFromTheme(const QStringList& icon_name);

    /// @}

	/**
 	 * Enumeration of some additional Qt::Key-like key names which don't exist in Qt 5.10.
 	 */
	enum Key
	{
		Key_ToggleShuffle, ///< Windows Media Player: CTRL+H
		Key_ToggleRepeat,  ///< Windows Media Player: CTRL+T
	};
	Q_ENUM(Key)

	// Return a platform- and possibly theme-specific QKeySequence corresponding to the string @a key.
	static QKeySequence keySequenceFromTheme(Theme::Key key);

protected:



public Q_SLOTS:

private:
	Q_DISABLE_COPY(Theme)

    static QStringList FindIconThemes();

	static QStringList m_available_styles;
};

#endif // THEME_H
