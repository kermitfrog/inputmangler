project(inputmangler)
cmake_minimum_required(VERSION 2.8.8)
find_package(Qt5Core)
find_package(Qt5Xml)
#find_package(Qt5Widgets)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(INPUTMANGLER_SRCS main.cpp inputmangler.cpp abstractinputhandler.cpp devhandler.cpp qtsignalhandler.cpp)

add_executable(inputmangler ${INPUTMANGLER_SRCS})

install(TARGETS inputmangler RUNTIME DESTINATION bin)

qt5_use_modules(inputmangler Core Xml)
