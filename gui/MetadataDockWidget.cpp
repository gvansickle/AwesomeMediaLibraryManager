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

#include "MetadataDockWidget.h"

#include "PixmapLabel.h"

#include <QItemSelection>
#include <QTreeView>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QDebug>

#include <QDataWidgetMapper>
#include <QLineEdit>

#include <functional>

#include <logic/MetadataAbstractBase.h>
#include <logic/LibraryEntry.h>
#include <logic/LibraryModel.h>
#include <gui/MDITreeViewBase.h>
#include <utils/ModelHelpers.h>
#include <logic/LibrarySortFilterProxyModel.h>
#include <logic/proxymodels/EntryToMetadataTreeProxyModel.h>

#include <utils/Theme.h>

MetadataDockWidget::MetadataDockWidget(const QString& title, QWidget *parent, Qt::WindowFlags flags) : QDockWidget(title, parent, flags)
{
    setObjectName("MetadataDockWidget");

    // Set up the proxy model.
    m_proxy_model = new EntryToMetadataTreeProxyModel(this);

    // Main layout is vertical.
    auto mainLayout = new QVBoxLayout();

    m_metadata_tree_view = new QTreeView(this);
    m_metadata_tree_view->setModel(m_proxy_model);

    m_metadata_widget = new QTreeWidget(this);
    m_metadata_widget->setRootIsDecorated(false);
    m_metadata_widget->setColumnCount(2);
    m_metadata_widget->setHeaderLabels(QStringList() << "Key" << "Value");


    m_cover_image_label = new PixmapLabel(this);
    m_cover_image_label->setText("IMAGE HERE");

    mainLayout->addWidget(m_metadata_tree_view);
    mainLayout->addWidget(m_metadata_widget);
    mainLayout->addWidget(m_cover_image_label);
    auto mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);
}

void MetadataDockWidget::connectToView(MDITreeViewBase* view)
{
    if(view == nullptr)
    {
        qWarning() << "VIEW IS NULL";
        return;
    }

    if(m_proxy_model != nullptr)
    {
		disconnect(m_proxy_model, &EntryToMetadataTreeProxyModel::dataChanged, this, &MetadataDockWidget::onDataChanged);
    }

    qDebug() << "Setting new source model and selection model:" << view->model() << view->selectionModel();

    m_proxy_model->setSourceModel(view->model());
    m_proxy_model->setSelectionModel(view->selectionModel());

    connect(m_proxy_model, &EntryToMetadataTreeProxyModel::dataChanged, this, &MetadataDockWidget::onDataChanged);
}

void MetadataDockWidget::viewSelectionChanged(const QItemSelection& newSelection, const QItemSelection& /*oldSelection*/)
{
    qDebug() << "Selection changed: " << newSelection;

    if(newSelection.size() > 0)
    {
        // Get the top-level source selection.
        QItemSelection tlis = ::mapSelectionToSource(newSelection);

        // Extract the first source index.
        QModelIndex fsi = tlis.indexes()[0];

        // Tell the filter model about the new selection.
        m_proxy_model->setSourceIndexToShow(fsi);

//		PopulateTreeWidget(fsi);
    }
    else
    {
        qDebug() << "selection was empty";

        m_proxy_model->setSourceIndexToShow(QModelIndex());
//		PopulateTreeWidget(QModelIndex());
    }
}

void MetadataDockWidget::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	qDebug() << "Data changed:" << topLeft << bottomRight << roles;
	
	Q_ASSERT(topLeft.model() == m_proxy_model);

	// Map the index to the top-level source model.
	QModelIndex source_model_index = m_proxy_model->mapToSource(topLeft);
	
//	PopulateTreeWidget(source_model_index);
}

