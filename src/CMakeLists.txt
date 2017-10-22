# This file is part of inputmangler, a programm which intercepts and
# transforms linux input events, depending on the active window.
# Copyright (C) 2014-2017 Arkadiusz Guzinski <kermit@ag.de1.cc>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#project(inputmangler)
#cmake_minimum_required(VERSION 2.8.11)


find_package(Qt5Core)
find_package(Qt5DBus)
find_package(Qt5Network)
find_package(Qt5Test)
#find_package(X11)
#find_package(Qt5Widgets)
#SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-lX11")
#SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-lX11")


# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

FILE(GLOB handlers_srcs "${CMAKE_CURRENT_SOURCE_DIR}/handlers/*.cpp")
FILE(GLOB output_srcs "${CMAKE_CURRENT_SOURCE_DIR}/output/*.cpp")
set(INPUTMANGLER_SRCS
        imdbusinterface.cpp
        inputevent.cpp
        inputmangler.cpp
        keydefs.cpp
        main.cpp
        output/outevent.cpp
        qtsignalhandler.cpp
        transformationstructure.cpp
        ${handlers_srcs}
        ${output_srcs}
        ConfParser.cpp
		)

find_library(PUGIXML_LIB pugixml)
#add_library(pugixml SHARED IMPORTED)
add_executable(inputmangler ${INPUTMANGLER_SRCS})
target_link_libraries(inputmangler pugixml)

# files to install
install(TARGETS inputmangler RUNTIME DESTINATION bin)
FILE(GLOB keymaps "${CMAKE_CURRENT_SOURCE_DIR}/../keymaps/*")
INSTALL(FILES ${keymaps} DESTINATION share/inputmangler/keymaps/)
FILE(GLOB doc "${CMAKE_CURRENT_SOURCE_DIR}/../doc/release/*")
INSTALL(FILES ${doc} DESTINATION share/doc/inputmangler/)
FILE(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/*")
INSTALL(PROGRAMS ${scripts} DESTINATION share/inputmangler/)

INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwinNotifyOnWindowChange/contents/code/main.js" DESTINATION "share/kde4/apps/kwin/scripts/kwinNotifyOnWindowChange/contents/code/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwinNotifyOnWindowChange/metadata.desktop" DESTINATION "share/kde4/apps/kwin/scripts/kwinNotifyOnWindowChange/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwin-script-notifyonwindowchange.desktop" DESTINATION "share/kde4/services/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwinNotifyOnWindowChange/contents/code/main.js" DESTINATION "share/kwin/scripts/kwinNotifyOnWindowChange/contents/code/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwinNotifyOnWindowChange/metadata.desktop" DESTINATION "share/kwin/scripts/kwinNotifyOnWindowChange/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/kwinscript/kwin-script-notifyonwindowchange.desktop" DESTINATION "share/kservices5/")

qt5_use_modules(inputmangler Core DBus Network Test)


#add_definitions( -DDEBUGME )

SET(CPACK_GENERATOR "DEB") # TBZ possibly broken -> https://cmake.org/Wiki/CMake:CPackPackageGenerators
#SET(CPACK_GENERATOR "DEB;RPM,TBZ2") # TBZ possibly broken -> https://cmake.org/Wiki/CMake:CPackPackageGenerators

# General package info
SET(CPACK_PACKAGE_NAME "inputmangler")
SET(CPACK_PACKAGE_VERSION "2.0.0")
SET(CPACK_PACKAGE_CONTACT "Arkadiusz Guzinski <kermit@ag.de1.cc>")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon that transforms input events.")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/meta/description")

# Debian
execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE TMPOUT)
string(STRIP ${TMPOUT} CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
SET(CPACK_SYSTEM_NAME ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
execute_process(COMMAND tail -n1 ${CMAKE_SOURCE_DIR}/meta/depends OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_DEPENDS)
SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")
SET(CPACK_DEBIAN_PACKAGE_SUGGESTS "qdbus | qdbus-qt5")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/postinst;${CMAKE_SOURCE_DIR}/debian/postrm;" )
SET(DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>= 9), cmake (>= 2.8), qtbase5-dev (>= 5.1), linux-headers, cdbs, libpugixml-dev")



#SET(CPACK_SOURCE_GENERATOR "TBZ2")


#SET(CPACK_INSTALL_CMAKE_PROJECTS "/home/arek/projects/inputMangler/build;inputmangler;ALL;/")
SET(CPACK_INSTALL_PREFIX "/usr")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "inputmangler ${CPACK_PACKAGE_VERSION}")

INCLUDE(CPack)
#SET(CPACK_BINARY_DEB "ON")
#SET(CPACK_BINARY_RPM "ON")
#SET(CPACK_BINARY_STGZ "OFF")
#SET(CPACK_BINARY_TBZ2 "ON")
#SET(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
#SET(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
#SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "inputmangler 0.1.1")
#SET(CPACK_PACKAGE_RELOCATABLE "true")
#SET(CPACK_PACKAGE_VENDOR "Humanity")
#SET(CPACK_RESOURCE_FILE_LICENSE "/usr/share/cmake-3.5/Templates/CPack.GenericLicense.txt")
#SET(CPACK_RESOURCE_FILE_README "/usr/share/cmake-3.5/Templates/CPack.GenericDescription.txt")
#SET(CPACK_RESOURCE_FILE_WELCOME "/usr/share/cmake-3.5/Templates/CPack.GenericWelcome.txt")
#SET(CPACK_SET_DESTDIR "OFF")
#SET(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/arek/projects/inputMangler/build/CPackSourceConfig.cmake")
#SET(CPACK_SYSTEM_NAME "Linux")
#SET(CPACK_TOPLEVEL_TAG "Linux")
#SET(CPACK_MODULE_PATH "")
#SET(CPACK_OUTPUT_CONFIG_FILE "/home/arek/projects/inputMangler/build/CPackConfig.cmake")
#SET(CPACK_PACKAGE_DEFAULT_LOCATION "/")
#SET(CPACK_PACKAGE_FILE_NAME "inputmangler-0.1.1-Linux")
