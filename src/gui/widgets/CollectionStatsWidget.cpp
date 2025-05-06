/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "CollectionStatsWidget.h"

// Std C++
#include <type_traits>

// Qt
#include <QLabel>
#include <QVBoxLayout>

#if 1
#include <QTextDocument>
#include <QTextTable>
#include <QTextCursor>
#include <QTextFrame>
#endif

// Ours
#include <logic/PerfectDeleter.h>


CollectionStatsWidget::CollectionStatsWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("CollectionStatsWidget");

	// QDockWidget Docs: "If the dock widget is visible when widget is added, you must show() it explicitly.
	// Note that you must add the layout of the widget before you call this function; if not, the widget will not be visible."
	auto layout = new QVBoxLayout();

	m_widget_text = new QTextEdit(tr("STATS GO HERE"), this);
    m_widget_text->setReadOnly(true);
    m_widget_text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sources_model_watcher = new ModelChangeWatcher(this);
    m_summary_model = new QSortFilterProxyModel(this);

	layout->addWidget(m_widget_text);
	setLayout(layout);

    connect(m_sources_model_watcher, &ModelChangeWatcher::modelHasRows, this, &std::decay_t<decltype(*this)>::SLOT_modelChanged);

	QTimer* timer = new QTimer(this);
	connect_or_die(timer, &QTimer::timeout, this, &std::decay_t<decltype(*this)>::SLOT_modelChanged);
	timer->start(1234);
}

// Static
QDockWidget *CollectionStatsWidget::make_dockwidget(const QString &title, QWidget *parent)
{
    auto retval = new QDockWidget(tr("Collection Stats"), parent);
	auto collection_stats_widget = new CollectionStatsWidget();
	retval->setWidget(collection_stats_widget);
	retval->setObjectName("DockWidget" + collection_stats_widget->objectName());
	retval->setWidget(collection_stats_widget);
    return retval;
}

void CollectionStatsWidget::setModel(QPointer<LibraryModel> model)
{
    m_sources_model = model;
    m_sources_model_watcher->setModelToWatch(m_sources_model);
    m_summary_model->setSourceModel(model);
	QTimer::singleShot(0, this, &std::decay_t<decltype(*this)>::SLOT_modelChanged);
}

#define M_DESC_ARG(strlit, value) QString("<li><b>" strlit ":</b> %1</li>").arg(value)

void CollectionStatsWidget::SLOT_modelChanged()
{
	// Model changed (@todo Or we got a periodic timer expiration), update stats.

	QString new_txt = "<h3>Collection Stats</h3><hr>";

	if(m_sources_model != nullptr)
	{
		auto num_files = m_sources_model->rowCount();
		auto num_tracks = m_sources_model->rowCount();

#if 0 // Giving QTextDocument a whirl.
	QTextDocument doc(this);

	Q_ASSERT(doc.rootFrame());
	QTextCursor cursor(doc.rootFrame()->firstCursorPosition());
	cursor.movePosition(QTextCursor::Start);

	cursor.insertText("Collection Stats");
	QTextTable *table = cursor.insertTable(2, 2);
	table->cellAt(0,0).firstCursorPosition().insertText("Number of files:");
	table->cellAt(0,1).firstCursorPosition().insertText(QString("%1").arg(num_files));
	table->cellAt(1,0).firstCursorPosition().insertText("Number of tracks:");
	table->cellAt(1,1).firstCursorPosition().insertText(QString("%1").arg(num_tracks));

	m_widget_text->setText(doc.toHtml());

#else
	new_txt += QString("Number of tracks: %1").arg(num_tracks);
	new_txt += M_DESC_ARG("Number of files", num_files);
#endif
	}

	QStringList strlist = PerfectDeleter::instance().stats();
	new_txt += "\n" "<h3>Resource Usage</h3><hr>" "<li>" + strlist.join("</li>\n<li>") + "</li>";

	m_widget_text->setText(new_txt);
}
