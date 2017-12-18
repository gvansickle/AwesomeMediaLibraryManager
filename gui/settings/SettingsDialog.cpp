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

#include "SettingsDialog.h"
#include "SDPageAppearance.h"
#include "SDPageLibrary.h"

#include <QApplication>
#include <QLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QMessageBox>
#include <QDebug>

#include <utils/Theme.h>


SettingsDialog::SettingsDialog(QWidget *parent, const Qt::WindowFlags &flags)
	: QDialog(parent, flags),
	  m_dialog_button_box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply
		                      | QDialogButtonBox::Help | QDialogButtonBox::RestoreDefaults,
	                      Qt::Horizontal, this),
	  m_page_stack_widget(this)
{
	/// @todo Experiments.  Where do these strings, set on the Dialog itself, show up, if anywhere?
	setStatusTip("SettingsDialog StatusTip");
	setToolTip("SettingsDialog toolTip");
	setWhatsThis("SettingsDialog what'sThis");

	setWindowTitle(tr("Settings"));

	setSizeGripEnabled(true);

    // Set the contents/page selector/side widget.
    m_contents_side_widget = new SettingsDialogSideWidget(this);

	// Add all the pages.
	addPage(new SDPageAppearance(this));
	addPage(new SDPageLibrary(this));

	// Now set up the layouts.

	// HBox containing the Sidebar and the page contents.
	QHBoxLayout *horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(m_contents_side_widget);
	horizontalLayout->addWidget(&m_page_stack_widget, 1);

//	QHBoxLayout *buttonsLayout = new QHBoxLayout;
//	//buttonsLayout->addStretch(1);
//	buttonsLayout->addWidget(&m_dialog_button_box);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(horizontalLayout);
	mainLayout->addStretch(1);
	//mainLayout->addSpacing(12);
	//mainLayout->addLayout(buttonsLayout);
	mainLayout->addWidget(&m_dialog_button_box);
	setLayout(mainLayout);

	connect(m_contents_side_widget, &SettingsDialogSideWidget::currentItemChanged, this, &SettingsDialog::changePage);

	// Connect up the buttons.
    // OK
    connect(&m_dialog_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    // Cancel, Esc
    connect(&m_dialog_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Help
	connect(&m_dialog_button_box, &QDialogButtonBox::helpRequested, this, &SettingsDialog::onHelpRequested);
}

void SettingsDialog::addPage(SettingsDialogPageBase *page)
{
	// Add the page to the page stack widget.
	m_page_stack_widget.addWidget(page);
	// and add a corresponding entry to the contents side widget.
	page->addContentsEntry(m_contents_side_widget);
}

void SettingsDialog::setField(const QString &name, const QVariant &value)
{
	//m_registered_fields[name] = value;
}

QVariant SettingsDialog::field(const QString &name) const
{
    if(m_registered_fields.contains(name))
    {
        //return m_registered_fields[name];
    }
    else
    {
        qWarning() << QString("No such field registered:") << name;
    }

	return QVariant();
}


void SettingsDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if(!current)
	{
		current = previous;
	}

	m_page_stack_widget.setCurrentIndex(m_contents_side_widget->row(current));
}

void SettingsDialog::onHelpRequested()
{
	QMessageBox::information(this, "Help", "Help is not yet implemented");
}

void SettingsDialog::registerField(const QString &name, QWidget *widget, const char *property, const char *changedSignal)
{
    m_registered_fields[name] = {widget, property, changedSignal};
}

void SettingsDialog::accept()
{
	// Collect the fields here.



	QDialog::accept();
}

