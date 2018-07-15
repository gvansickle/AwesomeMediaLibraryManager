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

#ifndef SRC_LOGIC_COVERARTJOB_H_
#define SRC_LOGIC_COVERARTJOB_H_

#include <config.h>

// Qt5
#include <QUrl>

// Ours
#include "concurrency/AMLMJob.h"

class CoverArtJob;
using CoverArtJobPtr = QPointer<CoverArtJob>;

/*
 *
 */
class CoverArtJob : public AMLMJob, public UniqueIDMixin<CoverArtJob>
{
	Q_OBJECT

	using BASE_CLASS = AMLMJob;

	/**
	 * @note CRTP: Still need this to avoid ambiguous name resolution.
	 * @see https://stackoverflow.com/a/46916924
	 */
	using UniqueIDMixin<CoverArtJob>::uniqueQObjectName;

Q_SIGNALS:
    void SIGNAL_ImageBytes(QByteArray);

protected:

    explicit CoverArtJob(QObject* parent, const QUrl& url);

public:
    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<QByteArray>;
    /// @}

	~CoverArtJob() override;

    static CoverArtJobPtr make_job(QObject *parent, const QUrl& url);

    QByteArray m_byte_array;

    ExtFutureType& get_extfuture_ref() override { return m_ext_future; }

protected:

    CoverArtJob* asDerivedTypePtr() override { return this; }

    void runFunctor() override;

private:

    ExtFutureType m_ext_future;

    QUrl m_audio_file_url;
};

#endif /* SRC_LOGIC_COVERARTJOB_H_ */
