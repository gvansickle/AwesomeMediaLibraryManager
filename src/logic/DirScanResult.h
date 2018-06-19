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
#include <QDateTime>
#include <QDataStream>
#include <QDebug>

/// Ours
#include <utils/QtHelpers.h>

class FileModificationInfo
{
    Q_GADGET

public:
    FileModificationInfo() = default;
    FileModificationInfo(const QFileInfo &fmodinfo)
        : m_size(fmodinfo.size()),
          m_last_modified_timestamp(fmodinfo.lastModified()),
          m_metadata_last_modified_timestamp(fmodinfo.metadataChangeTime()) {}
    FileModificationInfo(const FileModificationInfo& fmodinfo) = default;
    ~FileModificationInfo() = default;

    /// File size, or 0 if couldn't be determined.
    qint64 m_size;
    /// Last modified time.  Invalid if can't be determined(?).
    QDateTime m_last_modified_timestamp;
    /// Last modified time of file metadata (permissions etc.).  Invalid if can't be determined(?).
    QDateTime m_metadata_last_modified_timestamp;

//    QTH_FRIEND_QDEBUG_OP(FileModificationInfo);
    friend QDebug operator<<(QDebug dbg, const FileModificationInfo& obj)
    {
        return dbg << obj.m_size << obj.m_last_modified_timestamp << obj.m_metadata_last_modified_timestamp;
    }

    friend QDataStream &operator<<(QDataStream &out, const FileModificationInfo & myObj)
    {
        return out << myObj.m_size << myObj.m_last_modified_timestamp << myObj.m_metadata_last_modified_timestamp;
    }
    friend QDataStream &operator>>(QDataStream &in, FileModificationInfo & myObj)
    {
        return in >> myObj.m_size >> myObj.m_last_modified_timestamp >> myObj.m_metadata_last_modified_timestamp;
    }
};

Q_DECLARE_METATYPE(FileModificationInfo);
QTH_DECLARE_QDATASTREAM_OPS(FileModificationInfo);

/**
 * A single hit found during a directory scan.
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
        HasSidecarCueSheet = 0x04,
        /// Directory has one or more album art files.
        HasArt = 0x08,
        /// Dir is just a bunch of MP3's.
        JBOMP3s = 0x10,
        /// Nothing is known about the dir.
        Unknown = 0x00
    };
    Q_DECLARE_FLAGS(DirProps, DirProp)
    Q_FLAG(DirProps)

    DirProps getDirProps() const { return m_dir_props; }

    /// Get the URL which points to the actual media file found.
    QUrl getMediaQUrl() const { return m_found_url; }

    /// URL to any sidecar cuesheet found.
    /// If one was found, DirProp::HasSidecarCueSheet will be set.
    /// Returned URL will not be valid if there was no sidecar cue sheet.
    QUrl getSidecarCuesheetQUrl() const { return m_cue_url; }

    QTH_FRIEND_QDEBUG_OP(DirScanResult);
    QTH_FRIEND_QDATASTREAM_OPS(DirScanResult);

protected:

    void determineDirProps(const QFileInfo &);

    QVector<QUrl> otherMediaFilesInDir(const QFileInfo& finfo);

    DirProps m_dir_props { Unknown };

    /// The media URL which was found.
    QUrl m_found_url;
    /// Info for detecting changes
    FileModificationInfo m_found_url_modinfo;

    QUrl m_dir_url;

    /// URL to a sidecar cuesheet.  May be empty if none was found.
    QUrl m_cue_url;
    /// Info for detecting changes
    FileModificationInfo m_cue_url_modinfo;
};

Q_DECLARE_METATYPE(DirScanResult);

QTH_DECLARE_QDEBUG_OP(DirScanResult);
QTH_DECLARE_QDATASTREAM_OPS(DirScanResult);

#endif /* SRC_LOGIC_DIRSCANRESULT_H_ */
