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

// Std C++
#include <tuple>
#include <string>

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
#include <KIconTheme>
#include <KActionMenu>
#endif

/// Ours.
#include <AMLMSettings.h>
#include <gui/Theme.h>
#include <gui/MainWindow.h>
#include <utils/DebugHelpers.h>


QStringList Theme::m_available_qstyles;

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

/**
 * The Icon Theme Tragedy
 *
 * Notes:
 *
 * From @link https://api.kde.org/frameworks/kiconthemes/html/classKIconLoader.html#adeaa3967ffbbfb424aee7d335c26fe24:
 * "The icons are stored on disk in an icon theme or in a standalone directory. The icon theme directories contain multiple sizes
 * and/or depths for the same icon. The iconloader will load the correct one based on the icon group and the current theme. Icon
 * themes are stored globally in share/icons, or, application specific in share/apps/$appdir/icons.
 * The standalone directories contain just one version of an icon. The directories that are searched are: $appdir/pics and
 * $appdir/toolbar. Icons in these directories can be loaded by using the special group "User"."
 *
 * No idea if that's accurate, or even where the referred-to dirs even are.
 *
 * Looking at the code for KIconTheme.cpp::initRCCIconTheme(), it does this from a Q_COREAPP_STARTUP_FUNCTION():
 * - const QString iconThemeRcc = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("icontheme.rcc"));
 * - if not found, silent return.
 * - if found, Calls QResource::registerResource(iconThemeRcc, iconSubdir) with:
 * -- iconThemeRcc = path returned by locate above.
 * -- iconSubdir == /icons/kf5_rcc_theme  << Note not ":/".
 * - Looks for an index.theme in the file.
 * - Two possible warnings:
 * -- "No index.theme found in" << iconThemeRcc;
 * -- "Invalid rcc file" << iconThemeRcc;
 *
 * From the Qt docs, QStandardPaths::AppDataLocation defaults:
 * Linux:
 * - "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
 * -- It appears that $XDG_DATA_DIRS from the startup environment might also get added to this.  KDE's prefix.sh
 *    prepends "/..../install/share" to that var, and AppDataLocation then comes back with this path added to the list:
 *    "/..../install/share/gvansickle/AwesomeMediaLibraryManager".
 *    Looking at @link https://github.com/RSATom/Qt/blob/master/qtbase/src/corelib/io/qstandardpaths_unix.cpp,
 *    QStandardPaths::standardLocations() does in fact pick up that var, then appends "appendOrganizationAndApp" to every
 *    entry.
 * Windows:
 * - "C:/Users/<USER>/AppData/Roaming/<APPNAME>", "C:/ProgramData/<APPNAME>", "<APPDIR>", "<APPDIR>/data", "<APPDIR>/data/<APPNAME>"
 * -- Again it appears that appendOrganizationAndApp() will be appended to all of these.
 * appendOrganizationAndApp() specifically appends "QCoreApplication::organizationName()/QCoreApplication::applicationName()".
 * For this app, that's "/gvansickle/AwesomeMediaLibraryManager".
 * If we want to augment this path for KIconLoader, we have to do so prior to the QCoreApplication constructor,
 * since it is what runs the Q_COREAPP_STARTUP_FUNCTION()s.
 */


