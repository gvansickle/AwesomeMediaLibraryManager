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

#include "CoverArtJob.h"

CoverArtJob::CoverArtJob(QObject* parent) : BASE_CLASS(parent)
{
}

CoverArtJob::~CoverArtJob()
{
}

void CoverArtJob::AsyncGetCoverArt(const QUrl &url)
{
    m_audio_file_url = url;
}

void CoverArtJob::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);

    // Mostly copy/paste from QByteArray MetadataTaglib::getCoverArtBytes() const

    QByteArray retval;

    // Open the file ref.
    QString url_as_local = m_audio_file_url.toLocalFile();

    TagLib::FileRef fr {openFileRef(url_as_local)};
    if(fr.isNull())
    {
        qWarning() << "Unable to open file" << url_as_local << "with TagLib";
        Q_EMIT SIGNAL_ImageBytes(retval);
    }

    // Downcast it to whatever type it really is.
    if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
    {
        if (file->ID3v2Tag())
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
    }
    else
    {
        qDebug() << "Found no pic data";
    }

    Q_EMIT SIGNAL_ImageBytes(retval);
}