void MetadataDockWidget::PopulateTreeWidget(const QModelIndex& first_model_index)
{
	qDebug() << "Populating with: " << first_model_index;

	///qDebug() << "Incoming model:" << first_model_index.model();
	const LibrarySortFilterProxyModel* model = dynamic_cast<const LibrarySortFilterProxyModel*>(first_model_index.model());
	if(model == nullptr)
	{
		qCritical() << "Null model. first_model_index.isValid?:" << first_model_index.isValid();
	}
	auto selected_row = first_model_index.row();

	///qDebug() << "Selected Row: " << selected_row;
	//return
	std::shared_ptr<LibraryEntry> libentry = model->getItem(first_model_index);
	///qDebug() << "PLAYLIST ITEM: " << libentry;
	if(libentry != nullptr)
	{
		// Get a copy of the metadata.
		Metadata md = libentry->metadata();

		std::map<QString, QVariant> pimeta = libentry->getAllMetadata().toStdMap(); // QMap<QString, QVariant>
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
            m_metadata_widget->setFirstItemColumnSpanned(metadata_types, true);

            std::vector<std::tuple<QString, QVariant, TagMap>> md_list = {
				{"hasVorbisComments?", md.hasVorbisComments(), md.tagmap_VorbisComments()},
				{"hasID3v1?", md.hasID3v1(), md.tagmap_id3v1()},
				{"hasID3v2?", md.hasID3v2(), md.tagmap_id3v2()},
				{"hasAPE?", md.hasAPE(), TagMap()},
				{"hasXiphComment?", md.hasXiphComment(), md.tagmap_xiph()},
				{"hasInfoTag?", md.hasInfoTag(), md.tagmap_InfoTag()}
			};

			for(auto e : md_list)
			{
				auto md_type_item = new QTreeWidgetItem({std::get<0>(e), std::get<1>(e).toString()});
				metadata_types->addChild(md_type_item);
				if(!std::get<2>(e).empty())
				{
					addChildrenFromTagMap(md_type_item, std::get<2>(e));
					md_type_item->setExpanded(true);
				}
			}

		}

		auto length = libentry->get_length_secs();
		std::vector<std::pair<QString, QVariant>> list = {
			{"Subtrack?", libentry->isSubtrack()},
			{"Track No.", libentry->getTrackNumber()},
			{"Track Total", libentry->getTrackTotal()},
			{"Pre-gap offset", libentry->get_pre_gap_offset_secs().toQString()},
			{"Length", libentry->get_length_secs().toQString()}
		};
		for(auto p: list)
		{
            m_metadata_widget->addTopLevelItem(new QTreeWidgetItem({p.first, p.second.toString()}));
		}

		/// Dump all the metadata.
		for(auto entry = pimeta.begin(); entry != pimeta.end(); ++entry)
		{
			QString key = entry->first;
			QStringList value = entry->second.value<QStringList>();
			if(value.size() > 0)
			{
                m_metadata_widget->addTopLevelItem(new QTreeWidgetItem({key, value[0]}));
			}
		}

		// Display the cover image.
		auto cover_image_bytes = libentry->getCoverImageBytes();
		if(cover_image_bytes.size() != 0)
		{
			qDebug("Cover image found"); ///@todo << cover_image.mime_type;
			QImage image;
			if(image.loadFromData(cover_image_bytes) == true)
			{
				///qDebug() << "Image:" << image;
				m_cover_image_label->setPixmap(QPixmap::fromImage(image));
				//m_cover_image_label.adjustSize()
			}
			else
			{
				qWarning() << "Error attempting to load image.";
				QIcon no_pic_icon = Theme::iconFromTheme("image-missing");
				m_cover_image_label->setPixmap(no_pic_icon.pixmap(QSize(256,256)));
			}
		}
		else
		{
			// No image available.
			QIcon no_pic_icon = Theme::iconFromTheme("image-missing");
			m_cover_image_label->setPixmap(no_pic_icon.pixmap(QSize(256,256)));
		}
	}
	else
	{
		qWarning() << "PLAYLIST ITEM IS INVALID";
	}
}

void MetadataDockWidget::addChildrenFromTagMap(QTreeWidgetItem* parent, const TagMap& tagmap)
{
	for(auto e : tagmap)
	{
		QString key = QString::fromUtf8(e.first.c_str());
        // Filter out keys we don't want to see in the Metadata display.
        // Mainly this is CUESHEET and LOG entries in FLAC VORBIS_COMMENT blocks, which are gigantic and destroy the
        // formatting.
        ///@todo Maybe also filter on size of values.
        if(key.contains(QRegularExpression(R"!(CUESHEET|LOG|CTDBTRACKCONFIDENCE)!")))
        {
            continue;
        }
		for(auto f : e.second)
		{
			QString value = QString::fromUtf8(f.c_str());
			auto child = new QTreeWidgetItem({key, value});
			parent->addChild(child);
		}
	}
}

