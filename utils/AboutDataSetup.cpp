//
// Created by gary on 3/17/18.
//

#include "AboutDataSetup.h"

#include <KAboutData>
#include <QString>
#include <QObject> // for tr()

#include <utils/StringHelpers.h>
#include <resources/VersionInfo.h>

/**
 * From KDE5 docs at https://api.kde.org/frameworks/kcoreaddons/html/classKAboutData.html:
 * "Currently, the values set here are shown by the "About" box (see KAboutDialog), used by the bug report dialog (see KBugReport),
 *  and by the help shown on command line (see KAboutData::setupCommandLine())."
 */
KAboutData AboutDataSetup::GetKAboutData()
{
	KAboutData retval {
				"AwesomeMediaLibraryManager", // componentName, "Returns the application's internal name."
				"Awesome Media Library Manager", // displayName, "Returns the translated program name."
				toqstr(VersionInfo::get_version_quad()), // version, "Returns the program's version."
				"Audio Media Library Manager using the Qt 5 GUI framework", // shortDescription
				KAboutLicense::GPL_V3, // licenceType
				"Copyright (c) 2017, 2018 Gary R. Van Sickle", // copyrightStatement, "Returns the copyright statement."
				/// @todo Not sure where this gets used.
				QString(), // otherText
				"https://github.com/gvansickle/AwesomeMediaLibraryManager", // homePageAddress
				"https://github.com/gvansickle/AwesomeMediaLibraryManager/issues" // bugAddress
				};

	// Overwrite default-generated values of organizationDomain & desktopFileName.
	retval.setOrganizationDomain("gvansickle.github.io");
	retval.setDesktopFileName("io.github.gvansickle.awesomemedialibrarymanager");

	// Add me as the author.
	retval.addAuthor(QObject::tr("Gary R. Van Sickle"), // The developer's name. It should be translated.
					QObject::tr("Sole Proprietor"),
					 QString(), // An Email address where the person can be reached. Can be left empty.
					 "https://github.com/gvansickle", // The person's homepage or a relevant link. Start the address with "http://". "http://some.domain" is correct, "some.domain" is not. Can be left empty.
					 QString() // The person's Open Collaboration Services username. The provider can be optionally specified
				);

	return retval;
}
