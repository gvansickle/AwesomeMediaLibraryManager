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

#ifndef SCANRESULTSTREEMODEL_H
#define SCANRESULTSTREEMODEL_H

#include "AbstractTreeModel.h"

// Std C++
#include <shared_mutex>

// Qt5
#include <QUrl>
#include <QString>

// Ours
#include <utils/QtHelpers.h>
#include "ScanResultsTreeModelItem.h"
class AbstractTreeModelHeaderItem;
#include <future/enable_shared_from_this_virtual.h>
#include "UndoRedoHelper.h"


/**
 * A ScanResultsTreeModel represents the results of a directory traversal.  Each instance corresponds
 * roughly to what MusicBrainz refers to as a "Medium": @link https://musicbrainz.org/doc/Medium
 * It's essentially a URL to an mp3, ogg, flac, or other audio file which:
 * - Contains 1 or more tracks.
 * - May have a sidecar or embedded cue sheet.
 */
class ScanResultsTreeModel : public AbstractTreeModel//, public enable_shared_from_this_virtual<ScanResultsTreeModel>
{
	Q_OBJECT

	using BASE_CLASS = AbstractTreeModel;

public:

	static std::shared_ptr<ScanResultsTreeModel> construct(QObject *parent = nullptr);

protected:
	/**
	 * The constructed model will NOT have a root, that's what construct() adds.
	 */
	explicit ScanResultsTreeModel(QObject *parent = nullptr);

public:
    ~ScanResultsTreeModel() override = default;

    /**
     * Sets the base directory of the model.
     * @todo Not sure if we should support more than one or not, but should support "known alias paths".
     */
    void setBaseDirectory(const QUrl& base_directory);


	/// Append a vector of AbstractTreeModelItem's as children of @p parent.
//	bool appendItems(std::vector<std::shared_ptr<AbstractTreeModelItem>> new_items, const QModelIndex &parent = QModelIndex()) override;

	/// @todo Push these down?
	bool requestAppendItem(const std::shared_ptr<ScanResultsTreeModelItem>& item, UUIncD parent_uuincd, Fun& undo, Fun& redo);
	bool requestAppendItems(std::vector<std::shared_ptr<ScanResultsTreeModelItem>> items, UUIncD parent_uuincd, Fun& undo, Fun& redo);
	bool requestAddScanResultsTreeModelItem(const DirScanResult& dsr, UUIncD parent_uuincd, Fun& undo, Fun& redo);

	/// @name Serialization
	/// @{

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	QTH_FRIEND_QDATASTREAM_OPS(ScanResultsTreeModel);

	/// @}

protected:
	/**
	 * Adds @a item to this tree model.
	 */
	/// ~KDenLive
	bool addItem(const std::shared_ptr<ScanResultsTreeModelItem>& item, UUIncD parent_uuincd, Fun& undo, Fun& redo);

	QString getXmlStreamName() const override { return "AMLMScanResults"; };
	QString getXmlStreamVersion() const override { return "0.1"; };

	// The tree's base directory URL.
    QUrl m_base_directory;

private:

	/**
	 * Single writer/multi-reader mutex.
	 * @todo The KDenLive code has/needs this to be recursive, but we should try to un-recurse it.
	 */
//	mutable std::shared_mutex m_rw_mutex;
	mutable std::recursive_mutex m_rw_mutex;
};


#endif // SCANRESULTSTREEMODEL_H
