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
 * @file AbstractTreeModelWriter.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_

// Qt5
#include <QXmlStreamWriter>
class QIODevice;

// Ours
//#include "AbstractTreeModel.h"
//#include "AbstractTreeModelItem.h"
class AbstractTreeModel;
class AbstractTreeModelItem;

/**
 *
 */
class AbstractTreeModelWriter
{
public:
	explicit AbstractTreeModelWriter(const AbstractTreeModel* model);
	virtual ~AbstractTreeModelWriter();

	static inline QString versionAttribute() { return QStringLiteral("version"); }
	static inline QString hrefAttribute() { return QStringLiteral("href"); }
	static inline QString foldedAttribute() { return QStringLiteral("folded"); }

	bool write_to_iodevice(QIODevice* device);

private:

	void write_item(const AbstractTreeModelItem* item);

	QXmlStreamWriter m_xml_stream_writer;

	/// @todo This should be a shared pointer, or we shouldn't store it.
	const AbstractTreeModel* m_tree_model;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELWRITER_H_ */
