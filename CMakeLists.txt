cmake_minimum_required(VERSION 3.12...3.18)

project(the-arbor-gui
  VERSION 0.0.1
  LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

find_package(arbor REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_definitions()

set(resource_path "${CMAKE_INSTALL_PREFIX}/etc/arbor-gui")
add_compile_definitions(ARBORGUI_RESOURCES_BASE="${resource_path}")

# figure out OpenGL on *NIX
if (APPLE)
  find_library(cocoa_library Cocoa)
  find_library(iokit_library IOKit)
  list(APPEND ARBORGUI_LIBS ${cocoa_library} ${iokit_library})
  mark_as_advanced(cocoa_library iokit_library)
  find_library(corevideo_library CoreVideo)
  find_library(opengl_library OpenGL)
    list(APPEND NANOGUI_LIBS GL)
  # Disable OpenGL deprecation warnings
  add_definitions(-DGL_SILENCE_DEPRECATION)
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

add_executable(arbor-gui main.cpp
  src/definition.hpp
  src/cell_builder.hpp src/cell_builder.cpp
  src/gui_state.hpp src/gui_state.cpp
  src/window.hpp src/window.cpp
  src/utils.hpp src/utils.cpp
  src/geometry.hpp src/geometry.cpp
  src/location.hpp src/location.cpp
  3rd-party/imgui/examples/libs/gl3w/GL/gl3w.c
  3rd-party/imgui/examples/imgui_impl_opengl3.cpp
  3rd-party/imgui/examples/imgui_impl_glfw.cpp
  3rd-party/imgui/imgui.cpp 3rd-party/imgui/imgui_draw.cpp
  3rd-party/imgui/imgui_widgets.cpp)
target_include_directories(arbor-gui PRIVATE 3rd-party/fmt/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/icons)
target_include_directories(arbor-gui PRIVATE 3rd-party/spdlog/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/json/include)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui/examples)
target_include_directories(arbor-gui PRIVATE 3rd-party/imgui/examples/libs/gl3w)
target_include_directories(arbor-gui PRIVATE 3rd-party/filebrowser)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GL3W)
target_include_directories(arbor-gui PRIVATE src)
target_link_libraries(arbor-gui PRIVATE ${ARBORGUI_LIBS})
target_link_libraries(arbor-gui PRIVATE arbor::arbor)
target_link_libraries(arbor-gui PRIVATE glfw)
target_compile_options(arbor-gui PRIVATE -Wall -Wextra -pedantic)

install(TARGETS arbor-gui
  RUNTIME DESTINATION bin)
install(DIRECTORY glsl
  DESTINATION ${resource_path})
install(DIRECTORY fonts
  DESTINATION ${resource_path})
