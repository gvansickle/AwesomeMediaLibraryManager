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
 * @file ScanResultsTreeModelItem.cpp
 */

#include "ScanResultsTreeModelItem.h"

// Qt5
#include <QXmlStreamReader>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/DirScanResult.h>

//template <class T>
//struct SerializationDescriptorWriters
//{
//	static T& String(const T& outstr, const SerializationDescriptor& item)
//	{
////		char* endptr = 0;
////		if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
////		if (endptr != option.arg && *endptr == 0)
////		  return lmcppop::ARG_OK;
//
////		if (msg) printError("Option '", option, "' requires a numeric argument\n");
////		return lmcppop::ARG_ILLEGAL;
//	}
//};
//
//template <class OutSerializerRef>
//struct SerializationDescriptor
//{
//	const std::string m_tag_name;
//	const std::function<OutSerializerRef(OutSerializerRef, const SerializationDescriptor&)> m_xml_writer;
////	const unsigned m_index;
////	const int m_type;
////	const int m_notype {0};
////	const char* const m_shortopts;
////	const char* const m_longopts;
////	const char *const m_argname;
////	const lmcppop::CheckArg m_check_arg;
////	const std::string m_help;
////	const bool m_is_hidden { false };
////	const bool m_is_bracket_no { false };
//
////	struct section_header_tag {};
//	struct normal_element_tag {};
////	struct arbtext_tag {};
////	struct hidden_tag {};
//	struct attribute_tag {};
//
//	/**
//	 * Constructor overload for generic tags.
//	 *
//	 * @param tag_name  Text of the section header.
//	 */
//	SerializationDescriptor(const char *tag_name, normal_element_tag = normal_element_tag()) noexcept
//		: m_tag_name(tag_name), m_xml_writer(Arg::None))
//	{
//	};
//
//	/**
//	 * Atrribute list.
//	 */
//	SerializationDescriptor(const char *attr_name, attribute_tag = attribute_tag()) noexcept
//	{
//
//	};
//};
//
//static std::vector<SerializationDescriptor> f_tree_model_item_description {
//	{"exturl", "href"}
//};

ScanResultsTreeModelItem::ScanResultsTreeModelItem(DirScanResult* dsr, AbstractTreeModelItem* parent)
	: AbstractTreeModelItem(parent)
{
	m_dsr = *dsr;
//	dsr->getDirProps();
//	dsr->getMediaExtUrl();
//	dsr->getSidecarCuesheetExtUrl();
}

ScanResultsTreeModelItem::ScanResultsTreeModelItem(QVector<QVariant> x, AbstractTreeModelItem *parent)
	: AbstractTreeModelItem(x, parent)
{

}

ScanResultsTreeModelItem::~ScanResultsTreeModelItem()
{
	// TODO Auto-generated destructor stub
}

ScanResultsTreeModelItem* ScanResultsTreeModelItem::parse(QXmlStreamReader* xmlp, AbstractTreeModelItem* parent)
{
	auto& xml = *xmlp;

	if(xml.name() != "srtmitem")
	{
		// Not fur us.
		return nullptr;
	}
	else
	{
		// Read in this and all children.
		auto* this_node = createChildItem(parent);

		while(xml.readNextStartElement())
		{
			// Read the columns.
			if(xml.name() == "column_data")
			{
				qDb() << "### column_data:" << xml.readElementText();
			}
			else
			{
				xml.skipCurrentElement();
			}
		}

		return this_node;
	}
}

bool ScanResultsTreeModelItem::writeItemAndChildren(QXmlStreamWriter* writer) const
{
	// Convenience ref.
	auto& xml = *writer;

	/// @todo Check if we're the root item?

	// Write out this item.

	xml.writeStartElement(m_item_tag_name);
	xml.writeAttribute("childNumber", QString("%1").arg(childNumber()));

	// Write the DirScanResults.
	auto dsr = m_dsr.toXml();
	dsr.write(&xml);

	// Write the columns of data.
//	for(int col = 0; col < columnCount(); ++col)
//	{
//		/// @todo Get header element info.
////		xml.writeAttribute("childNumber", QString("%1").arg(item->childNumber()));
//		xml.writeTextElement("column_data", data(col).toString());
//	}

	// Write out all children.
	for(int i = 0; i < childCount(); ++i)
	{
		// Hold on tight, we're going recursive!
		child(i)->writeItemAndChildren(writer);
	}
	xml.writeEndElement();

	/// @todo Default to something else if not overridden?
	return true;
}

QXmlQuery ScanResultsTreeModelItem::write() const
{
	QXmlQuery query;

	query.bindVariable("tagname", QVariant(m_item_tag_name));
	query.bindVariable("m_dsr", QVariant::fromValue<DirScanResult>(m_dsr));
//	query.bindVariable("inner_var", query_inner);
	query.setQuery(
				"<{$tagname}>"
				"<m_dsr>{$m_dsr}</m_dsr>"
				"</{$tagname}>"
				);
	Q_ASSERT(query.isValid());

	return query;
}

ScanResultsTreeModelItem* ScanResultsTreeModelItem::createChildItem(AbstractTreeModelItem* parent)
{
	ScanResultsTreeModelItem* child_item;

	if(parent)
	{
		child_item = new ScanResultsTreeModelItem(QVector<QVariant>(), parent);
	}
	else
	{
		child_item = new ScanResultsTreeModelItem();
	}

	return child_item;
}

