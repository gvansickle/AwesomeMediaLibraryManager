
message(STATUS "Entering CPackHelper...")


if(INCLUDE_QT_DLL_IN_BIN_PACKAGE OR NOT QT_CONFIG MATCHES "static")
	include( InstallRequiredSystemLibraries )
endif(INCLUDE_QT_DLL_IN_BIN_PACKAGE OR NOT QT_CONFIG MATCHES "static")

function(CPACKIFW_COMMON)
        set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
        set(CPACK_PACKAGE_VENDOR "Gary R. Van Sickle")
        set(CPACK_IFW_PACKAGE_PUBLISHER "gvansickle")
        set(CPACK_PACKAGE_FILE_NAME "${PRIMARY_TARGET_INSTALLER}")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} Installation Tool")
	set(CPACK_PACKAGE_VERSION "0.0.1") # Version of installer
        set(CPACK_COMPONENTS_ALL the_installer_comp)
        # Default below is PROJECT_NAME<space>VERSION, so this gets us CPACK_IFW_TARGET_DIRECTORY="c:\Program Files\${PROJECT_NAME}" on windows.
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
        set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "${PROJECT_NAME}")
        set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "${PROJECT_NAME}")
        # QtIFW only comes in a 32-bit Windows flavor right now, which inexplicably installs even 64-bit exes to "Program Files (x86)".  So force the X64 install dir.
        set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/${CPACK_PACKAGE_INSTALL_DIRECTORY}")
	set(CPACK_GENERATOR IFW)
	set(CPACK_IFW_VERBOSE ON)

        message_cpack_summary()

	include(CPack REQUIRED)
	include(CPackIFW REQUIRED)

        message_cpack_summary()

	cpack_add_component(
                    the_installer_comp
                        DISPLAY_NAME "${PROJECT_NAME} - The Awesome Media Library Manager"
                        DESCRIPTION "The main Awesome Media Library Manager program"
			REQUIRED
			)

	cpack_ifw_configure_component(
                the_installer_comp
                FORCED_INSTALLATION # Required to be installed
                NAME "io.github.gvansickle.root"  # "is used to create domain-like identification for this component. By default used origin component name."
		VERSION ${PROJECT_VERSION} # Version of component
                LICENSES GPL3 ${${PROJECT_NAME}_SOURCE_DIR}/LICENSE
                SCRIPT package/qtifw/create_start_menu_shortcuts.qs
		DEFAULT TRUE
		)
endfunction()

if(CPACK_IFW_ROOT OR DEFINED ENV{QTIFWDIR})
	if(DEFINED ENV{QTDIR})
		if(APPLE)
			if(EXISTS $ENV{QTDIR}/bin/macdeployqt)
				message("*")
				message("* Note")
				message("*")
				message("* Because of a bug in CPackIFW, it doesn't manage correctly the package created on OSX.")
				message("* Unfortunately CPackIFW doesn't forward the .dmg extension to the binarycreator (see IFW documentation for more details).")
				message("* Therefore it creates a .app directory on OSX, that isn't properly a file and cannot be treated the same way .run and .exe files are.")
				message("* As a result, make package generates an empty directory within the build directory and leaves the bundle somewhere within the _CPack_Packages directory.")
				message("*")
				message("* I strongly suggest to run binarycreator through an external script and use the .dmg extension directly. At least until the bug in CPackIFW isn't fixed.")
				message("* From within the build directory run:")
				message("*")
				message("*     binarycreator -f -c _CPack_Packages/Darwin/IFW/installer/config/config.xml -p _CPack_Packages/Darwin/IFW/installer/packages/ package/installer.dmg")
				message("*")
				message("* The bundle and the dmg file will be created within the package directory as:")
				message("*")
				message("*     package/installer.app [bundle]")
				message("*     package/installer.dmg [dmg]")
				message("*")

				add_custom_command(
					    TARGET ${PRIMARY_TARGET} POST_BUILD
						COMMAND $ENV{QTDIR}/bin/macdeployqt ${PROJECT_NAME}.app
						)

					CPACKIFW_COMMON()
			else()
				message("Unable to find executable QTDIR/bin/macdeployqt.")
			endif()
		elseif(WIN32)
			message(STATUS "Setting up Windows installation.  Installer will be named ${PRIMARY_TARGET_INSTALLER}")
			if(EXISTS $ENV{QTDIR}/bin/windeployqt.exe)
				add_custom_command(
					    TARGET ${PRIMARY_TARGET} POST_BUILD
						COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/windeployqt_stuff
						COMMAND $ENV{QTDIR}/bin/windeployqt.exe --compiler-runtime --dir ${CMAKE_BINARY_DIR}/windeployqt_stuff $<TARGET_FILE:AwesomeMediaLibraryManager>
						)

					install(
						DIRECTORY ${CMAKE_BINARY_DIR}/windeployqt_stuff/
						DESTINATION ${PROJECT_NAME}
                                                COMPONENT the_installer_comp
						)

					CPACKIFW_COMMON()
				message(STATUS "Setting up Windows installation complete.  Installer will be named ${PRIMARY_TARGET_INSTALLER}")
			else()
				message("Unable to find executable QTDIR/bin/windeployqt.")
			endif()
		elseif(UNIX)
			CPACKIFW_COMMON()
		endif()
	else()
		message("Set properly environment variable QTDIR to be able to create a package.")
	endif()
else()
	message("Did not find QtIFW.")
	message("If you want to enable target package you can:")
	message("\t* Either pass -DCPACK_IFW_ROOT=<path> to cmake")
	message("\t* Or set the environment variable QTIFWDIR")
	message("To specify the location of the QtIFW tool suite.")
	message("The specified path should not contain bin at the end (for example: D:\\DevTools\\QtIFW2.0.5).")
endif()


message(STATUS "Leaving CPackHelper.")
