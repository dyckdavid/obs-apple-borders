cmake_minimum_required(VERSION 3.21)

project(obs-apple-borders VERSION 1.2.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(APPLE_BORDERS_INSTALL_LOCAL
       "Install into the per-user OBS plugins directory on macOS"
       ON)

# ---------------------------------------------------------------------------
# Locate libobs (official package config, Homebrew, or manual OBS_DIR)
# ---------------------------------------------------------------------------
include(FindLibObs)

if(NOT TARGET OBS::libobs)
  message(FATAL_ERROR
    "libobs was not found.\n"
    "Set -DOBS_DIR=/path/to/obs-studio (source or build tree) or add the\n"
    "OBS/libobs CMake package to CMAKE_PREFIX_PATH.\n"
    "See README.md for macOS setup instructions.")
endif()

add_library(${PROJECT_NAME} MODULE
  src/plugin-main.c
  src/plugin-support.c
  src/apple-chrome-util.c
  src/apple-border-filter.c
  src/apple-pip-filter.c
  src/apple-backdrop-source.c
  src/apple-vignette-filter.c
  src/apple-spotlight-filter.c
  src/apple-frost-filter.c
  src/apple-lower-third-source.c
  src/apple-soft-gradient-source.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(${PROJECT_NAME} PRIVATE OBS::libobs)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin-version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/plugin-version.h"
  @ONLY
)

# ---------------------------------------------------------------------------
# Platform packaging
# ---------------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
  PREFIX ""
  OUTPUT_NAME "obs-apple-borders"
)

if(APPLE)
  set(MACOS_BUNDLE_ID "com.obsproject.obs-apple-borders")

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Info.plist.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    @ONLY
  )

  set_target_properties(${PROJECT_NAME} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION plugin
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOS_BUNDLE_ID}"
  )

  # Ship locale + effects inside the .plugin bundle Resources/
  file(GLOB_RECURSE APPLE_BORDERS_DATA_FILES
       CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
  foreach(data_file IN LISTS APPLE_BORDERS_DATA_FILES)
    file(RELATIVE_PATH rel_path
         "${CMAKE_CURRENT_SOURCE_DIR}/data" "${data_file}")
    get_filename_component(rel_dir "${rel_path}" DIRECTORY)
    set_source_files_properties("${data_file}" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources/${rel_dir}")
    target_sources(${PROJECT_NAME} PRIVATE "${data_file}")
  endforeach()

  if(APPLE_BORDERS_INSTALL_LOCAL)
    set(_obs_user_plugin_dir
        "$ENV{HOME}/Library/Application Support/obs-studio/plugins")
    install(TARGETS ${PROJECT_NAME}
            BUNDLE DESTINATION "${_obs_user_plugin_dir}"
            LIBRARY DESTINATION "${_obs_user_plugin_dir}"
            ARCHIVE DESTINATION "${_obs_user_plugin_dir}")
  else()
    install(TARGETS ${PROJECT_NAME}
            BUNDLE DESTINATION .
            LIBRARY DESTINATION .
            ARCHIVE DESTINATION .)
  endif()

  # Convenience: copy the built .plugin next to the build tree
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/rundir"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/obs-apple-borders.plugin"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/obs-apple-borders.plugin"
    COMMENT "Copying obs-apple-borders.plugin to rundir/"
    VERBATIM
  )
else()
  # Linux / Windows layout mirrors OBS data/obs-plugins paths
  install(TARGETS ${PROJECT_NAME}
          LIBRARY DESTINATION obs-plugins
          RUNTIME DESTINATION obs-plugins)
  install(DIRECTORY data/
          DESTINATION data/obs-plugins/${PROJECT_NAME})

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/obs-plugins"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/data/obs-plugins/${PROJECT_NAME}"
    COMMAND "${CMAKE_COMMAND}" -E copy
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/obs-plugins/"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "${CMAKE_CURRENT_BINARY_DIR}/rundir/data/obs-plugins/${PROJECT_NAME}"
    COMMENT "Copying plugin + data to rundir/"
    VERBATIM
  )
endif()

message(STATUS "obs-apple-borders ${PROJECT_VERSION}")
if(LIBOBS_INCLUDE_DIR)
  message(STATUS "  libobs include: ${LIBOBS_INCLUDE_DIR}")
endif()
if(LIBOBS_LIB)
  message(STATUS "  libobs library: ${LIBOBS_LIB}")
endif()
