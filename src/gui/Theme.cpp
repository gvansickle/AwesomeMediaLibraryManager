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

#include <config.h>

/// Qt5
#include <QIcon>
#include <QStyle>
#include <QStyleFactory>
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
#include <QMimeType>
#include <QToolButton>

/// KF5
#if HAVE_KF501
#include <KIconLoader>
#include <KActionMenu>
#endif

/// Ours.
#include <AMLMSettings.h>
#include <gui/Theme.h>
#include <gui/MainWindow.h>
#include <utils/DebugHelpers.h>


QStringList Theme::m_available_styles;

/**
 * From https://community.kde.org/Frameworks/Porting_Notes#Application:
 * "- KIcon: has been deprecated and moved to kde4support, prefer QIcon. Port KIcon("foo") to QIcon::fromTheme("foo")
 *  and KIcon("foo", iconLoaderPtr) to QIcon(new KIconEngine("foo", iconLoaderPtr)) using KIconEngine from the
 *  KIconThemes framework.
 *  Note: XDG_DATA_DIRS has to be set to allow icons to be found. (Use kde-dev-scripts/kf5/convert-kicon.pl to automate
 *  most of the conversion. )
 *
 *  - KPixmapSequence now can only be instanced with a fullPath, to use XDG icons use KIconLoader::loadPixmapSequence.
 *
 *  - Use QKeySequence instead of KShortcut to set shortcuts in actions."
 */




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

bool Theme::checkForTestIcon()
{
    M_WARNING("XXXXXX");
    qIn() << "Icon Theme Name:" << QIcon::themeName();
    qIn() << "Icon Theme Search Paths:" << QIcon::themeSearchPaths();
    QString test_icon_name = "folder-open";

    if(QIcon::hasThemeIcon(test_icon_name))
    {
        qDb() << "QIcon:" << QIcon::fromTheme(test_icon_name);
        return true;
    }
    else
    {
        qWr() << "Found no icon named:" << test_icon_name;
        return false;
    }
}

void Theme::dump_resource_tree(const QString &root)
{
    QDirIterator it(root, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        qIn() << it.next();
    }
}

void Theme::initialize()
{
    // Get OS info.

    ///////

    /// This is not directly related with what we're trying to do here.
    /// At startup it's empty anyway.
    /// @see http://doc.qt.io/qt-5/qdir.html#setSearchPaths.
//    qIn() << "Registered resource paths:";
//    for(const auto& respath : QDir::searchPaths("icontheme"))
//    {
//        qIn() << respath;
//    }

    auto app_dir_path = QCoreApplication::applicationDirPath();
    qIn() << "App dir path:" << app_dir_path;

    qIn() << "QStandardPaths::AppDataLocation:";
    auto app_data_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for(const auto& adp : app_data_path)
    {
        qIn() << "  " << adp;
    }

    qIn() << "Initial Icon Theme Search Paths:";
    for(const auto& respath : QIcon::themeSearchPaths())
    {
        qIn() << "  " << respath;
    }
    qIn() << "Initial Icon Theme Fallback Search Paths:";
    for(const auto& respath : QIcon::fallbackSearchPaths())
    {
        qIn() << "  " << respath;
    }

    // Interesting stuff in here by default.
    dump_resource_tree(":/");

    // Load the icon resources.
    int rccs_loaded = 0;
    auto rccs = {app_dir_path + tr("/data/icons/icontheme.rcc"),
                 app_dir_path + tr("/data/icons/icons_oxygen.rcc")};
    for(const auto& fname : rccs)
    {
        // Look for the specified file.
        QString full_path = QStandardPaths::locate(QStandardPaths::AppDataLocation, fname);

        if(full_path.isEmpty())
        {
            qWr() << "Couldn't find icon resource file:" << fname;
            continue;
        }

        bool opened = QResource::registerResource(fname);
        if(!opened)
        {
            qCr() << "FAILED TO OPEN RCC:" << fname;
        }
        else
        {
            qIn() << "Loaded RCC file:" << fname;
            rccs_loaded++;
        }
    }

//    Q_ASSERT(rccs_loaded > 0);

    //    M_WARNING("XXXXXX");
    //    Theme::check


    //////

M_WARNING("TODO");
#if 0

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
#endif
    qIn() << "QPA Platform plugin name:" << qApp->platformName();

    // Add supplementary paths to supplemental_icon_theme_dirs depending on OS.
    /// @note
//    QStringList supplemental_icon_theme_dirs;
//    QStringList bundled_theme_search_paths;
//    if(isWindows())
//    {
//        // Need to add a path so KF5::KIconTheme can get to the bundled icontheme.rcc
//        for(const QString& app_data_path : QStandardPaths::standardLocations(QStandardPaths::AppDataLocation))
//        {
//            bundled_theme_search_paths << app_data_path + "/icons";
//        }
//        qIn() << "Windows, adding paths to ThemeSearchPaths:" << bundled_theme_search_paths;

//        QIcon::setThemeSearchPaths({":/icons/oxygen", ":/icons"});//bundled_theme_search_paths);
//    }

//    M_WARNING("XXXXXX");
    checkForTestIcon();

    /// @todo DONT HARDCODE
    QIcon::setThemeName("oxygen");

    checkForTestIcon();

    QIcon::setThemeName("oxygen-icons");

    checkForTestIcon();

    qDebug() << "Current QIcon::iconThemeName():" << QIcon::themeName();
    qDebug() << "Current QIcon::themeSearchPaths():" << QIcon::themeSearchPaths();

    // Find all the icon themes we have access to.
    QStringList retval = FindIconThemes();
    qIn() << "Discovered Icon Themes:" << retval;


	// Get all the styles we have available.
	m_available_styles = QStyleFactory::keys();
	// Get the current desktop style.
	QString desktop_style = QApplication::style()->objectName();

	if(AMLMSettings::widgetStyle().isEmpty())
	{
		// This is the first program start.
		QStringList styles_to_ignore;
		/// @note If we want to ignore any styles by name, this is where we'd list the names.
//		styles_to_ignore << QStringLiteral("GTK+");

		if(styles_to_ignore.contains(desktop_style, Qt::CaseInsensitive))
		{
			// We don't want/can't use the current desktop style.
			qIn() << "Current desktop style \"" << desktop_style << "\" can't be used, looking for alternatives.";

			// Is Breeze available?
			if(m_available_styles.contains(QStringLiteral("breeze"), Qt::CaseInsensitive))
			{
				// Yes, use it.
				qIn() << "Style Breeze available, using it.";
				AMLMSettings::setWidgetStyle(QStringLiteral("Breeze"));
			}
			else if(m_available_styles.contains(QStringLiteral("fusion"), Qt::CaseInsensitive))
			{
				// Second choice is Fusion, use that.
				qIn() << "Style \"Fusion\" available, using it.";
				AMLMSettings::setWidgetStyle(QStringLiteral("Fusion"));
			}
		}
		else
		{
			// Use the current default desktop widget style.
            qIn() << "Using current desktop widget style:" << desktop_style;
			AMLMSettings::setWidgetStyle(QStringLiteral("Default"));
		}
	}
}

