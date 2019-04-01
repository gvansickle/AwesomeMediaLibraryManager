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

#include <QMimeDatabase>

#include "SupportedMimeTypes.h"

#include <utils/StringHelpers.h>


//SupportedMimeTypes* SupportedMimeTypes::m_the_instance {nullptr};

SupportedMimeTypes& SupportedMimeTypes::instance(QObject* parent)
{
	static SupportedMimeTypes* m_the_instance = (Q_ASSERT(parent != nullptr), new SupportedMimeTypes(parent));

    return *m_the_instance;
}

SupportedMimeTypes::SupportedMimeTypes(QObject* parent) : QObject(parent)
{
    // "*.flac", "*.mp3", "*.ogg", "*.wav"
    m_mime_audio_types << "audio/flac" << "audio/mpeg" << "audio/ogg" << "audio/x-flac+ogg" << "audio/x-vorbis+ogg" << "audio/x-wav";
    m_mime_audio_associated_types << "application/x-cue";
    m_mime_playlist_types << "audio/x-mpegurl" /* *.m3u/8/ .vlc */ << "application/xspf+xml";
}

SupportedMimeTypes::~SupportedMimeTypes()
{
}

QVector<QMimeType> SupportedMimeTypes::supportedAudioMimeTypes() const
{
    QMimeDatabase mdb;

	QVector<QMimeType> retval;

    for(const auto& s : m_mime_audio_types)
	{
        auto mimetype = mdb.mimeTypeForName(s);
        Q_ASSERT(mimetype.isValid());
        retval.push_back(mimetype);
	}

    return retval;
}

QStringList SupportedMimeTypes::supportedAudioMimeTypesAsFilterStringList() const
{
    QStringList retval;

    auto mimetypes = supportedAudioMimeTypes();

    for(const auto& s : mimetypes)
    {
        retval.push_back(s.filterString());
    }

    return retval;
}

QStringList SupportedMimeTypes::supportedAudioMimeTypesAsSuffixStringList() const
{
    QStringList retval;

    auto mimetypes = supportedAudioMimeTypes();

    for(const auto& s : mimetypes)
    {
        auto suffixes = s.suffixes();
        for(const auto& substr : suffixes)
        {
            retval.push_back("*." + substr);
        }
    }

    retval.removeDuplicates();

    return retval;
}

