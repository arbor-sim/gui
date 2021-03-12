cmake_minimum_required(VERSION 3.12)

project(the-arbor-gui
  VERSION 0.0.1
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

set(resource_path "${CMAKE_INSTALL_PREFIX}/etc/arbor-gui")
add_compile_definitions(ARBORGUI_RESOURCES_BASE="${resource_path}")
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GL3W="ON")

# Tweak Arbor
set(ARB_WITH_NEUROML     ON CACHE BOOL "enable neuroml"    FORCE)
set(ARB_USE_BUNDLED_LIBS ON CACHE BOOL "use internal deps" FORCE)

# Tweak GLFW
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# figure out OpenGL on *NIX
if (APPLE)
  find_library(cocoa_library Cocoa)
  find_library(iokit_library IOKit)
  list(APPEND ARBORGUI_LIBS ${cocoa_library} ${iokit_library})
  mark_as_advanced(cocoa_library iokit_library)
  find_library(corevideo_library CoreVideo)
  find_library(opengl_library OpenGL)
  # Disable OpenGL deprecation warnings
  add_definitions(-DGL_SILENCE_DEPRECATION=1)
  list(APPEND ARBORGUI_LIBS ${opengl_library} ${corevideo_library})
  mark_as_advanced(opengl_library corevideo_library)
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

set(gui_srcs
  main.cpp
  src/definition.hpp
  src/cell_builder.hpp src/cell_builder.cpp
  src/gui_state.hpp src/gui_state.cpp
  src/window.hpp src/window.cpp
  src/id.hpp
  src/utils.hpp src/utils.cpp
  src/view_state.hpp
  src/events.hpp
  src/component.hpp
  src/file_chooser.hpp
  src/loader.hpp src/loader.cpp
  src/geometry.hpp src/geometry.cpp
  src/location.hpp)

set(gui_srcs ${gui_srcs})

set(im_srcs
  3rd-party/imgui/examples/libs/gl3w/GL/gl3w.c
  3rd-party/imgui/examples/imgui_impl_opengl3.cpp
  3rd-party/imgui/examples/imgui_impl_glfw.cpp
  3rd-party/imgui/imgui.cpp 3rd-party/imgui/imgui_draw.cpp
  3rd-party/imgui/imgui_widgets.cpp
  3rd-party/imgui/imgui_demo.cpp
  3rd-party/imgui/misc/cpp/imgui_stdlib.cpp)

set_source_files_properties("${gui_srcs}" PROPERTIES COMPILE_FLAGS "-Wall -Wextra -pedantic -flto")

add_subdirectory(3rd-party/arbor)
add_subdirectory(3rd-party/glfw)

add_executable(arbor-gui ${gui_srcs} ${im_srcs})

if(NOT CMAKE_BUILD_TYPE STREQUAL "release")
  message("Adding profiling.")
  add_library(TracyClient STATIC 3rd-party/tracy/TracyClient.cpp)
  target_link_libraries(arbor-gui PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  target_link_libraries(arbor-gui PUBLIC TracyClient)
  add_compile_definitions(TRACY_ENABLE PUBLIC TRACY_ENABLE=1)
  add_compile_definitions(TRACY_ONLY_LOCALHOST PUBLIC TRACY_ONLY_LOCALHOST=1)
endif()

target_link_options(arbor-gui PUBLIC "-flto")
target_include_directories(arbor-gui PRIVATE 3rd-party/fmt/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/icons)
target_include_directories(arbor-gui PRIVATE 3rd-party/spdlog/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/json/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui)
target_include_directories(arbor-gui PRIVATE 3rd-party/glm)
target_include_directories(arbor-gui PRIVATE 3rd-party/stb)
target_include_directories(arbor-gui PRIVATE 3rd-party/tracy)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui/examples)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui/examples/libs/gl3w)
target_include_directories(arbor-gui PRIVATE src)
target_link_libraries(arbor-gui PRIVATE ${ARBORGUI_LIBS})
target_link_libraries(arbor-gui PRIVATE arbor arborio arbornml)
target_link_libraries(arbor-gui PRIVATE glfw)

# Set icon on output
if (APPLE)
  add_custom_command(TARGET arbor-gui POST_BUILD
    COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_CURRENT_BINARY_DIR}/arbor-gui
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png)
endif()
if (CMAKE_SYSTEM MATCHES "Linux")
  add_custom_command(TARGET arbor-gui POST_BUILD
    COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/set-linux-desktop-file.sh ${resource_path} ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_SOURCE_DIR}/images/arbor.svg ${CMAKE_SOURCE_DIR}/scripts/arbor-gui.desktop
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/set-linux-desktop-file.sh ${CMAKE_SOURCE_DIR}/images/arbor.svg ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_SOURCE_DIR}/scripts/arbor-gui.desktop)
endif()

install(TARGETS arbor-gui RUNTIME DESTINATION bin)
# Set icon _again_ since CMake strips installed binaries
if (APPLE)
  install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/set-osx-icon.sh ${CMAKE_SOURCE_DIR}/images/arbor.png ${CMAKE_INSTALL_PREFIX}/bin/arbor-gui)")
endif()
install(DIRECTORY glsl    DESTINATION ${resource_path})
install(DIRECTORY fonts   DESTINATION ${resource_path})
install(FILES default.ini DESTINATION ${resource_path} RENAME imgui.ini)