QActionGroup * Theme::getWidgetStylesActionGroup(MainWindow *main_window)
{
	/// Set up a "Style" menu.
	/// Adapted from similar code in Kdenlive::MainWindow::init().

	KActionMenu *stylesAction = new KActionMenu(tr("Style"), main_window);
	QActionGroup *stylesGroup = new QActionGroup(stylesAction);

	// Add a "Default" style action
	QAction *defaultStyle = new QAction(tr("Default"), stylesGroup);
	defaultStyle->setData(QStringLiteral("Default"));
	defaultStyle->setCheckable(true);
	stylesAction->addAction(defaultStyle);
	if (AMLMSettings::widgetStyle() == QLatin1String("Default") || AMLMSettings::widgetStyle().isEmpty())
	{
		// Settings has either nothing or "Default" as the selected style.
		defaultStyle->setChecked(true);
	}

	// Add all available styles to the menu, checking the currently selected one.
	for(const QString &style : m_available_styles)
	{
		QAction *a = new QAction(style, stylesGroup);
		a->setCheckable(true);
		a->setData(style);
		if (AMLMSettings::widgetStyle() == style)
		{
			a->setChecked(true);
		}
		stylesAction->addAction(a);
	}

	return stylesGroup;
}

QString Theme::getUserDefaultStyle(const char* fallback)
{
	KSharedConfigPtr kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
	KConfigGroup cg(kdeGlobals, "KDE");
	return cg.readEntry("widgetStyle", fallback);
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

bool Theme::setIconThemeName(const QString& name)
{
    qDb() << "Trying to set icon theme name to:" << name;

    QIcon::setThemeName(name);

    // Did it take?
    auto current_theme_name = QIcon::themeName();
    if(current_theme_name != name)
    {
        qWr() << "New theme name didn't take:" << name << "!=" << current_theme_name;
    }

#if 0
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
#endif

	return true;
}

QIcon Theme::iconFromTheme(const QString &icon_name)
{
#if 1
    return Theme::iconFromTheme(QStringList(icon_name));
#else
    QIcon retval;

#if HAVE_KF501

//    auto icon_loader = KIconLoader::global();
//    icon_loader->loadIcon()

#endif

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
#endif
}

QIcon Theme::iconFromTheme(const QStringList &icon_names)
{
    QIcon retval;

    // Go through all given names and return the first QIcon we find.
    for(const auto& name : icon_names)
    {
        retval = QIcon::fromTheme(name);
        if(!retval.isNull())
        {
            // Found one.
            qDb() << "Returning theme icon:" << name;
            return retval;
        }
    }

    // Couldn't find it.
    qDb() << "Failed to find icon with any of the names" << icon_names << "in icon theme search paths.";

    return retval;
}

QIcon Theme::iconFromTheme(const QMimeType &mime_type)
{
    // Return an icon from the theme matching the MIME type.
    return iconFromTheme({mime_type.iconName(), mime_type.genericIconName()});
}

void Theme::QToolButtonArrowIconFromTheme(QToolButton *button, const QString &icon_name, Qt::ArrowType arrow_type_fallback)
{
    QIcon up_icon = Theme::iconFromTheme(icon_name);
    if(up_icon.isNull())
    {
        // Use the arrow type instead of a normal icon.
        button->setArrowType(arrow_type_fallback);
    }
    else
    {
        button->setIcon(up_icon);
    }
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
