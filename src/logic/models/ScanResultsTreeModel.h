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
#include <initializer_list>

// Qt5
#include <QUrl>
#include <QString>

// Ours
#include <utils/QtHelpers.h>
#include "ColumnSpec.h"
#include "ScanResultsTreeModelItem.h"
//#include "AbstractTreeModel.h"
#include "ThreadsafeTreeModel.h"

class AbstractTreeModelHeaderItem;
#include <future/enable_shared_from_this_virtual.h>
#include <models/UndoRedoHelper.h>


/**
 * A ScanResultsTreeModel represents the results of a directory traversal.  Each instance corresponds
 * roughly to what MusicBrainz refers to as a "Medium": @link https://musicbrainz.org/doc/Medium
 * It's essentially a URL to an mp3, ogg, flac, or other audio file which:
 * - Contains 1 or more tracks.
 * - May have a sidecar or embedded cue sheet.
 */
class ScanResultsTreeModel : public ThreadsafeTreeModel, public virtual ISerializable//, public virtual enable_shared_from_this_virtual<ScanResultsTreeModel>
{
	Q_OBJECT
	Q_DISABLE_COPY(ScanResultsTreeModel);
	Q_INTERFACES(ISerializable);

	using BASE_CLASS = ThreadsafeTreeModel;

protected:
//	/**
//	 * The constructed model will NOT have a root, that's what construct() adds.
//	 */
//	explicit ScanResultsTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject *parent = nullptr);

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
//	/**
//	 * Named constructors.
//	 */
//	static std::shared_ptr<ScanResultsTreeModel> construct(std::initializer_list<ColumnSpec> column_specs, QObject *parent = nullptr);
//	/**
//	 * The constructed model will NOT have a root, that's what construct() adds.
//	 */
	explicit ScanResultsTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject *parent = nullptr);
	static std::shared_ptr<AbstractTreeModel> make_ScanResultsTreeModel(std::initializer_list<ColumnSpec> column_specs, QObject* parent);
//	explicit ScanResultsTreeModel(QObject *parent = nullptr);
	ScanResultsTreeModel() = delete;

	~ScanResultsTreeModel() override = default;

    /**
     * Sets the base directory of the model.
     * @todo Not sure if we should support more than one or not, but should support "known alias paths".
     */
    void setBaseDirectory(const QUrl& base_directory);

	/// @name Serialization
	/// @{

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/**
	 * Non-static factory functions for creating new, typed tree nodes from QVariantMaps.
	 */
	UUIncD requestAddScanResultsTreeModelItem(const QVariant& variant, UUIncD parent_id,
								   Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);
	UUIncD requestAddSRTMLibEntryItem(const QVariant& variant, UUIncD parent_id,
									  Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);
	UUIncD requestAddExistingTreeModelItem(std::shared_ptr<AbstractTreeModelItem> new_item, UUIncD parent_id,
										   Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);

#if 0
	void toOrm(std::string filename) const override;
	void fromOrm(std::string filename) override;
#endif

protected:
	/// @name Derived-class serialization info.
	/// @{

	void DERIVED_set_default_namespace() override;

	/// @}


	QTH_FRIEND_QDATASTREAM_OPS(ScanResultsTreeModel);

	/// @}

public Q_SLOTS:


protected:

	QString getXmlStreamName() const override { return "AMLMScanResults"; };
	QString getXmlStreamVersion() const override { return "0.1"; };

	// The tree's base directory URL.
    QUrl m_base_directory;

private:

	/// KDEN KeyFrameModel
	QPersistentModelIndex m_pmindex;
};



#endif // SCANRESULTSTREEMODEL_H
