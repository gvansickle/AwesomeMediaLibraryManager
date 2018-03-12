/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "Theme.h"

#include <QIcon>
#include <QString>
#include <QDebug>
#include <QApplication>
#include <QImageReader>
#if QT_VERSION_CHECK(5,9,0)
#include <QOperatingSystemVersion>
#else
#include <QSysInfo>
#endif
#include <QDirIterator>
#include <QUrl>
#include <QRegularExpression>

#include "DebugHelpers.h"


static bool isWindows()
{
#if QT_VERSION_CHECK(5,9,0)
	// @note Qt5.9, doesn't work on Linux yet.
	QOperatingSystemVersion os_version = QOperatingSystemVersion::current();
	return os_version.isAnyOfType({QOperatingSystemVersion::Windows});
#else
	return (QSysInfo::kernelType() == "winnt") && (QSysInfo::windowsVersion() & QSysInfo::WV_NT_based);
#endif
}


Theme::Theme(QWidget *parent) : QWidget(parent)
{

}

void Theme::initialize()
{
    // Get OS info.


    QStringList supplemental_icon_theme_dirs;

    // Add supplementary paths to supplemental_icon_theme_dirs depending on OS.
	if(isWindows())
    {
        /// @todo Do we actually need to do anything here anymore?
    }

	QStringList theme_search_paths = {":/icons"};//supplemental_icon_theme_dirs + QIcon::themeSearchPaths();
    QIcon::setThemeSearchPaths(theme_search_paths);

	// @note This always returns /usr/bin under PyQt5.
    auto app_dir = QApplication::applicationDirPath();
    qDebug() << "appDirPath:" << app_dir;

    // See if we can find our bundled Icon theme.  Something's pretty broken if we can't.
    auto original_icon_theme_name = QIcon::themeName();
    auto original_icon_theme_search_paths = QIcon::themeSearchPaths();
    qDebug() << "QImageReader::supportedImageFormats():" << QImageReader::supportedImageFormats();
    qDebug() << "Initial theme search paths:" << original_icon_theme_search_paths;
    qDebug() << "Initial QIcon::themeName():" << original_icon_theme_name;

    QStringList bundled_theme_search_paths({":/icons"});
    QString bundled_theme_name("oxygen-icons");
    QString test_theme_icon_name("folder-open");

	// Replace the default search paths with only the ":/icons" path.
    QIcon::setThemeSearchPaths(bundled_theme_search_paths);
    QIcon::setThemeName(bundled_theme_name);
    if(QIcon::hasThemeIcon(test_theme_icon_name))
    {
        qDebug() << "Found icon" << test_theme_icon_name << "in bundled theme" << bundled_theme_name;
        qDebug() << "QIcon:" << QIcon::fromTheme(test_theme_icon_name);
    }
    else
    {
        qCritical() << "Couldn't find icon " << test_theme_icon_name << "in bundled theme" << bundled_theme_name;
    }
    // Let's also see if we can diretly open one of the icons via the full qrc path.
    QString direct_test_icon_path(":/icons/Tango/scalable/actions/add.svg");
    QIcon test_icon = QIcon(direct_test_icon_path);
    if(test_icon.isNull())
    {
        qCritical() << "Full resource path test icon" << direct_test_icon_path << "can't be loaded";
    }
    else
    {
        qDebug() << "Was able to load icon" << direct_test_icon_path << "directly from resources.";
    }
    // Ok, put everything back the way it was.
//    QIcon::setThemeSearchPaths(original_icon_theme_search_paths);
//    QIcon::setThemeName(original_icon_theme_name);

    // For anything except X11, Qt5.9 docs say:
    // "Note: By default, only X11 will support themed icons. In order to use themed icons on Mac and Windows, you will
    // have to bundle a compliant theme in one of your themeSearchPaths() and set the appropriate themeName()."
    // (http://doc.qt.io/qt-5/qicon.html#fromTheme)
    // We set the bundled icons above, so now we just need to set the QIcon() theme name if necessary.
	if(true) //QIcon::themeName() == "")
    {
        QIcon::setThemeName(bundled_theme_name);
    }

    if(false) // use_only_bundled_icon_theme
    {
        qDebug() << "Using only bundled icon theme:" << bundled_theme_name << "on paths" << bundled_theme_search_paths;
        QIcon::setThemeSearchPaths(bundled_theme_search_paths);
        QIcon::setThemeName(bundled_theme_name);
        if(QIcon::hasThemeIcon(test_theme_icon_name))
        {
            qDebug() << "Found icon" << test_theme_icon_name << "in bundled theme" << bundled_theme_name;
            QIcon test_icon = QIcon::fromTheme(test_theme_icon_name);
            qDebug() << "QIcon:" << test_icon;
            qDebug() << "name: " << test_icon.name();
            qDebug() << "isNull: " << test_icon.isNull();
        }
    }

    qDebug() << "Current iconThemeName():" << QIcon::themeName();
    qDebug() << "Current themeSearchPaths():" << QIcon::themeSearchPaths();
}

QStringList Theme::FindIconThemes()
{
    QStringList retlist;

    for(QString search_dir : QIcon::themeSearchPaths())
    {
        QDirIterator it(QUrl::fromLocalFile(search_dir).toLocalFile(), QStringList({"index.theme"}), QDir::NoFilter, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            //qDebug() << "doFileScan URL:" << it.filePath();
            // Extract the name of the last directory we found the index.theme in.  That is the icon theme name.
            /// @todo Actually it probably isn't.  We should really parse the index.theme files to get the name.
            /// @todo Actually maybe not: setThemeName's docs say:
            /// "The name should correspond to a directory name in the themeSearchPath() containing an index.theme file describing it's contents"
            QString name = it.next();
            QRegularExpressionMatch mo;
            if(name.contains(QRegularExpression(R"!(.*/(.*?)/index.theme)!"), &mo))
            {
                retlist.append(mo.captured(1));
            }
        }
    }

    return retlist;
}

QStringList Theme::GetIconThemeNames()
{
	return FindIconThemes();
}

bool Theme::setThemeName(const QString& name)
{
	///@todo This doen't work like it should
	auto old_theme_name = QIcon::themeName();

	qDebug() << "trying to set Icon Theme from" << old_theme_name << "to" << name;

	QIcon::setThemeName(name);

	if(!QIcon::hasThemeIcon("folder-open"))
	{
		qWarning() << "Unable to set theme name" << name << ", reverting to built-in icon theme" << "oxygen-icons";
		QIcon::setThemeName("oxygen-icons");
		return false;
	}
	return true;
}

QIcon Theme::iconFromTheme(const QString &icon_name)
{
    QIcon retval;

    if(QIcon::hasThemeIcon(icon_name))
    {
        retval = QIcon::fromTheme(icon_name);
    }
    else
    {
        // Don't have the named icon.
        qDebug() << "Failed to find icon with name" << icon_name << "in icon them search paths.";
    }

    return retval;
}
QKeySequence Theme::keySequenceFromTheme(Theme::Key key)
{
    switch(key)
    {
	    case Theme::Key_ToggleShuffle:
		    return QKeySequence("Ctrl+H");
	    break;
	    case Theme::Key_ToggleRepeat:
		    return QKeySequence("Ctrl+T");
	    break;
    }
	return QKeySequence();
}
