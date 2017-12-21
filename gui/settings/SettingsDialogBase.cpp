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

#include "SettingsDialogBase.h"

#include <QApplication>
#include <QLayout>
#include <QStackedWidget>
#include <QMessageBox>
#include <QDebug>
#include <QDataWidgetMapper>
#include <QStandardItemModel>

#include <utils/Theme.h>

SettingsDialogBase::SettingsDialogBase(QWidget *parent, const Qt::WindowFlags &flags)
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

    // Allow user to resize the dialog.
    setSizeGripEnabled(true);

    // Create the settings model.
    m_settings_model = new QStandardItemModel(this);

    // Create the mapper.
    m_mapper = new QDataWidgetMapper(this);

    // Set the contents/page selector/side widget.
    m_contents_side_widget = new SettingsDialogSideWidget(this);

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

    connect(m_contents_side_widget, &SettingsDialogSideWidget::currentItemChanged, this, &SettingsDialogBase::changePage);

    // Connect up the buttons.
    // OK
    connect(&m_dialog_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    // Cancel, Esc
    connect(&m_dialog_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Help
    connect(&m_dialog_button_box, &QDialogButtonBox::helpRequested, this, &SettingsDialogBase::onHelpRequested);
}

void SettingsDialogBase::addPage(SettingsDialogPageBase *page)
{
    // Add the page to the page stack widget.
    m_page_stack_widget.addWidget(page);

    // Add a corresponding entry to the contents side widget.
    page->addContentsEntry(m_contents_side_widget);
}

void SettingsDialogBase::setField(const QString &name, const QVariant &value)
{
    auto index = m_reg_field_index_map.value(name, -1);

    if(index != -1)
    {
        // Found the field.
        const RegisteredField &field = m_registered_fields.at(index);
        auto retval = field.m_object->setProperty(field.m_property_name, value);
        if(!retval)
        {
            qWarning("Couldn't write to property '%s'", field.m_property_name.constData());
        }
    }
}

QVariant SettingsDialogBase::field(const QString &name) const
{
    auto index = m_reg_field_index_map.value(name, -1);
    if(index != -1)
    {
        const RegisteredField &field = m_registered_fields.at(index);
        return field.m_object->property(field.m_property_name);
    }
    else
    {
        qWarning() << QString("No such field registered:") << name;
    }

    return QVariant();
}



void SettingsDialogBase::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(!current)
    {
        current = previous;
    }

    m_page_stack_widget.setCurrentIndex(m_contents_side_widget->row(current));
}

void SettingsDialogBase::onHelpRequested()
{
    QMessageBox::information(this, "Help", "Help is not yet implemented");
}


void SettingsDialogBase::addField(const RegisteredField &field)
{
    RegisteredField local_reg_field = field;

    ///local_reg_field.resolve(defaultPropertyTable);

    /// @todo Check for dups.

    m_reg_field_index_map.insert(local_reg_field.m_name, m_registered_fields.size());
    m_registered_fields.push_back(local_reg_field);

    connect(local_reg_field.m_object, SIGNAL(destroyed(QObject*)), this, SLOT(onRegisteredFieldDestroyed(QObject*)));
}

void SettingsDialogBase::registerField(const QString &name, QWidget *widget, const char *property, const char *changedSignal)
{
    if(property == nullptr)
    {
        if(1/*isQFontComboBox*/)
        {
            property = "currentFont";
        }
    }

    if(changedSignal == nullptr)
    {
        if(1/*isQFontComboBox*/)
        {
            changedSignal = "currentFontChanged";
        }
    }

    //m_registered_fields[name] = {widget, property, changedSignal};

    //connect(widget, SIGNAL(changedSignal), [=](const QFont& newfont){ m_registered_fields[name].m_value = newfont; });
}

void SettingsDialogBase::accept()
{
    // Collect the fields here.



    QDialog::accept();
}
