# ----------------------------------------------------------------------------
#  Root CMake file for MomoCV
#
#    From the off-tree build directory, invoke:
#      $ cmake <PATH_TO_MOMOCV_ROOT>
#
#  author: conansherry
# ----------------------------------------------------------------------------

include(cmake/MomoCVMinDepVersions.cmake)

cmake_minimum_required(VERSION "${MIN_VER_CMAKE}" FATAL_ERROR)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
else(NOT CMAKE_TOOLCHAIN_FILE)
  #Android: set output folder to ${CMAKE_BINARY_DIR}
  set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")
  # any crosscompiling
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif(NOT CMAKE_TOOLCHAIN_FILE)

include(cmake/MomoCVUtils.cmake)

set(THIRD_PARTY_DEPENDENCIES "")

project(opengl_framework CXX C)

mmcv_update(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configs" FORCE)
if(DEFINED CMAKE_BUILD_TYPE)
  set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )
endif()

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX _d)
endif()

if(MSVC)
  set(CMAKE_USE_RELATIVE_PATHS ON CACHE INTERNAL "" FORCE)
endif()

# ----------------------------------------------------------------------------
# Break in case of popular CMake configuration mistakes
# ----------------------------------------------------------------------------
if(NOT CMAKE_SIZEOF_VOID_P GREATER 0)
  message(FATAL_ERROR "CMake fails to deterimine the bitness of target platform.
  Please check your CMake and compiler installation. If you are crosscompiling then ensure that your CMake toolchain file correctly sets the compiler details.")
endif()

MMCV_OPTION(BUILD_SHARED_LIBRARIES       "Build shared library"               OFF)
MMCV_OPTION(BUILD_TESTS                  "Build tests application"            OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    unset(CMAKE_INSTALL_PREFIX CACHE)
endif()
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")

if(NLOG)
  add_definitions(-DNLOG)
endif()

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
elseif(APPLE)
  add_definitions(-fembed-bitcode)
endif()
add_definitions(-DGLEW_NO_GLU)

if(WIN32)
elseif(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC ${CMAKE_CXX_FLAGS}")
endif()

# static lib or dynamic lib
if(BUILD_SHARED_LIBRARIES)
  set(MMCV_LIBTYPE SHARED)
else()
  set(MMCV_LIBTYPE STATIC)
endif()

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})
if(WIN32)
  list(APPEND THIRD_PARTY_DEPENDENCIES ${GLEW_LIBRARY_RELEASE})
else()
  list(APPEND THIRD_PARTY_DEPENDENCIES ${GLEW_LIBRARY})
endif()

find_library(OPENGL_gl_LIBRARY GL PATHS
  /usr/lib64
)
list(APPEND THIRD_PARTY_DEPENDENCIES ${OPENGL_gl_LIBRARY})

if(UNIX AND NOT APPLE)
  include(cmake/Modules/FindEGL.cmake)
  list(APPEND THIRD_PARTY_DEPENDENCIES ${EGL_LIBRARY})
endif()

include_directories("./include/")

file(GLOB_RECURSE MMCV_ALL_HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE MMCV_ALL_SRCS "src/*.cpp" "src/*.cc" "src/*.c")

# group files
mmcv_make_group("${MMCV_ALL_HEADERS}")
mmcv_make_group("${MMCV_ALL_SRCS}")

# build library
set(libname "opengl_framework")
set(MMCV_ALL_LIBS ${libname})
mmcv_buildlibrary(${libname} ${MMCV_LIBTYPE} SOURCES ${MMCV_ALL_HEADERS} ${MMCV_ALL_SRCS} DEPENDENCIES ${THIRD_PARTY_DEPENDENCIES})

install(FILES ${MMCV_ALL_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/)

list(APPEND THIRD_PARTY_DEPENDENCIES "opengl_framework")

# build tests
if(BUILD_TESTS)
  mmcv_subdirlist(test_subdirs "tests")
  foreach(subdir ${test_subdirs})
    if(EXISTS "${subdir}/CMakeLists.txt")
	  add_subdirectory(${subdir})
    endif()
  endforeach()
endif()
