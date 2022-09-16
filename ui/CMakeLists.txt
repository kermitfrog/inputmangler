#project(inputmangler)
#cmake_minimum_required(VERSION 2.8.11)

find_package(Qt5Core)
find_package(Qt5DBus)
find_package(Qt5Widgets)


# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(IMGUI_UIS
        inputmanglerui.ui
        )
set(IMGUI_SRCS
        inputmanglerui.cpp
        main.cpp
        imconfmodel.cpp
        ../shared/keydefs.cpp
        ../shared/inputevent.cpp
        inoutbox.cpp inoutbox.h)

#set(IMGUI_HEADERS
#        inputmanglerui.h
#        imconfmodel.h
#        ../shared/keydefs.h
#        ../shared/inputevent.h
#        )

qt5_wrap_ui(${IMGUI_UIS})
find_library(PUGIXML_LIB pugixml)
#add_library(pugixml SHARED IMPORTED)
add_executable(inputmangler_ui ${IMGUI_SRCS} ${IMGUI_UIS} ${IMGUI_HEADERS})
target_link_libraries(inputmangler_ui pugixml)

install(TARGETS inputmangler_ui RUNTIME DESTINATION bin)
#FILE(GLOB keymaps "${CMAKE_CURRENT_SOURCE_DIR}/../keymaps/*")
#INSTALL(FILES ${keymaps} DESTINATION share/inputmangler/keymaps/)
#FILE(GLOB doc "${CMAKE_CURRENT_SOURCE_DIR}/../doc/*")
#INSTALL(FILES ${doc} DESTINATION share/doc/inputmangler/)
#FILE(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/*")
#INSTALL(PROGRAMS ${scripts} DESTINATION share/inputmangler/)

qt5_use_modules(inputmangler_ui  Core DBus Widgets)

#add_definitions( -DDEBUGME )
