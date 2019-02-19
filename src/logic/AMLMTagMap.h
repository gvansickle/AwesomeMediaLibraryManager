/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file AMLMTagMap.h
 */
#ifndef SRC_LOGIC_AMLMTAGMAP_H_
#define SRC_LOGIC_AMLMTAGMAP_H_

// Std C++
#include <map>
#include <string>

// Qt5
#include <QMetaType>
#include <QVariant>

// Ours.
#include <future/guideline_helpers.h>
#include <logic/serialization/ISerializable.h>

using TagMap = std::map<std::string, std::vector<std::string>>;

/*
 *
 */
class AMLMTagMap : public virtual ISerializable
{
public:
	M_GH_RULE_OF_ZERO(AMLMTagMap);

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

private:
	using container_type = std::multimap<std::string, std::string>;

	container_type m_the_map;
};

Q_DECLARE_METATYPE(AMLMTagMap);

#endif /* SRC_LOGIC_AMLMTAGMAP_H_ */
