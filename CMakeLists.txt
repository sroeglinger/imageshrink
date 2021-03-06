project(imageshrink)
cmake_minimum_required(VERSION 2.8)

#configure options
option(USE_LOG4CXX "enable logging with log4cxx" OFF)
if(USE_LOG4CXX)
    add_definitions("-DUSE_LOG4CXX")
endif()

option(USE_MACPORTS "use libraries from mac-ports (e.g. for log4cxx)" OFF)

#configure libraries
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # configure turbojpeg
    include_directories( "${PROJECT_SOURCE_DIR}/libs/libjpeg-turbo-1.5.0" )
    include_directories( "${PROJECT_SOURCE_DIR}/libs/libjpeg-turbo-1.5.0/build/" )
    link_directories( "${PROJECT_SOURCE_DIR}/libs/libjpeg-turbo-1.5.0/build/.libs" )

    # include other stuff; assume mac ports
    if(USE_MACPORTS)
        link_directories( "/opt/local/lib/" )
        include_directories( "/opt/local/include/" )
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    link_directories( "/usr/lib/" )
else()
    message(FATAL_ERROR "system name not supported!")
endif()

#set project source paths
file(GLOB_RECURSE listOfCMakeLists
    RELATIVE ${PROJECT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/src/*CMakeLists.txt"
)

foreach(element ${listOfCMakeLists})
    string(REPLACE "/CMakeLists.txt" "" path ${element})
    message(STATUS "")
    #message(STATUS "add_subdirectory: " ${path})
    add_subdirectory(${path})
    #message(STATUS "include_directories: " ${path})
    include_directories(${path})
endforeach()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    # assume clang
    # set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -ggdb -ffast-math -Wall -Wold-style-cast -Woverloaded-virtual -fopenmp-use-tls")
    set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -ggdb -ffast-math -Wall -Woverloaded-virtual -fopenmp-use-tls")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    # assume gcc
    # set(CMAKE_CXX_FLAGS "-O4 -ggdb -ffast-math -Wall -Wold-style-cast -Woverloaded-virtual -fopenmp")
    set(CMAKE_CXX_FLAGS "-O4 -ggdb -ffast-math -Wall -Woverloaded-virtual -fopenmp")
else()
    message(FATAL_ERROR "compiler ID not supported!")
endif()


#print configuration
message(STATUS "")
message(STATUS "use configuration")
message(STATUS "cpp-files: " "${src_cpp}")
message(STATUS "h-files: " "${src_h}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

add_executable( imageshrink ${src_h} ${src_cpp} )
target_link_libraries( imageshrink jpeg turbojpeg )

if(USE_LOG4CXX)
    target_link_libraries( imageshrink log4cxx )
endif()
