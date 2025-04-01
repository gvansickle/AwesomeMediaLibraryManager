/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
				QObject::tr("Awesome Media Library Manager"), // displayName, "Returns the translated program name."
				toqstr(VersionInfo::get_full_version_info_string()), // version, "Returns the program's version."
				"An Audio Media Library Manager using the Qt GUI framework", // shortDescription
				KAboutLicense::Unknown, // licenceType, See addLicense() call below.
				QObject::tr("Copyright (c) 2017, 2018, 2019, 2023, 2024, 2025 Gary R. Van Sickle"), // copyrightStatement, "Returns the copyright statement."
				QObject::tr("Because the world needs a Media Library Manager which is Awesome."), // otherText, added to About box.
				"https://github.com/gvansickle/AwesomeMediaLibraryManager", // homePageAddress
				"https://github.com/gvansickle/AwesomeMediaLibraryManager/issues" // bugAddress
				};

	// Set GPL3-only.
	retval.addLicense(KAboutLicense::GPL_V3, KAboutLicense::OnlyThisVersion);

	// Overwrite default-generated values of organizationDomain & desktopFileName.
	retval.setOrganizationDomain("gvansickle.github.io");
	retval.setDesktopFileName("io.github.gvansickle.awesomemedialibrarymanager");

	// Add me as the author.
	retval.addAuthor(QObject::tr("Gary R. Van Sickle"), // The developer's name. It should be translated.
					QObject::tr("Sole Proprietor"),
					 QStringLiteral("grvs@users.sourceforge.net"), // An Email address where the person can be reached. Can be left empty.
					 QStringLiteral("https://github.com/gvansickle"), // The person's homepage or a relevant link. Start the address with "http://". "http://some.domain" is correct, "some.domain" is not. Can be left empty.
					 QString() // The person's Open Collaboration Services username. The provider can be optionally specified
				);

	return retval;
}
