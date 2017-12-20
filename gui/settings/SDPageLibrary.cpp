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

SDPageLibrary::SDPageLibrary(SettingsDialogBase *settings_dialog_base, QWidget *parent)
	: SettingsDialogPageBase(settings_dialog_base, parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout;
	QGroupBox *pageGroup = new QGroupBox(tr("Library Info"));
	QTreeWidget* treeWidget = new QTreeWidget;

	treeWidget->setColumnCount(1);
	auto newtags = Metadata::getNewTags();
	QString str = QString("These %1 tags were discovered:\n").arg(newtags.size());
	QList<QTreeWidgetItem*> item_list;
	for(auto s : newtags)
	{
		QString tag_name = QString::fromUtf8(s.c_str());
		qDebug() << tag_name;
		item_list.append(new QTreeWidgetItem((QTreeWidget*)nullptr, QStringList(tag_name)));
	}
	treeWidget->insertTopLevelItems(0, item_list);
	treeWidget->setHeaderLabels(QStringList("Tag Name"));

	// GroupBoxes need a layout.
	QVBoxLayout* pageGroupLayout = new QVBoxLayout;
	pageGroupLayout->addWidget(treeWidget);
	pageGroup->setLayout(pageGroupLayout);

	//mainLayout->addWidget(treeWidget);

	mainLayout->addWidget(pageGroup);
	//mainLayout->addStretch(1);
	setLayout(mainLayout);
}
void SDPageLibrary::addContentsEntry(SettingsDialogSideWidget *contents_widget)
{
	contents_widget->addPageEntry("Library", Theme::iconFromTheme("applications-multimedia"));
}
