project(HelloWorld)
cmake_minimum_required(VERSION 2.8)


#set includes
#include_directories(${PROJECT_SOURCE_DIR}/resources/boost_1_53_0)

#set libraries
#link_directories(${PROJECT_SOURCE_DIR}/resources/boost_1_53_0/stage/lib)

#configure packages
link_directories( "/usr/lib/" )
#find_package(  )

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

#set compiler
#set(CMAKE_CXX_COMPILER "g++")
#set(CMAKE_CC_COMPILER "gcc")
# set(CMAKE_CXX_FLAGS "-O0 -ggdb -fno-omit-frame-pointer -ffast-math -rdynamic -Wall -Wold-style-cast -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "-O4 -ggdb -ffast-math -Wall -Wold-style-cast -Woverloaded-virtual -fopenmp")
# set(CMAKE_CXX_FLAGS "-O4 -ggdb -ffast-math -Wall -Wold-style-cast -Woverloaded-virtual")

#print configuration
message(STATUS "")
message(STATUS "use configuration")
message(STATUS "cpp-files: " "${src_cpp}")
message(STATUS "h-files: " "${src_h}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

add_executable( HelloWorld ${src_h} ${src_cpp} )
target_link_libraries( HelloWorld jpeg turbojpeg log4cxx )

#if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    target_link_libraries(HelloWorld ...) 
#elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    target_link_libraries(HelloWorld ...) 
#endif()

