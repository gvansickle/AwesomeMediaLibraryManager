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

/**
 * @file ExtUrl.h
 */
#ifndef SRC_LOGIC_EXTURL_H_
#define SRC_LOGIC_EXTURL_H_

// Std C++
#include <memory> // for std::unique_ptr<>.

// Qt5
#include <QtCore>
#include <QUrl>
#include <QDateTime>
#include <QXmlQuery>
class QFileInfo;

// Ours
#include <utils/QtHelpers.h>
#include <logic/serialization/XmlObjects.h>
#include "ISerializable.h"


#if 0 // FileInfo
class FileModificationInfo
{
    Q_GADGET

public:
    /// @name Default and copy constructors and destructor for Q_DELCARE_METATYPE().
    /// @{
    FileModificationInfo() = default;
    FileModificationInfo(const FileModificationInfo& fmodinfo) = default;
    ~FileModificationInfo() = default;
    /// @}

    explicit FileModificationInfo(const QFileInfo &fmodinfo)
        : m_file_size_bytes(fmodinfo.size()),
          m_last_modified_timestamp(fmodinfo.lastModified()),
          m_metadata_last_modified_timestamp(fmodinfo.metadataChangeTime()) {}

    /// File size, or 0 if couldn't be determined.
    qint64 m_file_size_bytes {0};
    /// Last modified time.  Invalid if can't be determined(?).
    QDateTime m_last_modified_timestamp;
    /// Last modified time of file metadata (permissions etc.).  Invalid if can't be determined(?).
    QDateTime m_metadata_last_modified_timestamp;

//    QTH_FRIEND_QDEBUG_OP(FileModificationInfo);
    friend QDebug operator<<(QDebug dbg, const FileModificationInfo& obj)
    {
        return dbg << obj.m_file_size_bytes << obj.m_last_modified_timestamp << obj.m_metadata_last_modified_timestamp;
    }
};

Q_DECLARE_METATYPE(FileModificationInfo);
//QTH_DECLARE_QDATASTREAM_OPS(FileModificationInfo);
#endif

/**
 * An extended URL class.
 * Extensions are data used to detect if the referenced item has changed.
 */
class ExtUrl : public ISerializable
{
	Q_GADGET

public:
	ExtUrl() = default;
	ExtUrl(const ExtUrl& other) = default;
	~ExtUrl() override = default;

    /**
     * Construct an ExtUrl from a QUrl and QFileInfo.
     * Use case is e.g. results of a directory scan, where we'd have QFileInfo available.
     */
	explicit ExtUrl(const QUrl& qurl, const QFileInfo* qurl_finfo = nullptr);

	/// User-defined conversion to QUrl.
	explicit operator QUrl() const { return m_url; }

	/// Assignment from QUrl.
	ExtUrl& operator=(const QUrl& qurl) { m_url = qurl; return *this; /** @todo determine other info. */}


    /**
     * ExtUrl Status enum.
     */
    enum Statuses
    {
        Exists = 0x01,
        Accessible = 0x02,
        IsStale = 0x04
    };
    /// "The Q_DECLARE_FLAGS(Flags, Enum) macro expands to: typedef QFlags<Enum> Flags;"
    /// "The Q_DECLARE_FLAGS() macro does not expose the flags to the meta-object system"
    /// @link http://doc.qt.io/qt-5/qflags.html#flags-and-the-meta-object-system
    /// @link http://doc.qt.io/qt-5/qflags.html#Q_DECLARE_FLAGS
    Q_DECLARE_FLAGS(Status, Statuses)
    /// "This macro registers a single flags type with the meta-object system.".
    /// @link http://doc.qt.io/qt-5/qobject.html#Q_FLAG
    Q_FLAG(Status)

    /**
     * Check the status of the URL, if it is accessible, if it's stale, etc.
     */
    Status getStatus();

    /// @name Data members.
    /// @{

	/// The QUrl.
	QUrl m_url;

	/// @name Info mainly for determining if the file was modified since we last looked at it.
	/// @{

	/// Last time we refreshed the modification info related to this URL.
	QDateTime m_timestamp_last_refresh;

	/// File size, or 0 if couldn't be determined.
	qint64 m_file_size_bytes {0};

	/// Creation time.
	/// Needed for XSPF etc.
	QDateTime m_creation_timestamp;

	/// Last modified time.  Invalid if can't be determined(?).
	QDateTime m_last_modified_timestamp;

	/// Last modified time of file metadata (permissions etc.).  Invalid if can't be determined(?).
	QDateTime m_metadata_last_modified_timestamp;
	/// @}

	/// @}

//	bool isValid() { return m_url.isValid(); }

	QTH_FRIEND_QDATASTREAM_OPS(ExtUrl);

	/// QXmlStream{Read,Write} operators.
//	QTH_FRIEND_QXMLSTREAM_OPS(ExtUrl);

	/// @todo Can these be protected?
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/**
	 * Return an XmlElement representing this ExtUrl.
	 */
	XmlElement toXml() const;


protected:

	void save_mod_info(const QFileInfo* qurl_finfo);

	/// Populate the modification info from qurl_finfo.
	/// If null, load the info using m_url.
	void load_mod_info(const QFileInfo* qurl_finfo = nullptr);

};

Q_DECLARE_METATYPE(ExtUrl);
/// "declares global operator|() functions for Flags"
Q_DECLARE_OPERATORS_FOR_FLAGS(ExtUrl::Status);

QTH_DECLARE_QDEBUG_OP(ExtUrl);
QTH_DECLARE_QDATASTREAM_OPS(ExtUrl);
//QTH_DECLARE_QXMLSTREAM_OPS(ExtUrl);

#endif /* SRC_LOGIC_EXTURL_H_ */
