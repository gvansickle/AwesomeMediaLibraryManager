//
// Created by gary on 11/26/18.
//

#ifndef AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
#define AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H

// Std C++
#include <map>
#include <string>

// Qt5
#include <QObject>
#include <QtXml>

class ScanResultsTreeModelXMLTags
{

};

class DSRTag
{
	Q_GADGET
public:
	enum TagName
	{
		EXTURL_DIR,
		EXTURL_MEDIA,
		EXTURL_CUESHEET
	};
	Q_ENUM(TagName)

	/**
	 * Forwards DSRTag [] operator to the .at() function, so we don't populate the map but rather throw.
	 */
	const std::string operator[](DSRTag::TagName i) { return m_dsrtag_to_string.at(i); };

protected:
	static std::map<DSRTag::TagName, std::string> m_dsrtag_to_string;

};



#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