static bool isWindows()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,9,0)
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
    qIn() << "Icon Theme Name:" << QIcon::themeName();
    qIn() << "Icon Theme Search Paths:" << QIcon::themeSearchPaths();
    QString test_icon_name = "folder-open";

    if(QIcon::hasThemeIcon(test_icon_name))
    {
        qDb() << "Found icon named " << test_icon_name << ":" << QIcon::fromTheme(test_icon_name);
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
    qIn() << "START Initializing Theme";

    auto app_dir_path = QCoreApplication::applicationDirPath();
    qIn() << "App dir path:" << app_dir_path;

	// Log QStandardPaths::AppDataLocation, it's particularly critical and problematic.
	// Among other things, our icon *.rcc files should be under at least one of these directories.
	// When debugging/running the program, even when built+installed, these paths aren't automatically correct.
	// The built-source "prefix.sh" file is intended to take care of that, at least on Linux, at gdb-time.
	// For icons in particular, the env var "XDG_DATA_DIRS" is critical here.  prefix.sh prepends "<builddir>/<installdir>/share" to it,
	// which appears to be enough to both add it to this list of paths, and add it to QIcon::themeSearchPaths(), where it gets
	// the "/icons" dir appended to it (not sure what's doing that, the QPA?).

    qIn() << "QStandardPaths::AppDataLocation:";
    auto app_data_path = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    log_QStringList(app_data_path, qInfo());

    qIn() << "Initial Icon Theme Search Paths:";
    log_QStringList(QIcon::themeSearchPaths(), qInfo());

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    // QIcon::fallbackSearchPaths() Introduced in Qt5 5.11.0.
    qIn() << "Initial Icon Theme Fallback Search Paths:";
	auto fsp = QIcon::fallbackSearchPaths().toStdList();
	for(const auto& respath : fsp)
    {
        qIn() << "  " << respath;
    }
#endif

    LogIconThemeInfo();

#if 0 /// @todo KIconLoader should be handling icontheme.rcc.
    // Load the bundled icon resources.
    int rccs_loaded = 0;
    // Filename/resource map root pairs
    using string_pair = std::tuple<std::string, std::string>;
    using string_pair_list = std::vector<string_pair>;
    const string_pair_list rccs {
        {"icontheme.rcc", ":/icons/breeze"}
        /*, "icons_oxygen.rcc"*/};
    for(const string_pair& rcc_entry : rccs)
    {
        // Look for the specified file.
        std::string fname, map_root;
        std::tie(fname, map_root) = rcc_entry;

        QString full_path;
        if(true /** @fixme */)
        {
        	/**
        	 * @todo FIXME For finding the icons when built and installed, but not installed on system.
        	 * This is what prefix.sh is for, Windows equivalent?
        	 */

            //full_path = app_dir_path + "/../share/icons/" + fname;
            full_path = QStandardPaths::locate(QStandardPaths::AppDataLocation, toqstr(fname));
        }
        else
        {
            full_path = app_dir_path + "/../share/icons/" + toqstr(fname);
            if(!QFile::exists(full_path))
            {
                qDb() << "No file at:" << full_path;
                full_path.clear();
            }
        }

        if(full_path.isEmpty())
        {
            qWr() << "Couldn't locate icon resource file:" << fname;
            continue;
        }
        else
		{
			qIn() << "Located resource file:" << fname << "found at absoulte path:" << full_path;
		}

        bool opened = QResource::registerResource(full_path, toqstr(map_root));
        if(!opened)
        {
            qCr() << "FAILED TO OPEN RCC:" << full_path;
        }
        else
        {
            qIn() << "Loaded RCC file:" << full_path;
            rccs_loaded++;
        }
    }

    Q_ASSERT(rccs_loaded > 0);
#endif
    // Interesting stuff in here by default.
	dump_resource_tree(":/");

    LogIconThemeInfo();


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
//    QIcon::setThemeName("oxygen");

//    checkForTestIcon();

//    LogIconThemeInfo();

//    QIcon::setThemeName("oxygen-icons");

//    checkForTestIcon();

    LogIconThemeInfo();

    qIn() << "Current QIcon::themeSearchPaths():";
    log_QStringList(QIcon::themeSearchPaths(), qInfo());

    LogIconThemeInfo();

    // Find all the icon themes we have access to.
    QStringList retval = FindIconThemes();
    qIn() << "Discovered Icon Themes:" << retval;


    // Get all the QStyle styles we have available.
    m_available_qstyles = QStyleFactory::keys();
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
            if(m_available_qstyles.contains(QStringLiteral("breeze"), Qt::CaseInsensitive))
			{
				// Yes, use it.
				qIn() << "Style Breeze available, using it.";
				AMLMSettings::setWidgetStyle(QStringLiteral("Breeze"));
			}
            else if(m_available_qstyles.contains(QStringLiteral("fusion"), Qt::CaseInsensitive))
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
    qIn() << "END Initializing Theme";
}

