/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Std C++
#include <optional>

// Qt5
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QDataStream>
#include <QDebug>

// Ours
#include <utils/RegisterQtMetatypes.h> ///< For at least std::optional<bool>.
#include <utils/QtHelpers.h>
#include "ExtUrl.h"
#include <logic/models/AbstractTreeModelItem.h>
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>

class CollectionMedium;
class ScanResultsTreeModelItem;

/**
 * A single hit found during a directory scan.
 */
class DirScanResult : public virtual ISerializable
{
	Q_GADGET // Needed for DirProp enum below.

public:
	/// @name Public default and copy constructors and destructor needed for Q_DECLARE_METATYPE().
	/// @{
	M_GH_RULE_OF_FIVE_DEFAULT_C21(DirScanResult);
	~DirScanResult() override = default;
	/// @}

    /// Constructor for public consumption.
	explicit DirScanResult(const QUrl& found_url, const QFileInfo& found_url_finfo);

	friend class CollectionMedium;

	/**
	 * Flags for various properties of a scanned directory.
	 * [[deprecated]]
	 */
    enum DirProp
    {
	    /// Nothing is known about the directory.
	    Unknown = 0x00000000'00000000,
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

	DirPropFlags getDirProps() const { return m_flags_dirprops; }

	/// Get the ExtUrl which points to the actual media file found.
	const ExtUrl& getMediaExtUrl() const { return m_exturl_media; }

    /// URL to any sidecar cuesheet found.
    /// If one was found, DirProp::HasSidecarCueSheet will be set.
    /// Returned URL will not be valid if there was no sidecar cue sheet.
	const ExtUrl& getSidecarCuesheetExtUrl() const { return m_exturl_cuesheet; }

	/// @name Serialization
	/// @{

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/// @}

    QTH_DECLARE_FRIEND_QDEBUG_OP(DirScanResult);
//	QTH_FRIEND_QDATASTREAM_OPS(DirScanResult);

protected:

	void determineDirProps(const QFileInfo& finfo);

	QVector<ExtUrl> otherMediaFilesInDir(const QFileInfo& finfo);

	// Member vars.

	/// Absolute URL to the directory.
	ExtUrl m_exturl_dir_url;

	DirPropFlags m_flags_dirprops { Unknown };

	/// @name Fancy New C++17 flags using std::optional.
	/// @{
	/// Directory contains a separate cue sheet file.
	/// @note Does not preclude an embedded cuesheet.
	std::optional<bool> m_has_sidecar_cuesheet;
	/// Directory (actually the media file) contains an embedded cue sheet.
	/// @note Does not preclude a sidecar cuesheet.
	std::optional<bool> m_has_embedded_cuesheet;
	/// Directory contains only one album, not just e.g. a dump of mp3's.
	std::optional<bool> m_single_album;
	/// @}

    /// The media URL which was found.
	ExtUrl m_exturl_media;

    /// URL to a sidecar cuesheet.  May be empty if none was found.
	ExtUrl m_exturl_cuesheet;

};

Q_DECLARE_METATYPE(DirScanResult);

Q_DECLARE_OPERATORS_FOR_FLAGS(DirScanResult::DirPropFlags);

QTH_DECLARE_QDEBUG_OP(DirScanResult);
//QTH_DECLARE_QDATASTREAM_OPS(DirScanResult);

#endif /* SRC_LOGIC_DIRSCANRESULT_H_ */
