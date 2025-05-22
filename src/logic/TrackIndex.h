/*
 * Copyright 2017, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H
#define AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H

/// @file

// Std C++
#include <string>
#include <vector>

// Qt
#include <QVariant>

// Ours
//#include "Frames.h"
using Frames = qint64;
//#include "TrackMetadata.h"
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>
//#include <third_party/libcue/libcue.h>
#include "AMLMTagMap.h"

class TrackIndex : public virtual ISerializable
{
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(TrackIndex);
	~TrackIndex() override = default;

	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

	// ~ "00" to "99"
	std::string m_index_num {};

	// Index value in Frames.
	Frames m_index_frames {};
};

Q_DECLARE_METATYPE(TrackIndex);
Q_DECLARE_METATYPE(std::vector<TrackIndex>);


#endif //AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H
