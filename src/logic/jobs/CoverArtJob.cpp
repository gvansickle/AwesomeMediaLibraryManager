/*
 * Copyright 2018, 2024 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "CoverArtJob.h"


/// TagLib includes.
#if 1
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/wavfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#else
#include <tag.h>
#include <fileref.h>
#include <tpropertymap.h>
#include <audioproperties.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <wavfile.h>
#include <attachedpictureframe.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <apetag.h>
#include <flacfile.h>
#include <flacpicture.h>
#endif

/// Ours
#include "TagLibHelpers.h"

CoverArtJob::CoverArtJob(QObject* parent, const QUrl &url) : BASE_CLASS(parent), m_audio_file_url(url)
{
	// Set our capabilities.
	setCapabilities(KJob::Capability::Killable);
}

CoverArtJob::~CoverArtJob()
{
}

CoverArtJobPtr CoverArtJob::make_job(QObject *parent, const QUrl& url)
{
    auto retval = new CoverArtJob(parent, url);

	return retval;
}

ExtFuture<QByteArray> CoverArtJob::make_task(QObject* parent, const QUrl& url)
{
#if 0
	ExtFuture<QByteArray> ret_future;

	QtConcurrent::run(&CoverArtJob::LoadCoverArt, ret_future, nullptr, url);

	return ret_future;
#endif

    return QtConcurrent::run(&CoverArtJob::LoadCoverArt, nullptr, url);
}

///
/// Mostly copy/paste from QByteArray MetadataTaglib::getCoverArtBytes() const and company.
///

static QByteArray getCoverArtBytes_ID3(TagLib::ID3v2::Tag* tag)
{
    const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
    if (!frameList.isEmpty())
    {
        qDebug() << "Found" << frameList.size() << "embedded pictures.";
		const auto* frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
        return QByteArray(frame->picture().data(), frame->picture().size());
    }

    return QByteArray();
}

static QByteArray getCoverArtBytes_APE(TagLib::APE::Tag* tag)
{
    const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
    if (listMap.contains("COVER ART (FRONT)"))
    {
        const TagLib::ByteVector nullStringTerminator(1, 0);
        TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
        const int pos = item.find(nullStringTerminator);	// Skip the filename.
        if (pos != -1)
        {
            const TagLib::ByteVector& pic = item.mid(pos + 1);
            return QByteArray(pic.data(), pic.size());
        }
    }

    return QByteArray();
}

static QByteArray getCoverArtBytes_FLAC(TagLib::FLAC::File* file)
{
    const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
    if (!picList.isEmpty())
    {
        // Just grab the first image.
        const TagLib::FLAC::Picture* pic = picList[0];
        // Create a copy of the bytes.  ::fromRawData() doesn't do that.
        return QByteArray(pic->data().data(), pic->data().size());
    }

    return QByteArray();
}

void CoverArtJob::LoadCoverArt(QPromise<QByteArray>& promise, CoverArtJobPtr kjob, const QUrl& url)
{
	// Mostly copy/paste from QByteArray MetadataTaglib::getCoverArtBytes() const

	// The byte array containing the cover art.
	QByteArray retval;

	// Open the file ref.
	QString url_as_local = url.toLocalFile();

	TagLib::FileRef fr {openFileRef(url_as_local)};
	if(fr.isNull())
	{
		qWarning() << "Unable to open file" << url_as_local << "with TagLib";

#if 0
		/// @todo Should maybe throw?
		promise.cancel();
#endif
		if(kjob != nullptr)
		{
			Q_EMIT kjob->SIGNAL_ImageBytes(retval);
		}
		return;
	}

	// Downcast it to whatever type it really is.
	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag() != nullptr)
		{
			retval = getCoverArtBytes_ID3(file->ID3v2Tag());
		}
		if (retval.isEmpty() && file->APETag())
		{
			retval = getCoverArtBytes_APE(file->APETag());
		}
	}
	else if (TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		retval = getCoverArtBytes_FLAC(file);
		if (retval.isEmpty() && file->ID3v2Tag())
		{
			retval = getCoverArtBytes_ID3(file->ID3v2Tag());
		}
	}

	if(retval.size() > 0)
	{
		qDebug() << "Found pic data, size:" << retval.size();
//        setSuccessFlag(true);
	}
	else
	{
		qDebug() << "Found no pic data";
//        setSuccessFlag(false);
	}

	promise.addResult(retval);

M_WARNING("TODO: Getting asserts here on app close during dir scan.");
	if(kjob != nullptr)
	{
		Q_EMIT kjob->SIGNAL_ImageBytes(retval);
	}
}

#if 0
void CoverArtJob::runFunctor()
{
	this->LoadCoverArt(m_ext_future, this, m_audio_file_url);
}
#endif
