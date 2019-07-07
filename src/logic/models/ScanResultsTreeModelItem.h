/*
 * Copyright 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Ours
#include <logic/DirScanResult.h>
#include <logic/serialization/ISerializable.h>
#include <future/enable_shared_from_this_virtual.h>
class LibraryEntry;
class ScanResultsTreeModel;
/// @todo TEMP


/**
 * Model of the results of scanning a directory tree.
 */
class ScanResultsTreeModelItem : public AbstractTreeModelItem, public enable_shared_from_this_virtual<ScanResultsTreeModelItem>

{
	using BASE_CLASS = AbstractTreeModelItem;

protected:
	/// Create a new model item populated with the passed DirScanResult.
	explicit ScanResultsTreeModelItem(const DirScanResult& dsr,
	                                  const std::shared_ptr<AbstractTreeModel> model,
	                                  bool is_root = false);

public:
	/**
	 * Named constructors.
	 */
	static std::shared_ptr<ScanResultsTreeModelItem> construct(const DirScanResult& dsr,
			std::shared_ptr<AbstractTreeModel> model,
            bool is_root = false);
	static std::shared_ptr<ScanResultsTreeModelItem> construct(const QVariant& variant,
			std::shared_ptr<AbstractTreeModel> model,
			bool is_root = false);
	ScanResultsTreeModelItem() {};
	~ScanResultsTreeModelItem() override;

	void setDirscanResults(const DirScanResult& dsr);

	/**
	 * Column data override.
	 *
	 * @todo Add role.
	 */
	QVariant data(int column, int role = Qt::DisplayRole) const override;

	DirScanResult getDsr() const { return m_dsr; };

	int columnCount() const override;

	/// @name ISerializable interface
	/// @{

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @} // END Serialization


protected:

	/// The directory scan results corresponding to this entry.
	/// This is things like the main media URL, sidecar cue sheet URLs, timestamp info, etc.
	DirScanResult m_dsr;
};


class SRTMItem_LibEntry : public ScanResultsTreeModelItem, public enable_shared_from_this_virtual<SRTMItem_LibEntry>
{
	using BASE_CLASS = ScanResultsTreeModelItem;

protected:
	explicit SRTMItem_LibEntry(const DirScanResult& dsr,
							   const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root);

public:
	static std::shared_ptr<SRTMItem_LibEntry> construct(const DirScanResult& dsr,
			const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root = false);
	static std::shared_ptr<SRTMItem_LibEntry> construct(const QVariant& variant,
			const std::shared_ptr<ScanResultsTreeModel>& model, bool is_root = false);
	SRTMItem_LibEntry() {};
	~SRTMItem_LibEntry() override = default;

	QVariant data(int column, int role = Qt::DisplayRole) const override;

	int columnCount() const override;

	void setLibraryEntry(std::shared_ptr<LibraryEntry> libentry) { m_library_entry = libentry; };

	/// @name ISerializable interface
	/// @{

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @} // END Serialization
	
protected:

private:
	std::string m_key {"key"};
	std::string m_val {"value"};
	std::shared_ptr<LibraryEntry> m_library_entry;
};

/// @todo Need this here for QVariant::fromValue().
//Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(ScanResultsTreeModelItem);
Q_DECLARE_METATYPE(SRTMItem_LibEntry);

#endif /* SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_ */
