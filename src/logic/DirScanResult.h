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

#ifndef SRC_LOGIC_DIRSCANRESULT_H_
#define SRC_LOGIC_DIRSCANRESULT_H_

#include <config.h>

/// Qt5
#include <QUrl>
#include <QFileInfo>

/**
 *
 */
class DirScanResult
{
    Q_GADGET

public:
	DirScanResult();
    DirScanResult(const QUrl& found_url, const QFileInfo& found_url_finfo);
	virtual ~DirScanResult();

    /**
     * URLs:
     * - Sidecar cuesheet
     * - Sidecar album art (folder.jpg/cover.jpg)
     * Bools:
     * - Is result:
     * -- Dir with only single album/disc rip
     * -- Dir with random files
     */

    enum DirProp
    {
        /// Directory contains only one album, not just e.g. a dump of mp3's.
        SingleAlbum = 0x01,
        /// Directory contains a single audio file.
        SingleFile = 0x02,
        /// Directory contains a separate cue sheet file.
        /// @note Does not preclude an embedded cuesheet.
        HasCueSheet = 0x04,
        /// Directory has one or more album art files.
        HasArt = 0x08,
        /// Dir is just a bunch of MP3's.
        JBOMP3s = 0x10,
        /// Nothing is known about the dir.
        Unknown = 0x80
    };
    Q_DECLARE_FLAGS(DirProps, DirProp)
    Q_FLAG(DirProps)

    DirProps getDirProps() const { return m_dir_props; }

protected:

    void determineDirProps();

    QUrl m_found_url;
    QFileInfo m_found_url_finfo;

    DirProps m_dir_props { Unknown };

    QUrl m_dir_url;
    QUrl m_cue_url;

};

Q_DECLARE_METATYPE(DirScanResult);

#endif /* SRC_LOGIC_DIRSCANRESULT_H_ */
