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

class AboutPage : public QWizardPage
{
    //Q_OBJECT

public:
    AboutPage(QWidget* parent) : QWizardPage(parent)
    {
        setTitle(QString("About %1").arg(qApp->applicationDisplayName()));
        setSubTitle(QString("The Awesome Media Library Manager"));

        m_label = new QLabel(tr("This wizard will generate a skeleton C++ class "
                                  "definition, including a few functions. You simply "
                                  "need to specify the class name and set a few "
                                  "options to produce a header file and an "
                                  "implementation file for your new C++ class."));
        m_label->setWordWrap(true);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(m_label);
        setLayout(layout);
    }

private:

    QLabel *m_label;
};

AboutBox::AboutBox(QWidget *parent, const Qt::WindowFlags &flags) : QWizard(parent, flags)
{
    setOptions(QWizard::NoBackButtonOnStartPage | QWizard::NoBackButtonOnLastPage | QWizard::HaveFinishButtonOnEarlyPages);

    setWindowTitle(tr("About %1").arg(qApp->applicationDisplayName()));

    addPage(new AboutPage(this));
}
