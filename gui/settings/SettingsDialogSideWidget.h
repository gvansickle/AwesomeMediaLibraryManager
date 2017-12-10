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

#ifndef SETTINGSDIALOGSIDEWIDGET_H
#define SETTINGSDIALOGSIDEWIDGET_H

#include <QListWidget>

class QString;
class QIcon;

/**
 * @todo write docs
 */
class SettingsDialogSideWidget : public QListWidget
{
    Q_OBJECT

public:

    /**
     * Constructor
     */
    SettingsDialogSideWidget(QWidget* parent);

	void addPageEntry(const QString& label_text, const QIcon& icon,
	                  const QString& tooltip_str = QString(),
	                  const QString& statustip_str = QString(),
	                  const QString& whatsthis_str = QString());

};

#endif // SETTINGSDIALOGSIDEWIDGET_H
