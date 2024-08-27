cmake_minimum_required(VERSION 3.19)

find_package(Git)

project(the-arbor-gui
  VERSION 0.9.1
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

# Generate a .json file with full compilation command for each file.
set(CMAKE_EXPORT_COMPILE_COMMANDS "YES")

# Do we want to generate a OSX thing?
set(ARBORGUI_BUILD_BUNDLE OFF CACHE BOOL "Make an OSX package")

# Tweak Arbor
set(ARB_WITH_NEUROML     ON  CACHE BOOL "enable neuroml"    FORCE)
set(ARB_WITH_PYTHON      OFF CACHE BOOL "disable pyarb"     FORCE)
set(ARB_USE_BUNDLED_LIBS ON  CACHE BOOL "use internal deps" FORCE)
add_subdirectory(3rd-party/arbor)

# Add OpenGL loader
set(OPTION_BUILD_EXAMPLES          OFF CACHE BOOL "" FORCE)
set(OPTION_BUILD_TOOLs             OFF CACHE BOOL "" FORCE)
set(OPTION_USE_GIT_INFORMATION     OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS              OFF CACHE BOOL "" FORCE)
add_subdirectory(3rd-party/glbinding)

# Tweak GLFW
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        ON  CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_NONE   ON  CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  ON  CACHE BOOL "" FORCE)
add_subdirectory(3rd-party/glfw)

# Set up sanitizer targets
list(APPEND CMAKE_CONFIGURATION_TYPES asan)
list(APPEND CMAKE_CONFIGURATION_TYPES msan)
list(APPEND CMAKE_CONFIGURATION_TYPES ubsan)

set(CMAKE_C_FLAGS_ASAN
  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C compiler for Asan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_ASAN
  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_ASAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
  "Linker flags to be used to create executables for Asan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_ASAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
  "Linker lags to be used to create shared libraries for Asan build type." FORCE)

set(CMAKE_C_FLAGS_MSAN
  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=memory -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C compiler for Msan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_MSAN
  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=memory -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C++ compiler for Msan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_MSAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=memory" CACHE STRING
  "Linker flags to be used to create executables for Msan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_MSAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=memory" CACHE STRING
  "Linker lags to be used to create shared libraries for Msan build type." FORCE)

set(CMAKE_C_FLAGS_UBSAN
  "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C compiler for Ubsan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_UBSAN
  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined -fno-omit-frame-pointer" CACHE STRING
  "Flags used by the C++ compiler for Ubsan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_UBSAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=undefined" CACHE STRING
  "Linker flags to be used to create executables for Ubsan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_UBSAN
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=undefined" CACHE STRING
  "Linker lags to be used to create shared libraries for Ubsan build type." FORCE)

# figure out OpenGL on *NIX
if (APPLE)
  find_library(cocoa_library Cocoa)
  find_library(iokit_library IOKit)
  find_library(cf_library CoreFoundation)
  list(APPEND ARBORGUI_LIBS ${cf_library} ${cocoa_library} ${iokit_library})
  mark_as_advanced(cocoa_library iokit_library)
  find_library(corevideo_library CoreVideo)
  # find_library(opengl_library OpenGL)
  # Disable OpenGL deprecation warnings
  add_definitions(-DGL_SILENCE_DEPRECATION=1)
  list(APPEND ARBORGUI_LIBS ${corevideo_library})
  # mark_as_advanced( corevideo_library)
elseif (CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
  list(APPEND ARBORGUI_LIBS X11 pthread)
  list(APPEND ARBORGUI_LIBS GL)
  if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND ARBORGUI_LIBS rt)
  endif()
  if (CMAKE_SYSTEM MATCHES "Linux")
    list(APPEND ARBORGUI_LIBS dl)
  endif()
endif()

add_library(arbor-gui-deps INTERFACE)
target_link_libraries(arbor-gui-deps INTERFACE glbinding)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/fmt/include)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/icons)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/spdlog/include)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/json/include)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/imgui)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/implot)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/ImGuizmo)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/glm)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/stb)
target_include_directories(arbor-gui-deps SYSTEM INTERFACE 3rd-party/imgui/backends)
add_library(imgui OBJECT
  3rd-party/imgui/backends/imgui_impl_opengl3.cpp
  3rd-party/imgui/backends/imgui_impl_glfw.cpp
  3rd-party/imgui/imgui.cpp
  3rd-party/imgui/imgui_draw.cpp
  3rd-party/imgui/imgui_widgets.cpp
  3rd-party/imgui/imgui_tables.cpp
  3rd-party/imgui/imgui_demo.cpp
  3rd-party/imgui/misc/cpp/imgui_stdlib.cpp
  3rd-party/ImGuizmo/ImGuizmo.cpp
  3rd-party/implot/implot.cpp
  3rd-party/implot/implot_items.cpp)
