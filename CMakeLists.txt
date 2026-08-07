cmake_minimum_required(VERSION 3.10)

# Project setup
project(WojakCoin VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Always use system libraries
set(USE_DEPENDS OFF)
message(STATUS "Using system libraries")

# Find required packages from system
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Network 
    DBus 
    PrintSupport 
    REQUIRED
)

find_package(OpenSSL REQUIRED)
find_package(BerkeleyDB REQUIRED)
find_package(Boost COMPONENTS system filesystem thread chrono REQUIRED)
find_package(Protobuf REQUIRED)
find_package(QRencode)
find_package(Threads REQUIRED)

# Set variables for compatibility
set(BDB_INCLUDE_DIR ${BerkeleyDB_INCLUDE_DIRS})
set(BDB_LIBRARY ${BerkeleyDB_LIBRARIES})

# Add definitions for system build
add_definitions(-DHAVE_CONFIG_H)

if(UNIX AND NOT APPLE)
    add_definitions(-DHAVE_DECL_STRERROR_R=1)
endif()

if(APPLE)
    add_definitions(-DMAC_OSX)
endif()

# Add include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${BDB_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers
)

# Add subdirectories
add_subdirectory(src/leveldb)
add_subdirectory(src)
add_subdirectory(src/qt)

# Set output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Install rules
install(TARGETS wojakcoin-qt
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Package configuration
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
include(CPack)

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
if(USE_DEPENDS)
    message(STATUS "Using depends prefix: ${DEPENDS_PREFIX}")
endif()
