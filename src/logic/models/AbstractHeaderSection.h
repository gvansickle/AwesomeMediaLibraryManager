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
 * @file AbstractHeaderSection.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTHEADERSECTION_H_
#define SRC_LOGIC_MODELS_ABSTRACTHEADERSECTION_H_

// Std C++
#include <map>
#include <memory>

// Qt5
#include <QVariant>

/**
 * Class representing a single model header section (column).  An array of these would be held in the
 * model's AbstractTreeModelHeaderItem as its values.
 */
class AbstractHeaderSection
{
public:

//	enum HeaderItemDataRole
//	{
//		XmlTagName = Qt::ItemDataRole::UserRole+1, ///< string
//		XmlAttributes, ///< QXmlStreamAttributes, ~QVector of QString pairs.
//		/// In derived classes, start any new ItemDataRoles at this value.
//		NextHeaderItemDataRole
//	};

public:
	AbstractHeaderSection();
	virtual ~AbstractHeaderSection();

	/**
	 * Override in derived classes to return the QVariant corresponding to section+orientation+role.
	 * Section and orientation are fixed, so other than error checking, role is the only mapping input.
	 */
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) = 0;

	/**
	 * This gets called via:
	 * Model::setHeaderData()->ModelHeaderItem::setHeaderData()->root_item->setData(section, orientation, value, role).
	 * Override in derived classes to set the header data corresponding to the given section number, orientation, and role.
	 */
	virtual bool setHeaderData(int section, Qt::Orientation orientation,
						 const QVariant &value, int role = Qt::EditRole) = 0;

	virtual QVariant lookup_role(int role) const = 0;

	virtual int section() = 0;
	virtual Qt::Orientation orientation() = 0;
};

//Q_DECLARE_METATYPE(AbstractHeaderSection);
Q_DECLARE_METATYPE(std::shared_ptr<AbstractHeaderSection>);

/**
 * Class representing a "dumb" header section, i.e. no more functionality than
 * that provided by the model interface.
 */
class BasicHeaderSection : public AbstractHeaderSection
{
public:
	BasicHeaderSection(int section, Qt::Orientation orientation,
            const QVariant &value, int role = Qt::EditRole);
	~BasicHeaderSection() override = default;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) override;

	bool setHeaderData(int section, Qt::Orientation orientation,
							 const QVariant &value, int role = Qt::EditRole) override;

	QVariant lookup_role(int role) const override;

	int section() override { return m_section; };
	Qt::Orientation orientation() override { return m_orientation; };

protected:

	/// Section and Orientation are fixed.
	int m_section {0};
	Qt::Orientation m_orientation {Qt::Horizontal};

	/**
	 * Mapping from roles to values.
	 */
	std::map</*Qt::ItemDataRole*/int, QVariant> m_role_to_value_map;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTHEADERSECTION_H_ */
