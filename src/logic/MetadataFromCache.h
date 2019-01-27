/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef METADATAFROMCACHE_H
#define METADATAFROMCACHE_H

#include "MetadataAbstractBase.h"

#include <QByteArray>


class QJsonObject;

class MetadataFromCache : public MetadataAbstractBase
{
public:
	MetadataFromCache();
	virtual ~MetadataFromCache() override;

	virtual bool isFromCache() const override { return true; }

    virtual bool read(const QUrl /*url*/&) override { Q_ASSERT(0); return false; }

	void readFromJson(const QJsonObject& jo);

	/// Track metadata.
	virtual Metadata get_one_track_metadata(int track_index) const override;

	/// Embedded art.
	virtual QByteArray getCoverArtBytes() const override;

private:

	virtual MetadataFromCache* clone_impl() const override;
};


#endif // METADATAFROMCACHE_H
