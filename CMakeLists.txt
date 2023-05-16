cmake_minimum_required(VERSION 3.13)  # CMake version check
project(c2c)               			  # Create project "c2c"
set(CMAKE_CXX_STANDARD 17)            # Enable c++17 standard

# Enable CMake test infrastructure
#include(CTest)

#-------Set global compilation options-------

#include(CheckIPOSupported)
#check_ipo_supported(RESULT supported OUTPUT error)
#
#if( supported )
#    message(STATUS "IPO / LTO enabled")
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#else()
#    message(STATUS "IPO / LTO not supported: <${error}>")
#endif()

add_compile_options(-march=native) 
#Add lots of warnings and all warnings as errors
# add_compile_options(-Wall -Wextra -pedantic -Werror )
#To get checks built in: 
# add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fsanitize=leak)


add_compile_definitions(NUM_OF_SAMPLES=20000)

include_directories("${CMAKE_SOURCE_DIR}/src/include")

#-------Generate the core library-------

FILE(GLOB_RECURSE CORE_SOURCE_FILES src/core/**.c)
message(STATUS "Found core sources ${CORE_SOURCE_FILES}")
#Alternative is SHARED
add_library(c2c STATIC ${CORE_SOURCE_FILES})
target_link_libraries(c2c m pthread)


#-------Generate basic use cases -------

add_executable(tc2c 			src/usecases/tc2c.c )
target_link_libraries(tc2c c2c)
add_executable(alltoall 		src/usecases/alltoall.c )
target_link_libraries(alltoall c2c)
add_executable(fixed-alltoall 	src/usecases/fixed-alltoall.c )
target_link_libraries(fixed-alltoall c2c)



#-------Detect optional dependencies for complex use cases-------

#Detect Boost (only needed for some usecases)
#set(Boost_VERBOSE ON)
find_package(Boost 1.74.0 COMPONENTS program_options )
message("BOOST includes at ${Boost_INCLUDE_DIRS}")
message("BOOST libraries are ${Boost_LIBRARIES}")
if(${BOOST_FOUND})
	include_directories(${Boost_INCLUDE_DIRS})
else(${BOOST_FOUND})
	message("BOOST was not found.")
endif(${BOOST_FOUND})

#Detect PAPI 
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(PAPI)
if(!${PAPI_FOUND})
	message("PAPI was not found.")
endif(!${PAPI_FOUND})

#-------Generate complex use cases -------

add_executable(llbench 	src/usecases/llbench.cpp )
target_link_libraries(llbench c2c)
target_link_libraries(llbench ${Boost_LIBRARIES})
if(${PAPI_FOUND})
	target_link_libraries(llbench ${PAPI_LIBRARIES})
endif(${PAPI_FOUND})


