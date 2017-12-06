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



AboutBox::AboutBox(QWidget *parent, const Qt::WindowFlags &flags) : QWizard(parent, flags)
{
    setOptions(QWizard::NoBackButtonOnStartPage | QWizard::NoBackButtonOnLastPage | QWizard::HaveFinishButtonOnEarlyPages | QWizard::NoCancelButton);

    // Turn the "Finish" button into an "Ok" button.
    setButtonText(QWizard::FinishButton, tr("OK"));

    setWindowTitle(tr("About %1").arg(qApp->applicationDisplayName()));

    addPage(new AboutPage(this));

	adjustSize();

    /// @todo This doesn't work.
    connect(this, &AboutBox::currentIdChanged, this, &AboutBox::fitToContents);
}

void AboutBox::fitToContents(int pageid)
{
    // Size Wizard to fit the page.
    adjustSize();
}

AboutPage::AboutPage(QWidget* parent) : QWizardPage(parent)
{
        setTitle(QString("%1").arg(qApp->applicationDisplayName()));
    setSubTitle(QString("<p>The Awesome Media Library Manager</p>"
    "<p>Version %1</p>"
    "<p>Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).</p>").arg(qApp->applicationVersion()));

    m_label = new QLabel(this);
    m_label->setTextFormat(Qt::RichText);
    m_label->setText(tr("<body>"
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
    "</body>"));
    m_label->setWordWrap(true);
    m_label->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);
    m_label->setOpenExternalLinks(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_label);
    setLayout(layout);
}
