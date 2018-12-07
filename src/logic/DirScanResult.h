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

// Qt5
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QDataStream>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QXmlQuery>

// Ours
#include <src/utils/QtHelpers.h>
#include "ExtUrl.h"
#include <src/logic/models/AbstractTreeModelItem.h>
#include <src/logic/serialization/ISerializable.h>

class CollectionMedium;
class ScanResultsTreeModelItem;

/**
 * A single hit found during a directory scan.
 */
class DirScanResult : public ISerializable
{
	Q_GADGET

public:
	/// @name Public default and copy constructors and destructor needed for Q_DECLARE_METATYPE().
	/// @{
    DirScanResult() = default;
    DirScanResult(const DirScanResult& other) = default;
	~DirScanResult() override = default;
	/// @}

    /// Constructor for public consumption.
	explicit DirScanResult(const QUrl& found_url, const QFileInfo& found_url_finfo);

	friend class CollectionMedium;

    enum DirProp
    {
	    /// Nothing is known about the directory.
	    Unknown = 0x00,
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
        JBOMP3s = 0x10
    };
	/// "The Q_DECLARE_FLAGS(Flags, Enum) macro expands to: typedef QFlags<Enum> Flags;"
	/// "The Q_DECLARE_FLAGS() macro does not expose the flags to the meta-object system"
	/// @link http://doc.qt.io/qt-5/qflags.html#flags-and-the-meta-object-system
	/// @link http://doc.qt.io/qt-5/qflags.html#Q_DECLARE_FLAGS
    Q_DECLARE_FLAGS(DirPropFlags, DirProp)
	/// "This macro registers a single flags type with the meta-object system.".
	/// @link http://doc.qt.io/qt-5/qobject.html#Q_FLAG
    Q_FLAG(DirPropFlags)

	DirPropFlags getDirProps() const { return m_dir_props; }

	/// Get the ExtUrl which points to the actual media file found.
	const ExtUrl& getMediaExtUrl() const { return m_media_exturl; }

    /// URL to any sidecar cuesheet found.
    /// If one was found, DirProp::HasSidecarCueSheet will be set.
    /// Returned URL will not be valid if there was no sidecar cue sheet.
	const ExtUrl& getSidecarCuesheetExtUrl() const { return m_cue_exturl; }

	/// @name Serialization
	/// @{

	/// @todo Can these be protected?
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/// @}

    QTH_FRIEND_QDEBUG_OP(DirScanResult);
//	QTH_FRIEND_QDATASTREAM_OPS(DirScanResult);

protected:

	void determineDirProps(const QFileInfo& finfo);

	QVector<ExtUrl> otherMediaFilesInDir(const QFileInfo& finfo);

	// Member vars.

	/// Absolute URL to the directory.
	ExtUrl m_dir_exturl;

    DirPropFlags m_dir_props { Unknown };

    /// The media URL which was found.
	ExtUrl m_media_exturl;

    /// URL to a sidecar cuesheet.  May be empty if none was found.
	ExtUrl m_cue_exturl;

};

Q_DECLARE_METATYPE(DirScanResult);

Q_DECLARE_OPERATORS_FOR_FLAGS(DirScanResult::DirPropFlags);

QTH_DECLARE_QDEBUG_OP(DirScanResult);
//QTH_DECLARE_QDATASTREAM_OPS(DirScanResult);

#endif /* SRC_LOGIC_DIRSCANRESULT_H_ */
