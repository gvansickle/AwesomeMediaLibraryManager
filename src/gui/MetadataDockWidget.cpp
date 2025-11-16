/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
/// @file

#include "MetadataDockWidget.h"

// Std C++
#include <functional>

// Qt
#include <QItemSelection>
#include <QTreeView>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QDebug>
#include <QDataWidgetMapper>
#include <QLineEdit>
#include <QSplitter>
#include <QRegularExpression>
#include <QHeaderView>
#include <QAbstractItemModelTester>

// Ours
#include <AMLMApp.h>
#include <utils/EnumFlagHelpers.h>

#include "widgets/PixmapLabel.h"
#include "logic/ModelUserRoles.h"
#include <logic/MetadataAbstractBase.h>
#include <logic/LibraryEntry.h>
#include <logic/models/LibraryModel.h>
#include <gui/MDITreeViewBase.h>
#include <gui/Theme.h>
#include <jobs/CoverArtJob.h>
#include <logic/proxymodels/ModelChangeWatcher.h>
#include <logic/proxymodels/ModelHelpers.h>
#include <logic/proxymodels/SelectionFilterProxyModel.h>
#include <logic/AMLMTagMap.h>
#include <utils/RegisterQtMetatypes.h> //< For at least std::optional<bool>.

#include <utils/StringHelpers.h>

#include <logic/proxymodels/LibrarySortFilterProxyModel.h>


MetadataDockWidget::MetadataDockWidget(const QString& title, QWidget *parent, Qt::WindowFlags flags) : QDockWidget(title, parent, flags)
{
    setNumberedObjectName(this);

    // Set up the proxy model.
    m_proxy_model = new SelectionFilterProxyModel(this);
	// Set up the model tester.
	// See https://doc.qt.io/qt-6/qabstractitemmodeltester.html
	// ...and https://www.kdab.com/new-in-qt-5-11-improvements-to-the-model-view-apis-part-2/
	// new QAbstractItemModelTester(m_proxy_model, QAbstractItemModelTester::FailureReportingMode::Warning, this);
	// Set up the watcher.
	m_proxy_model_watcher = new ModelChangeWatcher(this);
	m_proxy_model_watcher->setModelToWatch(m_proxy_model);

    // Main widget is a splitter.
    auto mainWidget = new QSplitter(this);
    mainWidget->setOrientation(Qt::Vertical);
//    auto mainWidget = new QWidget(this);

    m_metadata_tree_view = new QTreeView(this);
    m_metadata_tree_view->setModel(m_proxy_model);

    m_metadata_widget = new QTreeWidget(this);
    m_metadata_widget->setRootIsDecorated(false);
    m_metadata_widget->setColumnCount(2);
    m_metadata_widget->setHeaderLabels(QStringList() << "Key" << "Value");
	// Set resize behavior.
	m_metadata_widget->header()->setStretchLastSection(false);
	m_metadata_widget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// The Cover Art label.
    m_cover_image_label = new PixmapLabel(this);
    m_cover_image_label->setText("IMAGE HERE");

	/// @todo Make this into the real Metadata tree view.  Until then, keep it hidden.
//	// Main layout is vertical.
//	auto mainLayout = new QVBoxLayout();
//	mainLayout->addWidget(m_metadata_tree_view);
	m_metadata_tree_view->hide();

#if 1 // splitter
    mainWidget->addWidget(m_metadata_widget);
    mainWidget->addWidget(m_cover_image_label);
#else
    mainLayout->addWidget(m_metadata_widget);
    mainLayout->addWidget(m_cover_image_label);
    mainWidget->setLayout(mainLayout);
#endif
    setWidget(mainWidget);

	// Connect up to the proxy model.  We won't have to disconnect/reconnect since we own this proxy model.
	connect_or_die(m_proxy_model, &SelectionFilterProxyModel::dataChanged, this, &MetadataDockWidget::onDataChanged);
	connect_or_die(m_proxy_model_watcher, &ModelChangeWatcher::modelHasRows, this, &MetadataDockWidget::onProxyModelChange);
}

void MetadataDockWidget::connectToView(MDITreeViewBase* view)
{
    if(view == nullptr)
    {
        qWarning() << "VIEW IS NULL";
        return;
    }

	if (view == m_connected_view)
	{
		qDebug() << "Already connected to this view.";
		return;
	}

	m_connected_view = view;

	qDebug() << "Setting new source model and selection model:" << view->model() << view->selectionModel();

	m_proxy_model->setSourceModel(view->model());
	m_proxy_model->setSelectionModel(view->selectionModel());
}

