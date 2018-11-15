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
 * @file AbstractTreeModelReader.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELREADER_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELREADER_H_

// Qt5
class QIODevice;
#include <QXmlStreamReader>

// Ours
class AbstractTreeModel;
class AbstractTreeModelItem;



/*
 *
 */
class AbstractTreeModelReader
{
public:
	explicit AbstractTreeModelReader(AbstractTreeModel* model);
	virtual ~AbstractTreeModelReader();

	bool read(QIODevice *device);

	QString errorString() const;

	static inline QString versionAttribute() { return QStringLiteral("version"); }
	static inline QString hrefAttribute() { return QStringLiteral("href"); }
	static inline QString foldedAttribute() { return QStringLiteral("folded"); }

private:
	QXmlStreamReader m_xml_stream_reader;
	AbstractTreeModel *m_model;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELREADER_H_ */
