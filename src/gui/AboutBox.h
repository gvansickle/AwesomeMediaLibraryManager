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

#ifndef AWESOMEMEDIALIBRARYMANAGER_ABOUTBOX_H
#define AWESOMEMEDIALIBRARYMANAGER_ABOUTBOX_H

//#include <nomocdefs.h>

#include <QtGlobal>
#include <QDialog>
#include <QString>

class AboutBox : public QDialog
{
    Q_OBJECT

public:
    AboutBox(QWidget *parent = nullptr, const Qt::WindowFlags& flags = 0);

	int exec() override;

private:
	Q_DISABLE_COPY(AboutBox)

	QString m_text_str;
	QString m_title_str;
};

#endif //AWESOMEMEDIALIBRARYMANAGER_ABOUTBOX_H
