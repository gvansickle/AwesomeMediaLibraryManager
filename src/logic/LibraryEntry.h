/*
 * Copyright 2017, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef LIBRARYENTRY_H
#define LIBRARYENTRY_H

// Std C++
#include <vector>
#include <memory>

// Qt5
#include <QMetaType>
#include <QObject>
#include <QUrl>

// Qt5 Helpers
#include <src/utils/QtHelpers.h>

// Ours
#include <future/guideline_helpers.h>
#include "ExtMimeType.h"
#include "Metadata.h"
#include "src/utils/Fraction.h"
#include "serialization/ISerializable.h"


class LibraryEntry : public std::enable_shared_from_this<LibraryEntry>, public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(LibraryEntry);
	~LibraryEntry() override = default;

    explicit LibraryEntry(const QUrl& m_url);

    static std::shared_ptr<LibraryEntry> fromUrl(const QUrl& fileurl = QUrl());

	void populate(bool force_refresh = false);
	std::vector<std::shared_ptr<LibraryEntry>> split_to_tracks();

	// Same as populate().
	void refresh_metadata();

	bool isPopulated() const { return m_is_populated; }
	bool isError() const { return m_is_error; }
	bool isSubtrack() const { return m_is_subtrack; }
	bool isFromSameFileAs(const LibraryEntry *other) const;

	bool hasNoPregap() const;
	int getTrackNumber() const { return m_track_number; }
	int getTrackTotal() const { return m_total_track_number; }
	QUrl getUrl() const { return m_url; }

	// Get the decorated QUrl suitable for sending to the M2 player for this track.
	// Returns QUrl() if this entry has not been populated.
	QUrl getM2Url() const;

	QString getFilename() const { return m_url.fileName(); }
	QString getFileType() const { return m_metadata ? QString::fromUtf8(m_metadata.GetFiletypeName().c_str()) : QString(); }
    QMimeType getMimeType() const { return m_mime_type; };

	/// @name Serialization
	/// @{

	/// @name ISerializable interface
	/// @{

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @} // END ISerializable

	QTH_DECLARE_FRIEND_QDEBUG_OP(LibraryEntry);
    QTH_FRIEND_QDATASTREAM_OPS(LibraryEntry);

	/// @} // END Serialization

	/// @todo Do we want to return some kind of actual Image class here instead?
	QByteArray getCoverImageBytes();

	QVariantMap getAllMetadata() const;

	Fraction get_pre_gap_offset_secs() const { return m_pre_gap_offset_secs; }
	Fraction get_offset_secs() const { return m_offset_secs; }
	Fraction get_length_secs() const { return m_length_secs; }

	Metadata metadata() const { return m_metadata; }
	Metadata track_cuesheet_metadata() const;

	QStringList getMetadata(QString key) const;


protected:

	// The URL to the media.
	QUrl m_url;

	// All we have is (maybe) a URL, we don't have any other info on this file yet, so all other fields are
	// not valid.
	bool m_is_populated = false;

	// True if there was an error trying to open or read this URL.
	bool m_is_error = false;

    ExtMimeType m_mime_type;

    /// @todo Is all the below soon to be obsolete?
	// Flag if this is a subtrack of a single-file album rip.
	// See https://xiph.org/flac/format.html#metadata_block_cuesheet
	bool m_is_subtrack = false;

	// The track number of this track on the CD.
	qint64 m_track_number {0};
	// Total number of tracks on the CD.
	qint64 m_total_track_number {0};

	Fraction m_pre_gap_offset_secs;// = 0;
	/// Start of the audio, in secs.
	Fraction m_offset_secs; // = 0;
	/// Length of the audio in secs.
	Fraction m_length_secs; // = 0;

	Metadata m_metadata;
};

/// So we can more easily pass ptrs in QVariants.
Q_DECLARE_METATYPE(LibraryEntry);
Q_DECLARE_METATYPE(LibraryEntry*);
Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<LibraryEntry>);
Q_DECLARE_METATYPE(std::vector<std::shared_ptr<LibraryEntry>>);
Q_DECLARE_METATYPE(QSharedPointer<LibraryEntry>);

QTH_DECLARE_QDEBUG_OP(LibraryEntry);

inline QDebug operator<<(QDebug dbg, const std::shared_ptr<LibraryEntry> &libentry)
{
	return dbg << libentry.get();
}

QTH_DECLARE_QDATASTREAM_OPS(LibraryEntry);
QTH_DECLARE_QDATASTREAM_OPS(std::shared_ptr<LibraryEntry>);

#endif // LIBRARYENTRY_H
