# FindLibObs.cmake — locate OBS Studio's libobs for plugin builds.
#
# Search order:
#   1. find_package(libobs CONFIG) via CMAKE_PREFIX_PATH / OBS_DIR
#   2. pkg-config (Linux)
#   3. Manual OBS_DIR / LIBOBS_INCLUDE_DIR / LIBOBS_LIB hints
#
# On success, creates imported target OBS::libobs and sets:
#   LIBOBS_FOUND, LIBOBS_INCLUDE_DIR, LIBOBS_LIB

if(TARGET OBS::libobs)
  set(LIBOBS_FOUND TRUE)
  return()
endif()

# Allow -DOBS_DIR=/path/to/obs-studio[/build] or obs-deps prefix
if(DEFINED OBS_DIR AND NOT OBS_DIR STREQUAL "")
  list(PREPEND CMAKE_PREFIX_PATH "${OBS_DIR}")
  if(EXISTS "${OBS_DIR}/lib/cmake/libobs")
    list(PREPEND CMAKE_PREFIX_PATH "${OBS_DIR}")
  endif()
  if(EXISTS "${OBS_DIR}/build")
    list(PREPEND CMAKE_PREFIX_PATH "${OBS_DIR}/build")
  endif()
endif()

# Official OBS builds / plugin template export a config package.
find_package(libobs CONFIG QUIET)

if(libobs_FOUND AND TARGET OBS::libobs)
  get_target_property(_obs_inc OBS::libobs INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(_obs_loc OBS::libobs IMPORTED_LOCATION)
  if(NOT _obs_loc)
    get_target_property(_obs_loc OBS::libobs IMPORTED_LOCATION_RELEASE)
  endif()
  set(LIBOBS_INCLUDE_DIR "${_obs_inc}")
  set(LIBOBS_LIB "${_obs_loc}")
  set(LIBOBS_FOUND TRUE)
  message(STATUS "Found libobs via CMake package config")
  return()
endif()

# pkg-config fallback (common on Linux)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_LIBOBS QUIET libobs)
endif()

find_path(LIBOBS_INCLUDE_DIR
  NAMES obs-module.h
  HINTS
    ${PC_LIBOBS_INCLUDE_DIRS}
    ${LIBOBS_INCLUDE_DIR}
    ENV LIBOBS_INCLUDE_DIR
    ${OBS_DIR}/libobs
    ${OBS_DIR}/include
    ${OBS_DIR}/include/obs
    /opt/homebrew/include/obs
    /usr/local/include/obs
    /usr/include/obs
  PATH_SUFFIXES obs libobs
)

find_library(LIBOBS_LIB
  NAMES obs libobs
  HINTS
    ${PC_LIBOBS_LIBRARY_DIRS}
    ${LIBOBS_LIB}
    ENV LIBOBS_LIB
    ${OBS_DIR}
    ${OBS_DIR}/build
    ${OBS_DIR}/lib
    /opt/homebrew/lib
    /usr/local/lib
    /usr/lib
  PATH_SUFFIXES
    lib
    lib64
    RoundTable
    UI
    rundir/RelWithDebInfo/bin/64bit
    rundir/Release/bin/64bit
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibObs
  FOUND_VAR LIBOBS_FOUND
  REQUIRED_VARS LIBOBS_LIB LIBOBS_INCLUDE_DIR
)

if(LIBOBS_FOUND AND NOT TARGET OBS::libobs)
  add_library(OBS::libobs UNKNOWN IMPORTED)
  set_target_properties(OBS::libobs PROPERTIES
    IMPORTED_LOCATION "${LIBOBS_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBOBS_INCLUDE_DIR}"
  )

  # Headers may live in .../include/obs or directly as obs-module.h next to libobs/
  get_filename_component(_obs_inc_parent "${LIBOBS_INCLUDE_DIR}" DIRECTORY)
  if(EXISTS "${LIBOBS_INCLUDE_DIR}/obs-module.h")
    # include dir already correct (contains obs-module.h)
  elseif(EXISTS "${_obs_inc_parent}/obs-module.h")
    set_target_properties(OBS::libobs PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${_obs_inc_parent}")
  endif()

  # On macOS, OBS.app embeds frameworks — allow linking against framework style
  if(APPLE AND EXISTS "/Applications/OBS.app/Contents/Frameworks")
    target_link_options(OBS::libobs INTERFACE
      "LINKER:-F/Applications/OBS.app/Contents/Frameworks")
  endif()
endif()

mark_as_advanced(LIBOBS_INCLUDE_DIR LIBOBS_LIB)
