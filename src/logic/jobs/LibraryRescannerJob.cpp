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

#include "LibraryEntry.h"
#include <memory>
#include <functional>

// Qt5
#include <QObject>
#include <QtConcurrent>

// Ours
#include <utils/DebugHelpers.h>
#include <concurrency/ExtAsync.h>
#include <jobs/LibraryRescannerJob.h>
#include <utils/TheSimplestThings.h>
#include "LibraryModel.h"

#if 0

LibraryRescannerJob::LibraryRescannerJob(QObject* parent) : AMLMJobT(parent)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

	/// @todo This should be coming through the ExtFuture.
	setProgressUnit(KJob::Unit::Files);

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable | KJob::Capability::Suspendable);
}

LibraryRescannerJobPtr LibraryRescannerJob::make_job(QObject *parent)
{
    auto retval = new LibraryRescannerJob(parent);
    /// @todo Hook things up in here.

    return retval;
}

LibraryRescannerJobPtr LibraryRescannerJob::make_job(QObject *parent, LibraryRescannerMapItem item_to_refresh,
                                                     const LibraryModel* current_libmodel)
{
    auto retval = new LibraryRescannerJob(parent);

    QVector<VecLibRescannerMapItems> vec_vec_items_to_refresh;
    VecLibRescannerMapItems vec_items_to_refresh;
    vec_items_to_refresh.push_back(item_to_refresh);
    vec_vec_items_to_refresh.push_back(vec_items_to_refresh);

    retval->setDataToMap(vec_vec_items_to_refresh, current_libmodel);

    return retval;
}

void LibraryRescannerJob::setDataToMap(QVector<VecLibRescannerMapItems> items_to_rescan,
                                                        const LibraryModel* current_libmodel)
{
    m_items_to_rescan = items_to_rescan;
    m_current_libmodel = current_libmodel;
}

#endif


MetadataReturnVal refresher_callback(const VecLibRescannerMapItems &mapitem)
{
	qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	MetadataReturnVal retval;

	// If we have more than a single entry in the incoming list, we have a multi-track file to refresh.
	if(mapitem.size() == 1)
	{
		// Only one entry.

		// Get the LibraryEntry* to the existing entry.
M_WARNING("There's no locking here, there needs to be, or these need to be copies.");
		std::shared_ptr<LibraryEntry> item = mapitem[0].item;

		if(!item->isPopulated())
		{
			// Item's metadata has not been looked at.  We may have multiple tracks.

			// Only one pindex though.
			retval.m_original_pindexes.push_back(mapitem[0].pindex);

			item->populate();
			auto vec_items = item->split_to_tracks();
			for (const auto& i : vec_items)
			{
				if (!i->isPopulated())
				{
					qCritical() << "NOT POPULATED" << i.get();
				}
				retval.push_back(i);

//                qDb() << "LIBENTRY METADATA:" << i->getAllMetadata();

			}
		}
		else if (item->isPopulated() && item->isSubtrack())
		{
			qCr() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << item->getUrl();
			Q_ASSERT(0);
		}
		else
		{
			qDb() << "Re-reading metatdata for item" << item->getUrl();
			item->refresh_metadata();

			if(item->isError())
			{
				// Couldn't load the metadata from the file.
				// Only option here is to return the old item, which should now be marked with an error.
				qCritical() << "Couldn't load metadata for file" << item->getUrl();
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(item);
				retval.m_num_tracks_found = 1;
			}
			else
			{
				// Repackage it and return.
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(item);
				retval.m_num_tracks_found = 1;
			}
		}
	}
	else if (mapitem.size() > 1)
	{
		// Multiple incoming tracks.
		std::shared_ptr<LibraryEntry> first_item = mapitem[0].item;
		first_item->populate(true);
		auto subtracks = first_item->split_to_tracks();
		if(subtracks.size() < mapitem.size())
		{
			// We got fewer back than we had before.
			if(subtracks.size() == 1)
			{
				// We only got one back.  This means we weren't able to read the file.
				return retval;
			}
			else
			{
				// Not sure what exactly we got back.
				qCritical() << "Unknown response: mapitem.size()==" << mapitem.size() << "subtracks.size()==" << subtracks.size();
				Q_ASSERT(0);
			}
		}
		for(int i=0; i<mapitem.size(); ++i)
		{
			Q_ASSERT(subtracks[i] != nullptr);
			retval.push_back(mapitem[i].pindex, subtracks[i]);
		}
	}
	else
	{
		qCritical() << "GOT EMPTY LIST OF LIBRARY ENTRIES TO RESCAN";
	}

	return retval;
}

