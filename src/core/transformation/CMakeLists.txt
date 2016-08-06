#print current source directory
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})

#find all sourde files
file(GLOB src_cpp_tmp
    RELATIVE ${PROJECT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c++"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

#find all header files
file(GLOB src_h_tmp
    RELATIVE ${PROJECT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h++"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

#print used files of current directory
message(STATUS "cpp-file: " "${src_cpp_tmp}")
message(STATUS "h-file: " "${src_h_tmp}")

#append global lists for source and header files
set(src_cpp ${src_cpp} ${src_cpp_tmp} PARENT_SCOPE)
set(src_h ${src_h} ${src_h_tmp} PARENT_SCOPE)
