cmake_minimum_required ( VERSION 2.8 )
project(Jorge)

set(CMAKE_CXX_STANDARD 11)

include_directories(include/)

add_subdirectory(libs/luajit)
include_directories(libs/luajit/src)

set(SOURCE_FILES main.cpp jorgeLua.cpp jorgeNetwork.cpp)
#add_executable(SpacInvaders ${SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/meta/Program.rc)
add_executable(Jorge ${SOURCE_FILES})

target_link_libraries(Jorge liblua)


