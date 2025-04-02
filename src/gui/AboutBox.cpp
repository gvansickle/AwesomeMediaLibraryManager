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
#include <QIcon>
#include <QDialogButtonBox>

#include <utils/StringHelpers.h>
#include <resources/VersionInfo.h>


AboutBox::AboutBox(QWidget *parent, const Qt::WindowFlags &flags) : QDialog(parent, flags)
{
	setObjectName("AboutBox");

	QString app_name_str = qApp->applicationDisplayName();
	QString app_full_version_info = toqstr(VersionInfo::get_full_version_info_string());

    m_title_str = tr("About %1").arg(qApp->applicationDisplayName());

    m_text_str = tr(
	    "<body>"
        "<h1>%1</h1>"
        "<h2>The Awesome Media Library Manager</h2>"
        "<h3>Because the world needs a Media Library Manager which is Awesome.</h3>"
        "<h4>Version %2</h4>"
		"<h4>Copyright (c) 2017, 2018 Gary R. Van Sickle</h4>"
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
		    "<hr>"
		    "</body>").arg(app_name_str).arg(app_full_version_info);

	// Main layout is vertical.
    auto mainLayout = new QVBoxLayout();

	// Above-buttons section is divided in two: left for icon, right for About text.
	auto hlayout = new QHBoxLayout();

	QIcon icon = parent->windowIcon();
	auto icon_as_label = new QLabel();
	icon_as_label->setPixmap(icon.pixmap(128));

	// The main About text QLabel.
	auto main_text = new QLabel(m_text_str, this);
	main_text->setWordWrap(true);
	// About text is link-clickable with mouse or keyboard, text can be selected and copied with mouse.
	main_text->setTextInteractionFlags(Qt::TextBrowserInteraction);
	main_text->setOpenExternalLinks(true);

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);

	hlayout->addWidget(icon_as_label, 0, Qt::AlignTop | Qt::AlignHCenter);
	hlayout->addWidget(main_text);

	mainLayout->addLayout(hlayout);
	mainLayout->addWidget(button_box);

    setLayout(mainLayout);

	setWindowTitle(m_title_str);
}

int AboutBox::exec()
{
    return QDialog::exec();
}

