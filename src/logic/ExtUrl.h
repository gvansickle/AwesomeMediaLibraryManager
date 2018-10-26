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

// Qt5
#include <QtCore>
#include <QUrl>
#include <QDateTime>
#include <QXmlQuery>
class QFileInfo;

// Ours
#include <utils/QtHelpers.h>

/**
 * An extended URL class.
 * Extensions are data used to detect if the referenced item has changed.
 */
class ExtUrl
{
	Q_GADGET

public:
	ExtUrl() = default;
	ExtUrl(const ExtUrl& other) = default;
	~ExtUrl() = default;

    /**
     * Construct an ExtUrl from a QUrl and QFileInfo.
     * Use case is e.g. results of a directory scan, where we'd have QFileInfo available.
     */
	explicit ExtUrl(const QUrl& qurl, const QFileInfo* qurl_finfo = nullptr);

	/// User-defined conversion to QUrl.
	explicit operator QUrl() const { return m_url; }

	ExtUrl& operator=(const QUrl& qurl) { m_url = qurl; return *this; /** @todo determine other info. */}


    /// ExtUrl Status enum.
    enum Status
    {
        Exists = 0x01,
        Accessible = 0x02,
        IsStale = 0x04
    };
    Q_DECLARE_FLAGS(Statuses, Status)
    Q_FLAG(Statuses)

    /**
     * Check the status of the URL, if it is accessible, if it's stale, etc.
     */
    Status getStatus();

    /// @name Data members.
    /// @{

	/// The QUrl.
	QUrl m_url;
	/// File size, or 0 if couldn't be determined.
	qint64 m_size {0};
	/// Last modified time.  Invalid if can't be determined(?).
	QDateTime m_last_modified_timestamp;
	/// Last modified time of file metadata (permissions etc.).  Invalid if can't be determined(?).
	QDateTime m_metadata_last_modified_timestamp;

    /// @}

//	bool isValid() { return m_url.isValid(); }

	/// QXmlStream{Read,Write} operators.
	QTH_FRIEND_QXMLSTREAM_OPS(ExtUrl);

	QXmlQuery write() const;

protected:

	void LoadModInfo();

};

Q_DECLARE_METATYPE(ExtUrl);
QTH_DECLARE_QDEBUG_OP(ExtUrl);
QTH_DECLARE_QDATASTREAM_OPS(ExtUrl);
QTH_DECLARE_QXMLSTREAM_OPS(ExtUrl);

#endif /* SRC_LOGIC_EXTURL_H_ */
