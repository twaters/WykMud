cmake_minimum_required(VERSION 3.25.2 FATAL_ERROR)

project(WykMud)

set(CMAKE_CXX_STANDARD 20)

set (src
  main.cpp
  src/Database.cpp
  src/Server.cpp
  src/Session.cpp
  thirdparty/sqlite/sqlite3.c)

add_executable(WykMud ${src})
target_include_directories(WykMud PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
