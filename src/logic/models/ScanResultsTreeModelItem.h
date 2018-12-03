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
 * @file ScanResultsTreeModelItem.h
 */
#ifndef SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_
#define SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_

#include "AbstractTreeModelItem.h"

// Qt5
#include <QStringLiteral>
class QXmlStreamReader;

// Ours
#include "../DirScanResult.h"
#include <logic/serialization/ISerializable.h>

/**
 * Model of the results of scanning a directory tree.
 */
class ScanResultsTreeModelItem : public AbstractTreeModelItem
{
	using BASE_CLASS = AbstractTreeModelItem;

public:
	/// Create a default-constructed (i.e. "blank") ScanResultsTreeModelItem, possibly with a given parent.
	explicit ScanResultsTreeModelItem(AbstractTreeModelItem *parent = nullptr) : AbstractTreeModelItem(parent) {};
	/// Create a new model item populated with the passed DirScanResult.
	explicit ScanResultsTreeModelItem(const DirScanResult& dsr, AbstractTreeModelItem *parent = nullptr);
	~ScanResultsTreeModelItem() override;

	/**
	 * Column data override.
	 *
	 * @todo Add role.
	 */
	QVariant data(int column) const override;

	int columnCount() const override;

	/// @name Serialization
	/// @{

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @} // END Serialization


protected:

	/**
	 * Factory function for creating default-constructed nodes.
	 * Used by insertChildren().  Override in derived classes.
	 * @todo Convert to smart pointer (std::unique_ptr<AbstractTreeModelItem>) return type, retain covariant return.
	 */
	ScanResultsTreeModelItem*
	create_default_constructed_child_item(AbstractTreeModelItem *parent = nullptr) override;

	/// @name Virtual functions called by the base class to complete certain operations.
	///       The base class will have error-checked function parameters.
	/// @{
	bool derivedClassSetData(int column, const QVariant &value) override;
	bool derivedClassInsertColumns(int insert_before_column, int num_columns) override;
	bool derivedClassRemoveColumns(int first_column_to_remove, int num_columns) override;
	/// @}

	/// The directory scan results corresponding to this entry.
	/// This is things like the main media URL, sidecar cue sheet URLs, timestamp info, etc.
	DirScanResult m_dsr;

};

#endif /* SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_ */
