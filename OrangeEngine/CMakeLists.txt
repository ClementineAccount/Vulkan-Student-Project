# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("OrangeEngine")

# Include sub-projects.
add_subdirectory ("src")

# https://stackoverflow.com/questions/49069493/is-it-possible-not-to-generate-all-build-project-in-cmake
# CMake projects into seperate folder 
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 

# https://cmake.org/cmake/help/v3.6/prop_dir/VS_STARTUP_PROJECT.html
# Set the Startup Project to be the Orange Engine main
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OrangeEngine)