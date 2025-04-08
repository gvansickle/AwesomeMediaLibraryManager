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
 * @file SRTMItemLibEntry.h
 */
#ifndef SRC_LOGIC_MODELS_SRTMITEMLIBENTRY_H_
#define SRC_LOGIC_MODELS_SRTMITEMLIBENTRY_H_

// Std C++
#include <memory>

// Ours
#include <future/enable_shared_from_this_virtual.h>
#include "ScanResultsTreeModelItem.h"
class LibraryEntry;

/**
 * A ScanResultsTreeModelItem with a LibraryEntry field.
 */
class SRTMItem_LibEntry : public ScanResultsTreeModelItem, public virtual ISerializable, public enable_shared_from_this_virtual<SRTMItem_LibEntry>
{
protected:
	using BASE_CLASS = ScanResultsTreeModelItem;

public:


protected:
    explicit SRTMItem_LibEntry(const std::shared_ptr<AbstractTreeModel>& model = nullptr);
    explicit SRTMItem_LibEntry(std::shared_ptr<LibraryEntry> libentry, const std::shared_ptr<AbstractTreeModel>& model);


public:
    static std::unique_ptr<SRTMItem_LibEntry> create(const std::shared_ptr<AbstractTreeModel>& model = nullptr)
    {
        return std::unique_ptr<SRTMItem_LibEntry>(new SRTMItem_LibEntry(model));
    }
    static std::unique_ptr<SRTMItem_LibEntry> create(std::shared_ptr<LibraryEntry> libentry, const std::shared_ptr<AbstractTreeModel>& model)
    {
        return std::unique_ptr<SRTMItem_LibEntry>(new SRTMItem_LibEntry(libentry, model));
    }
//	static std::shared_ptr<SRTMItem_LibEntry> construct(std::shared_ptr<LibraryEntry> libentry,
//	                                                    const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
//	static std::shared_ptr<SRTMItem_LibEntry> construct(const QVariant& variant,
//	                                                    const std::shared_ptr<AbstractTreeModelItem>& parent = nullptr, UUIncD id = UUIncD::null());
	~SRTMItem_LibEntry() override = default;

	QVariant data(int column, int role = Qt::DisplayRole) const override;

//	int columnCount() const override;

	void setLibraryEntry(std::shared_ptr<LibraryEntry> libentry) { m_library_entry = libentry; };

	/// @name ISerializable interface
	/// @{

	/// Serialize item and any children to a QVariant.
	QVariant toVariant() const override;
	/// Serialize item and any children from a QVariant.
	void fromVariant(const QVariant& variant) override;

	/// @} // END Serialization

private:
	std::string m_key {"key"};
	std::string m_val {"value"};
	std::shared_ptr<LibraryEntry> m_library_entry;
};

Q_DECLARE_METATYPE(SRTMItem_LibEntry);
Q_DECLARE_METATYPE(std::shared_ptr<SRTMItem_LibEntry>);

#endif /* SRC_LOGIC_MODELS_SRTMITEMLIBENTRY_H_ */
