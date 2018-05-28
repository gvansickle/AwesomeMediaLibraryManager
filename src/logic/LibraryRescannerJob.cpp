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
#include "LibraryRescannerJob.h"

/// Std C++
#include <memory>
#include <functional>
using std::placeholders::_1;


/// Qt5
#include <QtConcurrent>

/// Ours
#include <concurrency/ExtAsync.h>
#include <utils/TheSimplestThings.h>
#include "LibraryModel.h"

LibraryRescannerJob::LibraryRescannerJob(QObject* parent) : AMLMJob(parent)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable /*| KJob::Capability::Suspendable*/);
}

LibraryRescannerJob::~LibraryRescannerJob()
{
    qDb() << "LibraryRescannerJob DELETED:" << this << objectName();
}

void LibraryRescannerJob::setDataToMap(QVector<VecLibRescannerMapItems> items_to_rescan,
                                                         LibraryModel* current_libmodel)
{
    m_items_to_rescan = items_to_rescan;
    m_current_libmodel = current_libmodel;
}

void LibraryRescannerJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    qDb() << "ENTER run";

    AMLMJobPtr amlm_self = qSharedPtrToQPointerDynamicCast<AMLMJob>(self);

    setProgressUnit(KJob::Unit::Files);

    // Send out progress text.
    QString status_text = tr("Rereading metadata");
    Q_EMIT amlm_self->description(this, status_text);//,
//                                QPair<QString,QString>(QObject::tr("Root URL"), m_dir_url.toString()),
//                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

    setTotalAmount(KJob::Unit::Files, m_items_to_rescan.size());

    qulonglong num_items = 0;
    for(QVector<VecLibRescannerMapItems>::const_iterator i = m_items_to_rescan.cbegin(); i != m_items_to_rescan.cend(); ++i)
    {
        if(twWasCancelRequested())
        {
            // We were told to cancel.
            break;
        }

        qDb() << "Item number:" << num_items;
        MetadataReturnVal a = this->refresher_callback(*i);
        this->processReadyResults(a);
        num_items++;

        setProcessedAmount(KJob::Unit::Files, num_items);
    }

    // We've either completed our work or been cancelled.
    // Either way, defaultEnd() will handle setting the cancellation status as long as
    // we set success/fail appropriately.
//    if(twWasCancelRequested())
//    {
//        // Cancelled.
//        // Success == false is correct here.
//        amlm_self->setSuccessFlag(false);
//        amlm_self->setWasCancelled(true);
//    }
//    else
//    {
//        // Successful completion.
//        qDb() << "METADATA RESCAN COMPLETE";
//        amlm_self->setSuccessFlag(true);
//    }

}

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

            auto vec_items = item->populate();
            for (auto i : vec_items)
            {
                if (!i->isPopulated())
                {
                    qCritical() << "NOT POPULATED" << i.get();
                }
                retval.push_back(i);
            }
        }
        else if (item->isPopulated() && item->isSubtrack())
        {
            qCritical() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << item->getUrl();
            Q_ASSERT(0);
        }
        else
        {
            //qDebug() << "Re-reading metatdata for item" << item->getUrl();
            std::shared_ptr<LibraryEntry> new_entry = item->refresh_metadata();

            if(new_entry == nullptr)
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
                retval.m_new_libentries.push_back(new_entry);
                retval.m_num_tracks_found = 1;
            }
        }
    }
    else if (mapitem.size() > 1)
    {
        // Multiple incoming tracks.
        std::shared_ptr<LibraryEntry> first_item = mapitem[0].item;
        auto subtracks = first_item->populate(true);
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

void LibraryRescannerJob::processReadyResults(MetadataReturnVal lritem_vec)
{
    // We got one of ??? things back:
    // - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
    // - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
    // - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

    if(lritem_vec.m_num_tracks_found == 0)
    {
        qCritical() << "RESULT WAS EMPTY";
    }

    if(lritem_vec.m_num_tracks_found > 1
       && lritem_vec.m_original_pindexes.size() == 1
            && lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found)
    {
        // It's a valid, new, multi-track entry.
        m_current_libmodel->onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
    }
    else if(lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found
            && lritem_vec.m_original_pindexes.size() == lritem_vec.m_num_tracks_found)
    {
        // It's a matching set of pindexes and libentries.

        for(int i=0; i<lritem_vec.m_num_tracks_found; ++i)
        {
            if (!lritem_vec.m_original_pindexes[i].isValid())
            {
                qWarning() << "Invalid persistent index, ignoring update";
                return;
            }

            // None of the returned entries should be null.
            Q_ASSERT(lritem_vec.m_new_libentries[i] != nullptr);

            qDebug() << "Replacing entry"; // << item->getUrl();
            // item is a single song which has its metadata populated.
            // Reconstruct the QModelIndex we sent out.
            auto initial_row_index = QModelIndex(lritem_vec.m_original_pindexes[i]);
            auto row = initial_row_index.row();
            qDebug() << QString("incoming single item, row %1").arg(row);
            // Metadata's been populated.
            m_current_libmodel->setData(initial_row_index, QVariant::fromValue(lritem_vec.m_new_libentries[i]));
        }
    }
    else
    {
        // Not sure what we got.
        qCritical() << "pindexes/libentries/num_new_entries:" << lritem_vec.m_original_pindexes.size()
                                                              << lritem_vec.m_new_libentries.size();
                                                              // lritem_vec.m_new_libentries;
        Q_ASSERT_X(0, "Scanning", "Not sure what we got");
    }
}
