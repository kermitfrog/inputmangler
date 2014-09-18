project(inputmangler)
cmake_minimum_required(VERSION 2.8.11)


find_package(Qt5Core)
find_package(Qt5Xml)
find_package(Qt5DBus)
find_package(Qt5Network)
find_package(Qt5Test)
#find_package(X11)
#find_package(Qt5Widgets)
#SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-lX11")
#SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-lX11")

ADD_DEFINITIONS(
    -std=c++11
    )

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(INPUTMANGLER_SRCS 
						abstractinputhandler.cpp
						debughandler.cpp
						devhandler.cpp 
						imdbusinterface.cpp 
						inputevent.cpp
						inputmangler.cpp 
						keydefs.cpp 
						main.cpp
						nethandler.cpp
						output.cpp
						qtsignalhandler.cpp 
						transformationstructure.cpp
						xwatcher.cpp 
						)

add_executable(inputmangler ${INPUTMANGLER_SRCS})

install(TARGETS inputmangler RUNTIME DESTINATION bin)
FILE(GLOB keymaps "${CMAKE_CURRENT_SOURCE_DIR}/keymaps/*")
INSTALL(FILES ${keymaps} DESTINATION share/inputmangler/keymaps/)
FILE(GLOB doc "${CMAKE_CURRENT_SOURCE_DIR}/doc/*")
INSTALL(FILES ${doc} DESTINATION share/doc/inputmangler/)
FILE(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*")
INSTALL(FILES ${scripts} DESTINATION share/inputmangler/)

qt5_use_modules(inputmangler Core Xml DBus Network Test)

#add_definitions( -DDEBUGME )
