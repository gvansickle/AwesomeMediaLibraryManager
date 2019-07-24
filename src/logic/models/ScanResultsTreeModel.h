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

// Std C++
#include <shared_mutex>

// Qt5
#include <QUrl>
#include <QString>

// Ours
#include <utils/QtHelpers.h>
#include "ScanResultsTreeModelItem.h"
//#include "AbstractTreeModel.h"
#include "ThreadsafeTreeModel.h"

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
class ScanResultsTreeModel : public ThreadsafeTreeModel//, public virtual enable_shared_from_this_virtual<ScanResultsTreeModel>
{
	Q_OBJECT
	Q_DISABLE_COPY(ScanResultsTreeModel);
	Q_INTERFACES(ISerializable);

	using BASE_CLASS = ThreadsafeTreeModel;

protected:
	/**
	 * The constructed model will NOT have a root, that's what construct() adds.
	 */
	explicit ScanResultsTreeModel(QObject *parent = nullptr);

	/**
	 * Make sig/slot connections.
	 */
	void setup();


	/**
	 * Commit the modification to the model.
	 * Note that this is really a slot, but not marked as such in KDENLive
	 */
	void sendModification();

public:
	static std::shared_ptr<ScanResultsTreeModel> construct(QObject *parent = nullptr);
	~ScanResultsTreeModel() override = default;

    /**
     * Sets the base directory of the model.
     * @todo Not sure if we should support more than one or not, but should support "known alias paths".
     */
    void setBaseDirectory(const QUrl& base_directory);

	/// @name Serialization
	/// @{

	/// Load and save the database to a file.
//	void LoadDatabase(const QString& database_filename);
//	void SaveDatabase(const QString& database_filename);


	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/**
	 * Non-static factory functions for creating new, typed tree nodes from QVariantMaps.
	 */
	UUIncD requestAddScanResultsTreeModelItem(const QVariant& variant, UUIncD parent_id,
								   Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);
	UUIncD requestAddSRTMLibEntryItem(const QVariant& variant, UUIncD parent_id,
									  Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);

	void toOrm(std::string filename) const override;
	void fromOrm(std::string filename) override;


	QTH_FRIEND_QDATASTREAM_OPS(ScanResultsTreeModel);

	/// @}

public Q_SLOTS:


protected:

	/// Thread-safe overrides.
//	void register_item(const std::shared_ptr<AbstractTreeModelItem>& item) override;
//	void deregister_item(UUIncD id, AbstractTreeModelItem* item) override;

	/**
	 * Adds @a item to this tree model.
	 * ~KDenLive
	 * This is the workhorse threadsafe function which adds all new items to the model.  It should be not be called by clients,
	 * but rather called by one of the requestAddXxxx() members.
	 */
//	bool addItem(const std::shared_ptr<ScanResultsTreeModelItem>& item, UUIncD parent_uuincd, Fun& undo, Fun& redo);

	QString getXmlStreamName() const override { return "AMLMScanResults"; };
	QString getXmlStreamVersion() const override { return "0.1"; };

	// The tree's base directory URL.
    QUrl m_base_directory;

private:

	/// KDEN KeyFrameModel
	QPersistentModelIndex m_pmindex;
};



#endif // SCANRESULTSTREEMODEL_H
