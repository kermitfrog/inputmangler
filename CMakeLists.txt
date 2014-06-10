project(inputmangler)
cmake_minimum_required(VERSION 2.8.11)

#set(CMAKE_PREFIX_PATH /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/*/*)
#set(Qt5DBus_DIR /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/Qt5DBus)
#set(Qt5Core_DIR /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/Qt5Core)
#set(Qt5Xml_DIR /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/Qt5Xml)
#set(Qt5Network_DIR /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/Qt5Network)
#set(Qt5Test_DIR /home/arek/progs/Qt5.1.1/5.1.1/gcc_64/lib/cmake/Qt5Test)

find_package(Qt5Core)
find_package(Qt5Xml)
find_package(Qt5DBus)
find_package(Qt5Network)
find_package(Qt5Test)
find_package(X11)
#find_package(Qt5Widgets)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-lX11")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-lX11")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(INPUTMANGLER_SRCS 
						debughandler.cpp
						xwatcher.cpp 
						main.cpp
						inputmangler.cpp 
						abstractinputhandler.cpp
						devhandler.cpp 
						qtsignalhandler.cpp 
						imdbusinterface.cpp 
						keydefs.cpp 
						nethandler.cpp)

add_executable(inputmangler ${INPUTMANGLER_SRCS})

install(TARGETS inputmangler RUNTIME DESTINATION bin)

qt5_use_modules(inputmangler Core Xml DBus Network Test)

#add_definitions( -DDEBUGME )
