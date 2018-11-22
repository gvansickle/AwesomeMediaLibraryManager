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
#include "../ISerializable.h"

/*
 *
 */
class ScanResultsTreeModelItem : public AbstractTreeModelItem
{
public:
	explicit ScanResultsTreeModelItem(DirScanResult* dsr, AbstractTreeModelItem *parent = nullptr);
	explicit ScanResultsTreeModelItem(QVector<QVariant> x = QVector<QVariant>(), AbstractTreeModelItem *parent = nullptr);
	 ~ScanResultsTreeModelItem() override;

	/**
	 * Column data override.
	 *
	 * @todo Add role.
	 */
	QVariant data(int column) const override;


	/// @name Serialization
	/// @{

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/**
	 * Parses a new ScanResultsTreeModelItem* out of the passed XML stream.
	 * Returns nullptr if the next parse factory function should be tried.
	 * @param xml
	 * @return
	 */
	static ScanResultsTreeModelItem* parse(QXmlStreamReader* xmlp, AbstractTreeModelItem* parent);

	/**
	 * Write this item and any children to the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 * @returns true
	 */
	bool writeItemAndChildren(QXmlStreamWriter* writer) const override;

//	QXmlQuery write() const;

	/// @} // END Serialization

	static ScanResultsTreeModelItem* createChildItem(AbstractTreeModelItem* parent);

protected:

	/**
	 * Factory function primarily for creating default-constructed nodes.
	 * Used by insertChildren().  Override in derived classes.
	 * @todo Convert to smart pointer (std::unique_ptr<AbstractTreeModelItem>) return type, retain covariant return.
	 */
	ScanResultsTreeModelItem*
	create_default_constructed_child_item(AbstractTreeModelItem *parent = nullptr,
	                                      const QVector<QVariant> &vector = QVector<QVariant>()) override;

	const QString m_item_tag_name = QStringLiteral("scan_res_tree_model_item");

	/// The directory scan results corresponding to this entry.
	/// This is things like the main media URL, sidecar cue sheet URLs, timestamp info, etc.
	DirScanResult m_dsr;

};

#endif /* SRC_LOGIC_MODELS_SCANRESULTSTREEMODELITEM_H_ */
