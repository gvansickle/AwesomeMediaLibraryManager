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

#include "AboutBox.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>

#include <QMessageBox>


AboutBox::AboutBox(QWidget *parent, const Qt::WindowFlags &flags) : QDialog(parent, flags)
{
    m_title_str = tr("About %1").arg(qApp->applicationDisplayName());

    QString app_name_str = qApp->applicationDisplayName();

    m_text_str = tr("<body>"
                    "<h1>%1</h1>"
                    "<h2>The Awesome Media Library Manager</h2>"
                    "<h3>Because the world needs a Media Library Manager which is Awesome.</h3>"
                    "<h4>Copyright (c) 2017 Gary R. Van Sickle</h4>"
                    "<hr>"
               "<p><a href=\"https://github.com/gvansickle/AwesomeMediaLibraryManager\">AwesomeMediaLibraryManager</a> is free software: you can redistribute it and/or modify"
               " it under the terms of the GNU General Public License as published by"
               " the Free Software Foundation, either version 3 of the License, or"
               " (at your option) any later version."
               "</p>"
               "<p>AwesomeMediaLibraryManager is distributed in the hope that it will be useful,"
               " but WITHOUT ANY WARRANTY; without even the implied warranty of"
               " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
               " GNU General Public License for more details."
               "</p>"
               "<p>You should have received a copy of the GNU General Public License"
               " along with AwesomeMediaLibraryManager.  If not, see <a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>.</p>"
               "</body>").arg(app_name_str);
}

int AboutBox::exec()
{
    QMessageBox::about(this->parentWidget(), m_title_str, m_text_str);
    //return QDialog::exec();

    return 0;
}

