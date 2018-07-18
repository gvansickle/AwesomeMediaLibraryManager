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

#include "LibraryEntryLoaderJob.h"

#include "LibraryRescanner.h"
#include <utils/DebugHelpers.h>

LibraryEntryLoaderJobPtr LibraryEntryLoaderJob::make_job(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
{
    auto retval = new LibraryEntryLoaderJob(parent, pmi, libentry);

    /// @todo Hook things up in here.

    return retval;
}

LibraryEntryLoaderJob::LibraryEntryLoaderJob(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
    : BASE_CLASS(parent), m_pmi(pmi), m_libentry(libentry)
{

}

LibraryEntryLoaderJob::~LibraryEntryLoaderJob()
{

}

void LibraryEntryLoaderJob::runFunctor()
{
    qDbo() << "START RUNFUNCTOR" << m_pmi << m_libentry;

    MetadataReturnVal retval;

    // Make sure the index is still valid.  The model may have been destroyed since the message was sent.
    if(!m_pmi.isValid())
    {
        qWro() << "INVALID QPersistentModelIndex:" << m_pmi << ", ABORTING LOAD";
        return;
    }

    if(!m_libentry->isPopulated())
    {
        // Item's metadata has not been looked at.  We may have multiple tracks.

        // Only one pindex though.
        retval.m_original_pindexes.push_back(m_pmi);

        auto vec_items = m_libentry->populate();
        for (const auto& i : vec_items)
        {
            if (!i->isPopulated())
            {
                qCro() << "NOT POPULATED" << i.get();
            }
            retval.push_back(i);

            qDbo() << "LIBENTRY METADATA:" << i->getAllMetadata();

        }
    }
    else if (m_libentry->isPopulated() && m_libentry->isSubtrack())
    {
        qCro() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << m_libentry->getUrl();
        Q_ASSERT(0);
    }
    else
    {
        //qDebug() << "Re-reading metatdata for item" << item->getUrl();
        std::shared_ptr<LibraryEntry> new_entry = m_libentry->refresh_metadata();

        if(new_entry == nullptr)
        {
            // Couldn't load the metadata from the file.
            // Only option here is to return the old item, which should now be marked with an error.
            qCritical() << "Couldn't load metadata for file" << m_libentry->getUrl();
            retval.m_original_pindexes.push_back(m_pmi);
            retval.m_new_libentries.push_back(m_libentry);
            retval.m_num_tracks_found = 1;
        }
        else
        {
            // Repackage it and return.
            retval.m_original_pindexes.push_back(m_pmi);
            retval.m_new_libentries.push_back(new_entry);
            retval.m_num_tracks_found = 1;
        }
    }

    m_ext_future.reportResult(retval);
}

#include "moc_LibraryEntryLoaderJob.cpp"
