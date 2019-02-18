/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_AUDIOFILETYPE_H
#define AWESOMEMEDIALIBRARYMANAGER_AUDIOFILETYPE_H

// Qt5
#include <QMetaType>
#include <QObject>

// Ours.
#include <utils/RegisterQtMetatypes.h>

/**
 * @brief The AudioFileType class
 * @todo Do we really need this, or would ExtMimeType serve just as well?
 */
class AudioFileType
{
	Q_GADGET

public:
	enum Type
	{
		UNKNOWN,
		FLAC,
		MP3,
		OGG_VORBIS,
		WAV,
	};

	Q_ENUM(Type);
};

Q_DECLARE_METATYPE(AudioFileType);


#endif //AWESOMEMEDIALIBRARYMANAGER_AUDIOFILETYPE_H
