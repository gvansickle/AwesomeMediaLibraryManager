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

#include <config.h>

#include "LibraryEntryLoaderJob.h"

#include <AMLMApp.h>
#include "LibraryRescanner.h"
#include <utils/RegisterQtMetatypes.h>
#include <utils/DebugHelpers.h>

AMLM_QREG_CALLBACK([](){
    qIn() << "Registering LibraryEntryLoaderJob types";
    qRegisterMetaType<LibraryEntryLoaderJobResult>();
    qRegisterMetaType<ExtFuture<LibraryEntryLoaderJobResult>>();
    });

/// Stream operations for LibraryEntryLoaderJobResults
/// @{
#if 0
#define DATASTREAM_FIELDS(X) \
    X(m_original_pindex) X(m_original_libentry) X(m_new_libentries) X(m_num_tracks_found)


QDebug operator<<(QDebug dbg, const LibraryEntryLoaderJobResult & obj)
{
#define X(field) << obj.field
    dbg DATASTREAM_FIELDS(X);
#undef X
    return dbg;
}

QDataStream &operator<<(QDataStream &out, const LibraryEntryLoaderJobResult & myObj)
{
#define X(field) << myObj.field
    out DATASTREAM_FIELDS(X);
#undef X
    return out;
}

QDataStream &operator>>(QDataStream &in, LibraryEntryLoaderJobResult & myObj)
{
#define X(field) >> myObj.field
    return in DATASTREAM_FIELDS(X);
#undef X
}
#endif
/// @}

LibraryEntryLoaderJobPtr LibraryEntryLoaderJob::make_job(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
{
    auto retval = new LibraryEntryLoaderJob(parent, pmi, libentry);

    /// @todo Hook things up in here.

    return retval;
}

LibraryEntryLoaderJobPtr LibraryEntryLoaderJob::make_job(QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
{
    return make_job(AMLMApp::instance(), pmi, libentry);
}

LibraryEntryLoaderJob::LibraryEntryLoaderJob(QObject *parent, QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
    : BASE_CLASS(parent), m_pmi(pmi), m_libentry(libentry)
{
    // Set our object name.
    setObjectName(uniqueQObjectName());

    setProgressUnit(KJob::Unit::Files);

    // Set our capabilities.
    setCapabilities(KJob::Capability::Killable /*| KJob::Capability::Suspendable*/);
}

LibraryEntryLoaderJob::~LibraryEntryLoaderJob()
{

}

void LibraryEntryLoaderJob::LoadEntry(ExtFuture<LibraryEntryLoaderJobResult> ext_future, LibraryEntryLoaderJob* kjob,
									  QPersistentModelIndex pmi, std::shared_ptr<LibraryEntry> libentry)
{
	qDb() << "START LibraryEntryLoaderJob LoadEntry" << pmi << libentry;

	LibraryEntryLoaderJobResult retval(pmi, libentry);

    Q_ASSERT(retval.isValid());

    // Make sure the index is still valid.  The model may have been destroyed since the message was sent.
	if(!pmi.isValid())
    {
		qWr() << "INVALID QPersistentModelIndex:" << pmi << ", ABORTING LOAD";
		/// @todo
//        setError(InvalidQPersistentModelIndex);
        return;
    }
    // Make sure the LibraryEntry hasn't been deleted.  It shouldn't have been since we hold a shared_ptr<> to it.
	Q_ASSERT(libentry);

    // Make sure the LibraryEntry has a valid QUrl.  It should, but ATM we're getting here with empty URLs.
//    AMLM_ASSERT_EQ(m_libentry->getUrl().isValid(), true);
	if(!libentry->getUrl().isValid())
    {
		qWr() << "INVALID URL";
		/// @todo
//        setError(InvalidLibraryEntryURL);
        return;
    }

	if(!libentry->isPopulated())
    {
        // Item's metadata has not been looked at.  We may have multiple tracks.

		qIn() << "LOADING ITEM:" << libentry;
		auto vec_items = libentry->populate();
        for (const auto& i : vec_items)
        {
            if (!i->isPopulated())
            {
				qCr() << "NOT POPULATED" << i.get();
            }
            retval.push_back(i);

			qDb() << "LIBENTRY METADATA:" << i->getAllMetadata();

        }
    }
	else if (libentry->isPopulated() && libentry->isSubtrack())
    {
		qCr() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << libentry->getUrl();
        Q_ASSERT(0);
    }
    else
    {
        // Item needs to be refreshed.

        //qDebug() << "Re-reading metatdata for item" << item->getUrl();
		std::shared_ptr<LibraryEntry> new_entry = libentry->refresh_metadata();

        if(!new_entry)
        {
            // Couldn't load the metadata from the file.
            // Only option here is to return the old item, which should now be marked with an error.
			qCr() << "Couldn't load metadata for file" << libentry->getUrl();
//            retval.m_original_pindexes.push_back(m_pmi);
			retval.m_new_libentries.push_back(libentry);
            retval.m_num_tracks_found = 1;
        }
        else
        {
            // Repackage it and return.
//            retval.m_original_pindexes.push_back(m_pmi);
            retval.m_new_libentries.push_back(new_entry);
            retval.m_num_tracks_found = 1;
        }
    }

    Q_ASSERT(retval.isValid());

    Q_ASSERT(retval.m_num_tracks_found > 0);

	ext_future.reportResult(retval);
	ext_future.reportFinished();
}

void LibraryEntryLoaderJob::runFunctor()
{
    qDbo() << "START LibraryEntryLoaderJob RUNFUNCTOR" << m_pmi << m_libentry;

    LibraryEntryLoaderJobResult retval(m_pmi, m_libentry);

    Q_ASSERT(retval.isValid());

    // Make sure the index is still valid.  The model may have been destroyed since the message was sent.
    if(!m_pmi.isValid())
    {
        qWro() << "INVALID QPersistentModelIndex:" << m_pmi << ", ABORTING LOAD";
        setError(InvalidQPersistentModelIndex);
        return;
    }
    // Make sure the LibraryEntry hasn't been deleted.  It shouldn't have been since we hold a shared_ptr<> to it.
    Q_ASSERT(m_libentry);

    // Make sure the LibraryEntry has a valid QUrl.  It should, but ATM we're getting here with empty URLs.
//    AMLM_ASSERT_EQ(m_libentry->getUrl().isValid(), true);
    if(!m_libentry->getUrl().isValid())
    {
        qWro() << "INVALID URL";
        setError(InvalidLibraryEntryURL);
        return;
    }

    if(!m_libentry->isPopulated())
    {
        // Item's metadata has not been looked at.  We may have multiple tracks.

        qIno() << "LOADING ITEM:" << m_libentry;
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
        // Item needs to be refreshed.

        //qDebug() << "Re-reading metatdata for item" << item->getUrl();
        std::shared_ptr<LibraryEntry> new_entry = m_libentry->refresh_metadata();

        if(!new_entry)
        {
            // Couldn't load the metadata from the file.
            // Only option here is to return the old item, which should now be marked with an error.
            qCro() << "Couldn't load metadata for file" << m_libentry->getUrl();
//            retval.m_original_pindexes.push_back(m_pmi);
            retval.m_new_libentries.push_back(m_libentry);
            retval.m_num_tracks_found = 1;
        }
        else
        {
            // Repackage it and return.
//            retval.m_original_pindexes.push_back(m_pmi);
            retval.m_new_libentries.push_back(new_entry);
            retval.m_num_tracks_found = 1;
        }
    }

    Q_ASSERT(retval.isValid());

    Q_ASSERT(retval.m_num_tracks_found > 0);

    m_ext_future.reportResult(retval);
}
