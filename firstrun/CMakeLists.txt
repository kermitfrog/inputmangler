#project(inputmangler)
#cmake_minimum_required(VERSION 2.8.11)

find_package(Qt5 COMPONENTS Core DBus Widgets REQUIRED)


# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(FIRSTRUN_UIS
        firstrun.ui
        )
set(FIRSTRUN_SRCS
        FirstRunWizard.cpp
        UdevRules.cpp
        main.cpp
        ../shared/definitions.h
        ../shared/keydefs.cpp
        ../shared/inputevent.cpp
        )

qt5_wrap_ui(${FIRSTRUN_UIS})
find_library(PUGIXML_LIB pugixml)
#add_library(pugixml SHARED IMPORTED)
add_executable(inputmangler_firstrun_wizard ${FIRSTRUN_SRCS} ${FIRSTRUN_UIS})
target_link_libraries(inputmangler_firstrun_wizard pugixml Qt5::Core Qt5::Widgets Qt5::DBus)

install(TARGETS inputmangler_firstrun_wizard RUNTIME DESTINATION bin)
#FILE(GLOB keymaps "${CMAKE_CURRENT_SOURCE_DIR}/../keymaps/*")
#INSTALL(FILES ${keymaps} DESTINATION share/inputmangler/keymaps/)
#FILE(GLOB doc "${CMAKE_CURRENT_SOURCE_DIR}/../doc/*")
#INSTALL(FILES ${doc} DESTINATION share/doc/inputmangler/)
#FILE(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/*")
#INSTALL(PROGRAMS ${scripts} DESTINATION share/inputmangler/)

#add_definitions( -DDEBUGME )
