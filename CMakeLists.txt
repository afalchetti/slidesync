cmake_minimum_required (VERSION 2.6)
project (SlideSync)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(add_c_flag flag)
  string(FIND "${CMAKE_C_FLAGS}" flag alreadythere)
  if (alreadythere EQUAL -1)
    check_c_compiler_flag("${flag}" supported)
    if (supported)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

function(add_cxx_flag flag)
  string(FIND "${CMAKE_CXX_FLAGS}" flag alreadythere)
  if (alreadythere EQUAL -1)
    check_cxx_compiler_flag("${flag}" supported)
    if (supported)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_c_flag("-Wall")

add_cxx_flag("-Wall")

include_directories("${CMAKE_SOURCE_DIR}/dep/include")
link_directories("${CMAKE_SOURCE_DIR}/dep/lib")

find_package(OpenCV 3 REQUIRED)
find_package(OpenGL 3 REQUIRED)
find_package(ImageMagick 7 REQUIRED COMPONENTS Magick++ convert)
find_package(wxWidgets 3 REQUIRED)
include(${wxWidgets_USE_FILE})

# ImageMagick flags
add_cxx_flag("-fopenmp")

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${ImageMagick_INCLUDE_DIRS})

add_custom_target(doc ALL
                  COMMAND doxygen Doxyfile
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc
                  VERBATIM)

add_executable(slidesync "src/util.cpp" "src/Quad.cpp" "src/SyncInstructions.cpp" "src/CVCanvas.cpp" "src/Loops.cpp" "src/IMhelpers.cpp" "src/SlideSync.cpp")
target_link_libraries(slidesync ${ImageMagick_LIBRARIES} ${OpenCV_LIBS} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})