QActionGroup * Theme::getWidgetStylesActionGroup(MainWindow *main_window)
{
	/// Set up a "Style" menu.
	/// Adapted from similar code in Kdenlive::MainWindow::init().

    KActionMenu *stylesAction = new KActionMenu(tr("Widget Style"), main_window);
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
	for(const QString &style : qAsConst(m_available_qstyles))
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

QString Theme::getUserDefaultQStyle(const char* fallback)
{
	KSharedConfigPtr kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
	KConfigGroup cg(kdeGlobals, "KDE");
	return cg.readEntry("widgetStyle", fallback);
}

QStringList Theme::FindIconThemes()
{
#if HAVE_KF501
    // List all icon themes installed on the system, global and local.
    QStringList retlist = KIconTheme::list();
#else
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
#endif

    return retlist;
}

QStringList Theme::GetIconThemeNames()
{
    return FindIconThemes();
}

QString Theme::defaultIconThemeName()
{
    return KIconTheme::defaultThemeName();
}

QString Theme::currentIconThemeName()
{
    return KIconTheme::current();
}

void Theme::LogIconThemeInfo()
{
    QString default_kicon_theme_name = defaultIconThemeName();
    QString current_kicon_theme_name = currentIconThemeName();
    QString current_qicon_theme_name = QIcon::themeName();

    // Get the app's global icon loader.
    KIconLoader* kicon_loader = KIconLoader::global();
    /// @todo Is this of any use to us?: kicon_loader->addAppDir();
    KIconTheme* current_kicon_theme = kicon_loader->theme();
    QString cur_desc = current_kicon_theme->description();

    qIn() << M_NAME_VAL(default_kicon_theme_name);
    qIn() << M_NAME_VAL(current_kicon_theme_name) << "Desc:" << cur_desc
          << "Human readable name:" << current_kicon_theme->name()
          << "Internal name:" << current_kicon_theme->internalName()
          << "Dir:" << current_kicon_theme->dir()
          << "Example Icon Name:" << current_kicon_theme->example();
    qIn() << M_NAME_VAL(current_qicon_theme_name);

    AMLM_WARNIF(current_qicon_theme_name != current_kicon_theme_name);
}

void Theme::LogAllIconThemes()
{
    // List all icon themes installed on the system, global and local.
    QStringList all_icon_themes = GetIconThemeNames();

    qIn() << "All Icon Themes:";
    for(const auto& i : all_icon_themes)
    {
        qIn() << "  " << i;
    }
}

bool Theme::setIconThemeName(const QString& name)
{
    qIn() << "Trying to set icon theme name to:" << name;

    LogIconThemeInfo();

    QIcon::setThemeName(name);

    KIconTheme::reconfigure();

    // Did it take?

    // Check by QIcon name.
    auto current_qicon_theme_name = QIcon::themeName();
    if(current_qicon_theme_name != name)
    {
        qWr() << "New theme name didn't take:" << name << "!=" << current_qicon_theme_name;
    }

    // Check with KIconLoader.
    KIconLoader* kicon_loader = KIconLoader::global();
    KIconTheme* current_kicon_theme = kicon_loader->theme();
    qIn() << "KIconTheme name:" << current_kicon_theme->name();
    qIn() << "KIconTheme internal name:" << current_kicon_theme->internalName();
    QString example_icon_name = current_kicon_theme->example();
    qIn() << "KIconTheme example icon name:" << example_icon_name;
    if(QIcon::hasThemeIcon(example_icon_name))
    {
        qIn() << "Found example icon:" << example_icon_name << ", trying to load it.";
        QIcon example_icon = QIcon::fromTheme(example_icon_name);
        if(example_icon.isNull())
        {
            qCr() << "Couldn't load example icon:" << example_icon_name;
        }
        else
        {
            qIn() << "Successfully loaded example icon:" << example_icon;
        }
    }


    LogIconThemeInfo();

	return true;
}

// Static
QIcon Theme::iconFromTheme(const QString &icon_name)
{
    return Theme::iconFromTheme(QStringList(icon_name));
}

// Static
QIcon iconFromTheme(const QString& icon_name, const QIcon& fallback)
{
    return QIcon::fromTheme(icon_name, fallback);
}

// Static
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
//            qDb() << "Returning theme icon:" << name;
            return retval;
        }
    }

    // Couldn't find it.
//    qWr() << "Failed to find icon with any of the names" << icon_names << "in icon theme search paths.";

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