void MetadataDockWidget::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
{
	qDebug() << "Data changed:" << topLeft << bottomRight << roles;
	
	Q_ASSERT(topLeft.model() == m_proxy_model);

	if(topLeft.isValid())
	{
		// Update the QTreeWidget.
		QModelIndex mi = m_proxy_model->index(topLeft.row(), 0, QModelIndex());
		auto sp = m_proxy_model->data(mi, ModelUserRoles::PointerToItemRole).value<std::shared_ptr<LibraryEntry>>();
		qDebug() << "Pointer says:" << sp->getM2Url();
		PopulateTreeWidget(mi);
	}
}

void MetadataDockWidget::PopulateTreeWidget(const QModelIndex& first_model_index)
{
//	qDebug() << "Populating with: " << first_model_index;

	QModelIndex mi = m_proxy_model->index(first_model_index.row(), 0, QModelIndex());
	auto variant = m_proxy_model->data(mi, ModelUserRoles::PointerToItemRole);
//	qDebug() << "Variant is:" << variant;

//	qDebug() << variant.canConvert<std::shared_ptr<LibraryEntry>>();

	auto libentry = variant.value<std::shared_ptr<LibraryEntry>>();

	///qDebug() << "PLAYLIST ITEM: " << libentry;
	if(libentry)
	{
		// Get a copy of the metadata.
		Metadata md = libentry->metadata();
// M_TODO("Not getting some field here");
//		qDb() << "METADATA:" << md.toVariant();

		AMLMTagMap pimeta = libentry->getAllMetadata(); // QMap<QString, QVariant>
		///qDebug() << "PLAYLIST ITEM METADATA: " << pimeta;
		// clear out any old data we have.
        m_metadata_widget->clear();
		m_cover_image_label->clear();

		if(md)
		{
			// Pick out some fields directly from the Metadata object.

            // Top-level spanning title item for the "Metadata Types" section.
            QTreeWidgetItem* metadata_types = new QTreeWidgetItem({"Metadata Types", ""});
            m_metadata_widget->addTopLevelItem(metadata_types);
            metadata_types->setExpanded(true);
            metadata_types->setFirstColumnSpanned(true);
            /// @todo QT6 Change here, not sure if it was ever needed due to immediately above.
            m_metadata_widget->setFirstColumnSpanned(0, m_metadata_widget->indexFromItem(metadata_types).parent(), true);

            AMLMTagMap empty {};
            std::vector<std::tuple<QString, QVariant, AMLMTagMap>> md_list = {
				{"hasGeneric?", md.hasGeneric(), md.tagmap_generic()},
				{"hasID3v1?", md.hasID3v1(), md.tagmap_id3v1()},
				{"hasID3v2?", md.hasID3v2(), md.tagmap_id3v2()},
				{"hasAPE?", md.hasAPE(), md.tagmap_ape()},
				{"hasXiphComment?", md.hasXiphComment(), md.tagmap_xiph()},
				{"hasRIFFInfoTag?", md.hasRIFFInfo(), md.tagmap_RIFFInfo()},
				{"hasDiscCuesheet?", md.hasDiscCuesheet(), md.tagmap_cuesheet_disc()},
                {"CuesheetEmbedded?:", md.m_cuesheet_embedded.origin() == CueSheet::Embedded, empty},
				{"CuesheetSidecar?:", md.m_cuesheet_sidecar.origin() == CueSheet::Sidecar, empty},
            	{"hasCDTextFile?", QVariant::fromValue(md.m_cuesheet_combined.has_cdtext_file()), empty}
            	// {"Cuesheets equal?", (md.m_cuesheet_embedded == md.m_cuesheet_sidecar), empty},
					// {"CuesheetEmbedded?:", md.m_cuesheet_embedded.operator bool(), empty},
					// {"CuesheetSidecar?:", md.m_cuesheet_sidecar.operator bool(), empty},
#if 0 /// @todo
				{"hasTrackCuesheetInfo?", md.hasTrackCuesheet(), md.tagmap_cuesheet_track()}
#endif
			};

			// Add each of the tag type trees as a separate expanded subtree.
			for(const auto& e : md_list)
			{
				auto md_type_item = new QTreeWidgetItem({std::get<0>(e), std::get<1>(e).toString()});
				metadata_types->addChild(md_type_item);
				if(!std::get<2>(e).empty())
				{
					addChildrenFromAMLMTagMap(md_type_item, std::get<2>(e));
					md_type_item->setExpanded(true);
				}
			}

		}

		// Add the technical info.
		auto tech_info_tree = new QTreeWidgetItem({"Technical Info", ""});
		m_metadata_widget->addTopLevelItem(tech_info_tree);
		tech_info_tree->setExpanded(true);
		tech_info_tree->setFirstColumnSpanned(true);

		auto length = libentry->get_length_secs();
		std::vector<std::pair<QString, QVariant>> list = {
			{"Subtrack?", libentry->isSubtrack()},
			{"Track No.", libentry->getTrackNumber()},
			{"Track Total", libentry->getTrackTotal()},
			{"Pre-gap offset", QString::number(libentry->get_pre_gap_offset_secs())},
			{"Length", QString::number(libentry->get_length_secs())}
		};
		for(const auto& p: list)
		{
			tech_info_tree->addChild(new QTreeWidgetItem({p.first, p.second.toString()}));
			tech_info_tree->setExpanded(true);
		}

		/// Dump all the metadata.
		/// @todo Does this make any sense anymore? Just putting the entries into the tree here.
		pimeta.foreach_pair([&](QString key, QString val){
			m_metadata_widget->addTopLevelItem(new QTreeWidgetItem({key, val}));
		});
//		for(const auto& entry : pimeta)
//		{
//			QString key = toqstr(entry.first);
//			QStringList value = toqstr(entry.second);
//			if(value.empty())
//			{
//                m_metadata_widget->addTopLevelItem(new QTreeWidgetItem({key, value[0]}));
//			}
//		}

		//
		// Load and Display the cover image.
		//

		// Create the asynchronous Cover Art loader task.
		ExtFuture<QByteArray> coverart_future = CoverArtJob::make_task(this, libentry->getUrl());

		//qDb() << "CoverArtCallback: Adding to perfect deleter.";
		/// @todo Is there a race here? Should this be part of the make_, so we never see it if it gets deleted?
        AMLMApp::IPerfectDeleter().addQFuture(QFuture<void>(coverart_future));

		qDb() << "CoverArtCallback: Adding .then()..";
		coverart_future.then([=](ExtFuture<QByteArray> future) -> QImage
		{
			// Do as much as we can in the arbitrary non-GUI context we're called in.
/// @todo NEED TO FIX get() returning nothing
#if 0 // QT6
			if(future.hasException())
			{

			}
#endif
			QList<QByteArray> cover_image_bytes = future.results();
			QImage image;

			if(!future.isCanceled() && !cover_image_bytes.empty() && cover_image_bytes[0].size() > 0)
			{
				// Pic data load succeeded, see if we can get a valid QImage out of it.
				// QImage is safe to use in a non-GUI-thread context.

				if(image.loadFromData(cover_image_bytes[0]) == true)
				{
					// It was a valid image.
					qDb() << "Valid cover image loaded";
				}
				else
				{
					qWr() << "Error attempting to load image.";
					/// @todo Set error state on the QURL.
				}
			}
			return image;
		})
			/// @note Anything QPixmap needs to be in run the GUI thread.
			/// @todo I'm not clear on why we need to explicitly capture a copy of future...
			/// Oh wait, probably lambda isn't mutable.
// Qt6			ExtAsync::detail::run_in_event_loop(this, [=, future_copy=future](){
        .then(this, [this](QImage image){
			AMLM_ASSERT_IN_GUITHREAD();

			if(image.isNull())
			{
				// Error.  Load the "No image available" icon.
                qWr() << "ASYNC GetCoverArt FAILED"; // << kjob->error() << ":" << kjob->errorText() << ":" << kjob->errorString();
				QIcon no_pic_icon = Theme::iconFromTheme("image-missing");
				m_cover_image_label->setPixmap(no_pic_icon.pixmap(QSize(256,256)));
			}
			else
			{
				// Succeeded, convert QImage to QPixmap.
				qDb() << "Valid cover image found"; ///@todo << cover_image.mime_type;
				m_cover_image_label->setPixmap(QPixmap::fromImage(image));
				//m_cover_image_label.adjustSize()
				/// @todo Probably need to handle setPixmap() error.
            }
		});
	}
	else
	{
		qWarning() << "PLAYLIST ITEM IS INVALID";
	}

}

void MetadataDockWidget::addChildrenFromAMLMTagMap(QTreeWidgetItem* parent, const AMLMTagMap& tagmap)
{
	// Add all entries in tagmap to parent, with a few exceptions.
	tagmap.foreach_pair([=](QString key, QString value){
		if(!key.contains(QRegularExpression(R"!(^(REM\s+)?(CUESHEET|LOG|.*CTDB(TRACK|DISC)CONFIDENCE))!")))
		{
			auto child = new QTreeWidgetItem({key, value});
			parent->addChild(child);
		}
	});
}

void MetadataDockWidget::onProxyModelChange(bool has_rows)
{
//	qDebug() << "MODELWATCHER DETECTED CHANGE IN PROXY MODEL";

	if(has_rows)
	{
		// Update the tree widget.
		auto index = m_proxy_model->index(0, 0, QModelIndex());
		PopulateTreeWidget(index);
	}
	else
    {
        qDebug() << "NO ROWS";
    }
}

