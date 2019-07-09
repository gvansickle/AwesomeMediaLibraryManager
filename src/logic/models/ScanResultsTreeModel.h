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

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/**
	 * Non-static factory function for creating new, typed tree nodes from QVariantMaps.
	 */
	std::shared_ptr<AbstractTreeModelItem>
	make_item_from_variant(const QVariant& variant) override;

	UUIncD requestAddTreeModelItem(const QVariant& variant, UUIncD parent_id,
	                               Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda) override;

	virtual UUIncD requestAddExistingTreeModelItem(std::shared_ptr<AbstractTreeModelItem> item, UUIncD parent_id,
								   Fun undo = noop_undo_redo_lambda, Fun redo = noop_undo_redo_lambda);

	QTH_FRIEND_QDATASTREAM_OPS(ScanResultsTreeModel);

	/// @}

public Q_SLOTS:


protected:
	QString getXmlStreamName() const override { return "AMLMScanResults"; };
	QString getXmlStreamVersion() const override { return "0.1"; };

	// The tree's base directory URL.
    QUrl m_base_directory;

};

#endif // SCANRESULTSTREEMODEL_H
