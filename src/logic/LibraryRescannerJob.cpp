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

// Std C++
#include <memory>
#include <functional>

// Qt5
#include <QtConcurrent>

// Ours
#include <concurrency/ExtAsync.h>
#include <utils/TheSimplestThings.h>
#include "LibraryModel.h"


LibraryRescannerJob::LibraryRescannerJob(QObject* parent) : AMLMJob(parent)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable | KJob::Capability::Suspendable);
}

LibraryRescannerJob::~LibraryRescannerJob()
{
    qDb() << "LibraryRescannerJob DELETED:" << this << objectName();
}

LibraryRescannerJobPtr LibraryRescannerJob::make_job(QObject *parent)
{
    auto retval = new LibraryRescannerJob(parent);
    /// @todo Hook things up in here.

    return retval;
}

void LibraryRescannerJob::setDataToMap(QVector<VecLibRescannerMapItems> items_to_rescan,
                                                         LibraryModel* current_libmodel)
{
    m_items_to_rescan = items_to_rescan;
    m_current_libmodel = current_libmodel;
}

void LibraryRescannerJob::runFunctor()
{
    qDb() << "ENTER run";

    setProgressUnit(KJob::Unit::Files);

    // Send out progress text.
    QString status_text = tr("Rereading metadata");
    Q_EMIT description(this, status_text);//,
//                                QPair<QString,QString>(QObject::tr("Root URL"), m_dir_url.toString()),
//                                QPair<QString,QString>(QObject::tr("Current file"), QObject::tr("")));

    setTotalAmountAndSize(KJob::Unit::Files, m_items_to_rescan.size());

    // Make the internal connection to the SLOT_processReadyResults() slot.
    connect(this, &LibraryRescannerJob::processReadyResults, m_current_libmodel, &LibraryModel::SLOT_processReadyResults);

    qulonglong num_items = 0;
    for(QVector<VecLibRescannerMapItems>::const_iterator i = m_items_to_rescan.cbegin(); i != m_items_to_rescan.cend(); ++i)
    {
        qDb() << "Item number:" << num_items;
        MetadataReturnVal a = this->refresher_callback(*i);
        Q_EMIT processReadyResults(a);
        num_items++;

        setProcessedAmountAndSize(KJob::Unit::Files, num_items);

        if(functorHandlePauseResumeAndCancel())
        {
            // We've been cancelled.
            qIno() << "CANCELLED";
            m_ext_future.reportCanceled();
            break;
        }
    }

    // We've either completed our work or been cancelled.
    // Either way, defaultEnd() will handle setting the cancellation status as long as
    // we set success/fail appropriately.
    if(!wasCancelRequested())
    {
    	setSuccessFlag(true);
    }

    /// @todo push down
    m_ext_future.reportFinished();
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

                qDb() << "LIBENTRY METADATA:" << i->getAllMetadata();

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


