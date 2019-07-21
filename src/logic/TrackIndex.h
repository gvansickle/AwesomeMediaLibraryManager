//
// Created by gary on 7/21/19.
//

#ifndef AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H
#define AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H

// Std C++
#include <string>
#include <vector>

// Qt5
#include <QtCore>

// Ours
#include "TrackMetadata.h"
#include <logic/serialization/ISerializable.h>
#include <future/guideline_helpers.h>
#include <third_party/libcue/libcue.h>
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




#endif //AWESOMEMEDIALIBRARYMANAGER_TRACKINDEX_H
