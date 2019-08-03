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

// Std C++
#include <memory>

// Qt5
#include <QStringLiteral>

// Ours
#include "AbstractTreeModelItem.h"
#include <logic/DirScanResult.h>
#include <logic/serialization/ISerializable.h>
#include <future/enable_shared_from_this_virtual.h>
class AbstractTreeModelHeaderItem;
class LibraryEntry;
class ScanResultsTreeModel;
/// @todo TEMP


/**
 * Tree model item containing the results of a single DirScanResult.
 * KDEN: ~similar to AbstractProjectItem
 */
class ScanResultsTreeModelItem : public AbstractTreeModelItem//, public enable_shared_from_this_virtual<ScanResultsTreeModelItem>

{
	using BASE_CLASS = AbstractTreeModelItem;

//protected:
public:
	/// Create a new model item populated with the passed DirScanResult.
//	explicit ScanResultsTreeModelItem(const DirScanResult& dsr, std::shared_ptr<ScanResultsTreeModel> model,
//	                                  bool is_root = false);
	explicit ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
	explicit ScanResultsTreeModelItem(const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
	explicit ScanResultsTreeModelItem(const QVariant& variant, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());

//
//	explicit ScanResultsTreeModelItem(std::shared_ptr<ScanResultsTreeModel> model, bool is_root = false);
public:
//	/**
//	 * Named constructors.
//	 */
//	/// Create a new model item populated with the passed DirScanResult.
//	static std::shared_ptr<ScanResultsTreeModelItem> construct(const DirScanResult& dsr, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
//	static std::shared_ptr<ScanResultsTreeModelItem> construct(const QVariant& variant, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
//	ScanResultsTreeModelItem() {};
//	explicit ScanResultsTreeModelItem(const DirScanResult& dsr, const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
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

/// @todo Need this here for QVariant::fromValue().
//Q_DECLARE_METATYPE(std::string);
//Q_DECLARE_METATYPE(ScanResultsTreeModelItem);
Q_DECLARE_METATYPE(std::shared_ptr<ScanResultsTreeModelItem>);

#endif /* SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_ */