target_include_directories(imgui PRIVATE 3rd-party/imgui)
target_include_directories(imgui PUBLIC 3rd-party/implot)
target_include_directories(imgui PUBLIC 3rd-party/ImGuizmo/)
target_link_libraries(imgui PRIVATE glfw)
target_compile_definitions(arbor-gui-deps INTERFACE SPDLOG_FMT_EXTERNAL)

# Get commit hashes from git and inject into config
execute_process(COMMAND ${GIT_EXECUTABLE} -C ${PROJECT_SOURCE_DIR} describe --always --dirty --abbrev=0
                COMMAND_ECHO STDOUT
                OUTPUT_VARIABLE ARBORGUI_GIT_COMMIT
                OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file(src/config.hpp.in config.hpp @ONLY)

set(gui_srcs
  src/main.cpp
  src/cell_builder.hpp src/cell_builder.cpp
  src/gui_state.hpp src/gui_state.cpp
  src/window.hpp src/window.cpp
  src/id.hpp
  src/recipe.hpp
  src/utils.hpp src/utils.cpp
  src/view_state.hpp
  src/events.hpp
  src/spike_detector.hpp src/spike_detector.cpp
  src/stimulus.hpp src/stimulus.cpp
  src/probe.hpp src/probe.cpp
  src/ion.hpp src/ion.cpp
  src/simulation.hpp src/simulation.cpp
  src/parameter.hpp src/parameter.cpp
  src/cv_policy.hpp src/cv_policy.cpp
  src/mechanism.hpp src/mechanism.cpp
  src/component.hpp
  src/file_chooser.hpp src/file_chooser.cpp
  src/loader.hpp src/loader.cpp
  src/geometry.hpp src/geometry.cpp
  src/location.hpp)

set_source_files_properties("${gui_srcs}" PROPERTIES COMPILE_FLAGS "-Wall -Wextra -pedantic")

if(ARBORGUI_BUILD_BUNDLE)
  if (APPLE)
    set(resource_path "arbor-gui.app/Contents/Resources")

    set(ARBORGUI_ICON arbor.icns)
    set(ARBORGUI_ICON_PATH ${CMAKE_SOURCE_DIR}/images/${ARBORGUI_ICON})
    set_source_files_properties(${ARBORGUI_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    add_executable(arbor-gui MACOSX_BUNDLE ${ARBORGUI_ICON_PATH} ${gui_srcs} $<TARGET_OBJECTS:imgui>)
    set_target_properties(arbor-gui PROPERTIES MACOSX_BUNDLE_ICONFILE ${ARBORGUI_ICON})
    target_compile_definitions(arbor-gui PUBLIC FMT_HEADER_ONLY)
    target_include_directories(arbor-gui PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(arbor-gui PRIVATE ${ARBORGUI_LIBS} arbor arborio glbinding glfw arbor-gui-deps)

    install(TARGETS arbor-gui
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime)

    install(FILES     ${ARBORGUI_ICON_PATH} DESTINATION ${resource_path})
    install(FILES     default.ini           DESTINATION ${resource_path} RENAME imgui.ini)
    install(DIRECTORY glsl                  DESTINATION ${resource_path})
    install(DIRECTORY fonts                 DESTINATION ${resource_path})

    set(CPACK_BINARY_DRAGNDROP ON)
    set(CPACK_GENERATOR "DragNDrop")
    include(CPack)
  endif()
  if (CMAKE_SYSTEM MATCHES "Linux")
  endif()
else()  
  set(resource_path "${CMAKE_INSTALL_FULL_DATAROOTDIR}/arbor-gui")
  add_compile_definitions(ARBORGUI_RESOURCES_BASE="${resource_path}")

  add_executable(arbor-gui ${gui_srcs} $<TARGET_OBJECTS:imgui>)
  target_include_directories(arbor-gui PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(arbor-gui PRIVATE ${ARBORGUI_LIBS} arbor arborio glbinding glfw arbor-gui-deps)
  target_compile_definitions(arbor-gui PUBLIC FMT_HEADER_ONLY)

  # Set icon on output
  if (APPLE)
    add_custom_command(TARGET arbor-gui POST_BUILD
      COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_CURRENT_BINARY_DIR}/arbor-gui
      DEPENDS ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png)
  endif()
  if (CMAKE_SYSTEM MATCHES "Linux")
    install(FILES     ${CMAKE_SOURCE_DIR}/images/arbor.png DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/256x256/apps/)
    install(FILES     ${CMAKE_SOURCE_DIR}/images/arbor.svg DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps/)
    install(FILES     ${CMAKE_SOURCE_DIR}/scripts/arbor-gui.desktop DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications/)
  endif()

  install(TARGETS arbor-gui RUNTIME DESTINATION bin)
  # Set icon _again_ since CMake strips installed binaries
  if (APPLE)
    install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_INSTALL_PREFIX}/bin/arbor-gui)")
  endif()

  # install resources
  install(DIRECTORY glsl    DESTINATION ${resource_path})
  install(DIRECTORY fonts   DESTINATION ${resource_path})
  install(FILES default.ini DESTINATION ${resource_path} RENAME imgui.ini)
endif()
