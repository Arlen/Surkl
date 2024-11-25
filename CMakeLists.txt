cmake_minimum_required(VERSION 3.26)
project(Ksed)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Qt6 COMPONENTS REQUIRED Widgets Gui Core Sql)

set(SOURCES main.cpp)
add_executable(Ksed ${SOURCES})

target_link_libraries(Ksed Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql)
set_property(TARGET Ksed PROPERTY AUTOMOC ON)


## Source Files
file(GLOB CORE_SRC_FILES "src/core/*.cpp")
file(GLOB GUI_SRC_FILES "src/gui/*.cpp")

target_sources(Ksed PUBLIC ${CORE_SRC_FILES})
target_sources(Ksed PUBLIC ${GUI_SRC_FILES})

## Headers
target_include_directories(Ksed PUBLIC "src")
target_include_directories(Ksed PUBLIC "src/core")
target_include_directories(Ksed PUBLIC "src/gui")


## disables all the APIs deprecated before Qt 6.5.0
target_compile_definitions(Ksed PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x060500)