void library_metadata_rescan_task(ExtFuture<MetadataReturnVal> ext_future, AMLMJob* /*the_job*/,
									ExtFuture<VecLibRescannerMapItems> in_future,
//								  QVector<VecLibRescannerMapItems> items_to_rescan,
								  LibraryModel* current_libmodel)
{
	qDb() << "ENTER library_metadata_rescan_task";

	// For now we'll count progress in terms of files scanned.
	// Might want to change to tracks eventually.
	ext_future.setProgressUnit(KJob::Unit::Files);

	// Send out progress text.
	QString status_text = QObject::tr("Refreshing metadata");
	ext_future.reportDescription(status_text,
                                QPair<QString,QString>(QObject::tr("Root URL"), ""),
                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

	// Wait for the work to come in.
	QList<VecLibRescannerMapItems> items_to_rescan = in_future.get();

	/// @todo
	//setTotalAmountAndSize(KJob::Unit::Files, m_items_to_rescan.size());

	ext_future.setProgressRange(0, items_to_rescan.size());
	ext_future.setProgressValueAndText(0, status_text);

	qulonglong num_items = 0;
	for(QList<VecLibRescannerMapItems>::const_iterator i = items_to_rescan.cbegin(); i != items_to_rescan.cend(); ++i)
	{
		qDb() << "Item number:" << num_items;

		/// @todo eliminate the_job ptr.
		MetadataReturnVal a = /*the_job->*/refresher_callback(*i);

		// Report the new results to The Future.
		ext_future.reportResult(a);

		num_items++;

		/// @todo
//		setProcessedAmountAndSize(KJob::Unit::Files, num_items);
		/// @note New, temp.
		ext_future.setProgressValue(num_items);

		if(ext_future.HandlePauseResumeShouldICancel())
		{
			// We've been cancelled.
			qIn() << "CANCELLED";
			break;
		}
	}

	// And we're done.
	ext_future.reportFinished();
}

#if 0
MetadataReturnVal LibraryRescannerJob::refresher_callback(const VecLibRescannerMapItems &mapitem)
{
    qDebug() << "Current thread:" << QThread::currentThread()->objectName();

    MetadataReturnVal retval;

    // If we have more than a single entry in the incoming list, we have a multi-track file to refresh.
    if(mapitem.size() == 1)
    {
        // Only one entry.

        // Get the LibraryEntry* to the existing entry.
M_WARNING("There's no locking here, there needs to be, or these need to be copies.");
        std::shared_ptr<LibraryEntry> item = mapitem[0].item;

        if(!item->isPopulated())
        {
            // Item's metadata has not been looked at.  We may have multiple tracks.

            // Only one pindex though.
            retval.m_original_pindexes.push_back(mapitem[0].pindex);

			item->populate();
			auto vec_items = item->split_to_tracks();
			for (const auto& i : vec_items)
            {
                if (!i->isPopulated())
                {
                    qCritical() << "NOT POPULATED" << i.get();
                }
                retval.push_back(i);

//                qDb() << "LIBENTRY METADATA:" << i->getAllMetadata();

            }
        }
        else if (item->isPopulated() && item->isSubtrack())
        {
			qCr() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << item->getUrl();
            Q_ASSERT(0);
        }
        else
        {
			qDb() << "Re-reading metatdata for item" << item->getUrl();
            item->refresh_metadata();

            if(item->isError())
            {
                // Couldn't load the metadata from the file.
                // Only option here is to return the old item, which should now be marked with an error.
                qCritical() << "Couldn't load metadata for file" << item->getUrl();
                retval.m_original_pindexes.push_back(mapitem[0].pindex);
                retval.m_new_libentries.push_back(item);
                retval.m_num_tracks_found = 1;
            }
            else
            {
                // Repackage it and return.
                retval.m_original_pindexes.push_back(mapitem[0].pindex);
                retval.m_new_libentries.push_back(item);
                retval.m_num_tracks_found = 1;
            }
        }
    }
    else if (mapitem.size() > 1)
    {
        // Multiple incoming tracks.
        std::shared_ptr<LibraryEntry> first_item = mapitem[0].item;
        first_item->populate(true);
		auto subtracks = first_item->split_to_tracks();
        if(subtracks.size() < mapitem.size())
        {
            // We got fewer back than we had before.
            if(subtracks.size() == 1)
            {
                // We only got one back.  This means we weren't able to read the file.
                return retval;
            }
            else
            {
                // Not sure what exactly we got back.
                qCritical() << "Unknown response: mapitem.size()==" << mapitem.size() << "subtracks.size()==" << subtracks.size();
                Q_ASSERT(0);
            }
        }
        for(int i=0; i<mapitem.size(); ++i)
        {
            Q_ASSERT(subtracks[i] != nullptr);
            retval.push_back(mapitem[i].pindex, subtracks[i]);
        }
    }
    else
    {
        qCritical() << "GOT EMPTY LIST OF LIBRARY ENTRIES TO RESCAN";
    }

    return retval;
}
#endif

