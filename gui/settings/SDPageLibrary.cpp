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

#include "SDPageLibrary.h"


#include <utils/Theme.h>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QTreeWidget>
#include <logic/Metadata.h>
#include <utils/StringHelpers.h>
#include <QFormLayout>
#include <QLineEdit>
#include <QtWidgets/QLabel>
#include <QStandardItemModel>
#include <QDataWidgetMapper>
#include <QCheckBox>

static QLabel* make_qlabel(const QString& str, QWidget *parent)
{
    QLabel *retval = new QLabel(str, parent);
    retval->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    retval->setFrameStyle(QFrame::StyledPanel);
    return retval;
}

SDPageLibrary::SDPageLibrary(SettingsDialogBase *settings_dialog_base, QWidget *parent)
	: SettingsDialogPageBase(settings_dialog_base, parent)
{
	// The library info/stats group box.
	QGroupBox *lib_info = new QGroupBox(tr("Library Info"));

	/// The stats.
	// Number of songs.
	m_lib_num_songs_label = make_qlabel("12345", this);
    // Total size on disk.
	auto lib_size_on_disk = make_qlabel("55756 GB", this);

	///@todo EXPERIMENTAL
	auto check1 = new QCheckBox("Test 1", this);
	auto check2 = new QCheckBox("Test 2", this);

	// FormLayout for the stats.
	QFormLayout *lib_stats_form = new QFormLayout;
	lib_stats_form->addRow(tr("Total number of songs:"), m_lib_num_songs_label);
    lib_stats_form->addRow(tr("Total size on disk:"), lib_size_on_disk);

    ////
    lib_stats_form->addRow(tr("Check1"),check1);
    lib_stats_form->addRow(tr("Check2"),check2);
    ////

	QTreeWidget* treeWidget = new QTreeWidget;
	treeWidget->setColumnCount(1);
	auto newtags = Metadata::getNewTags();
	QString str = QString("These %1 tags were discovered:\n").arg(newtags.size());
	QList<QTreeWidgetItem*> item_list;
	for(auto s : newtags)
	{
		QString tag_name = toqstr(s);
		qDebug() << tag_name;
		item_list.append(new QTreeWidgetItem((QTreeWidget*)nullptr, QStringList(tag_name)));
	}
	treeWidget->insertTopLevelItems(0, item_list);
	treeWidget->setHeaderLabels(QStringList("Tag Name"));

	// GroupBoxes need a layout.
	QVBoxLayout* lib_info_layout = new QVBoxLayout;
	lib_info_layout->addLayout(lib_stats_form);
	lib_info_layout->addWidget(treeWidget);
	lib_info->setLayout(lib_info_layout);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	//mainLayout->addWidget(treeWidget);
	mainLayout->addWidget(lib_info);
	//mainLayout->addStretch(1);
	setLayout(mainLayout);

///M_WARNING("EXPERIMENTAL");

	auto si1 = new QStandardItem("true");
	auto si2 = new QStandardItem("false");
	m_model = new QStandardItemModel;
	m_model->appendRow({si1, si2});
	m_mapper = new QDataWidgetMapper;
	m_mapper->setModel(m_model);
	m_mapper->addMapping(check1, 0);
	m_mapper->addMapping(check2, 1);
	m_mapper->toFirst();

    registerField("m_lib_num_songs_label", m_lib_num_songs_label);
}

void SDPageLibrary::addContentsEntry(SettingsDialogSideWidget *contents_widget)
{
	contents_widget->addPageEntry("Library", Theme::iconFromTheme("applications-multimedia"));
}

void SDPageLibrary::onApply()
{
	qDebug() << "APPLIED SETTINGS:" << m_model->data(m_model->index(0, 0, QModelIndex()))
				<< m_model->data(m_model->index(0, 1, QModelIndex()));;
}
