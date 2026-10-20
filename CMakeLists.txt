cmake_minimum_required(VERSION 3.16)
project(leap-dicom-viewer VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LEAP_DICOM_WITH_LEAP "Drive the view with a Leap Motion controller (needs Leap SDK v2)" OFF)
set(LEAP_SDK_DIR "" CACHE PATH "Leap SDK v2 root, containing include/Leap.h and lib/")
option(LEAP_DICOM_BUILD_TESTS "Build unit tests" ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
# Debian/Ubuntu's VTK 9.1 package links VTK::mpi to MPI::MPI_C without finding MPI itself.
find_package(MPI QUIET COMPONENTS C)
find_package(VTK 9 REQUIRED COMPONENTS
  CommonCore
  FiltersSources
  IOImage
  InteractionStyle
  RenderingAnnotation
  RenderingCore
  RenderingFreeType
  RenderingOpenGL2
  RenderingVolume
  RenderingVolumeOpenGL2
)

# Hand-tracking logic: plain C++, no VTK or Leap.
add_library(leapviewer_interaction STATIC src/interaction.cpp)
target_include_directories(leapviewer_interaction PUBLIC include)
target_link_libraries(leapviewer_interaction PUBLIC Threads::Threads)

# Volume pipeline.
add_library(leapviewer_scene STATIC src/scene.cpp)
target_include_directories(leapviewer_scene PUBLIC include)
target_link_libraries(leapviewer_scene PUBLIC ${VTK_LIBRARIES})

add_executable(leap-dicom-viewer src/main.cpp)
target_link_libraries(leap-dicom-viewer PRIVATE leapviewer_interaction leapviewer_scene)

if(LEAP_DICOM_WITH_LEAP)
  find_path(LEAP_INCLUDE_DIR Leap.h HINTS "${LEAP_SDK_DIR}/include" "${LEAP_SDK_DIR}" REQUIRED)
  find_library(LEAP_LIBRARY NAMES Leap HINTS "${LEAP_SDK_DIR}/lib" "${LEAP_SDK_DIR}/lib/x64")
  target_sources(leap-dicom-viewer PRIVATE src/leap_input.cpp)
  target_include_directories(leap-dicom-viewer PRIVATE "${LEAP_INCLUDE_DIR}")
  target_compile_definitions(leap-dicom-viewer PRIVATE LEAP_DICOM_WITH_LEAP)
  if(LEAP_LIBRARY)
    target_link_libraries(leap-dicom-viewer PRIVATE "${LEAP_LIBRARY}")
  endif()
endif()

vtk_module_autoinit(TARGETS leap-dicom-viewer leapviewer_scene MODULES ${VTK_LIBRARIES})

if(LEAP_DICOM_BUILD_TESTS)
  enable_testing()
  add_executable(test_interaction tests/test_interaction.cpp)
  target_link_libraries(test_interaction PRIVATE leapviewer_interaction)
  add_test(NAME interaction COMMAND test_interaction)

  add_executable(test_scene tests/test_scene.cpp)
  target_link_libraries(test_scene PRIVATE leapviewer_scene)
  vtk_module_autoinit(TARGETS test_scene MODULES ${VTK_LIBRARIES})
  add_test(NAME scene COMMAND test_scene)
endif()
