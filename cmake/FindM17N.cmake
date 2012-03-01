# - Try to find the M17N libraries
# Once done this will define
#
#  M17N_FOUND - system has M17N
#  M17N_INCLUDE_DIR - the M17N include directory
#  M17N_LIBRARIES - M17N library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(M17N_INCLUDE_DIR AND M17N_LIBRARIES)
    # Already in cache, be silent
    set(M17N_FIND_QUIETLY TRUE)
endif(M17N_INCLUDE_DIR AND M17N_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_LIBM17N QUIET m17n-shell)

find_path(M17N_INCLUDE_DIR
          NAMES m17n.h
          HINTS ${PC_LIBM17N_INCLUDEDIR})

find_library(M17N_CORE_LIBRARIES
             NAMES m17n-core
             HINTS ${PC_LIBM17N_LIBDIR})

find_library(M17N_FLT_LIBRARIES
             NAMES m17n-flt
             HINTS ${PC_LIBM17N_LIBDIR})

find_library(M17N_M17N_LIBRARIES
             NAMES m17n
             HINTS ${PC_LIBM17N_LIBDIR})

set(M17N_LIBRARIES ${M17N_CORE_LIBRARIES} ${M17N_FLT_LIBRARIES} ${M17N_M17N_LIBRARIES})
set(M17N_VERSION ${PC_LIBM17N_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(M17N  DEFAULT_MSG M17N_VERSION M17N_LIBRARIES M17N_INCLUDE_DIR)

mark_as_advanced(M17N_INCLUDE_DIR M17N_LIBRARIES M17N_CORE_LIBRARIES M17N_FLT_LIBRARIES  M17N_M17N_LIBRARIES)
