cmake_minimum_required(VERSION 3.26)
project(Surkl)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wshadow -fsanitize=address")



### The main app
find_package(Qt6 COMPONENTS REQUIRED Widgets Gui Core Svg Sql Test)
qt6_standard_project_setup()

set(SOURCES main.cpp)
qt6_add_executable(surkl ${SOURCES})

target_link_libraries(surkl PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Svg Qt6::Sql)
set_property(TARGET surkl PROPERTY AUTOMOC ON)



## source files
file(GLOB CORE_SRC_FILES "src/core/*.cpp")
file(GLOB DB_SRC_FILES "src/db/*.cpp")
file(GLOB GUI_SRC_FILES "src/gui/*.cpp")
file(GLOB GUI_THEME_SRC_FILES "src/gui/theme/*.cpp")
file(GLOB GUI_VIEW_SRC_FILES "src/gui/view/*.cpp")
file(GLOB GUI_WINDOW_SRC_FILES "src/gui/window/*.cpp")

target_sources(surkl PUBLIC ${CORE_SRC_FILES})
target_sources(surkl PUBLIC ${DB_SRC_FILES})
target_sources(surkl PUBLIC ${GUI_SRC_FILES})
target_sources(surkl PUBLIC ${GUI_THEME_SRC_FILES})
target_sources(surkl PUBLIC ${GUI_VIEW_SRC_FILES})
target_sources(surkl PUBLIC ${GUI_WINDOW_SRC_FILES})

## headers
target_include_directories(surkl PUBLIC "src")
target_include_directories(surkl PUBLIC "src/core")
target_include_directories(surkl PUBLIC "src/db")
target_include_directories(surkl PUBLIC "src/gui")
target_include_directories(surkl PUBLIC "src/gui/theme")
target_include_directories(surkl PUBLIC "src/gui/view")
target_include_directories(surkl PUBLIC "src/gui/window")


## LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT IPOSupported OUTPUT error)

if (IPOSupported)
    message(STATUS "IPO enabled")
    set_property(TARGET surkl PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()


## Tracy
if (FALSE)
include(FetchContent)

FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(tracy)
include_directories(tracy/public)
target_link_libraries(surkl PRIVATE Tracy::TracyClient)
endif()



### Testing
enable_testing(true)
file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/test/*.cpp")

#Run through each source
foreach(ts ${TEST_SOURCES})
    #Extract the filename without an extension (NAME_WE)
    get_filename_component(test_name ${ts} NAME_WE)

    #Add compile target
    qt6_add_executable(${test_name} ${ts})
    #add_test(NAME ${test_name} COMMAND ${test_name})
    target_link_libraries(${test_name} PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Svg Qt6::Sql Qt6::Test)

    target_sources(${test_name} PUBLIC ${CORE_SRC_FILES})
    target_sources(${test_name} PUBLIC ${DB_SRC_FILES})
    target_sources(${test_name} PUBLIC ${GUI_SRC_FILES})
    target_sources(${test_name} PUBLIC ${GUI_THEME_SRC_FILES})
    target_sources(${test_name} PUBLIC ${GUI_VIEW_SRC_FILES})
    target_sources(${test_name} PUBLIC ${GUI_WINDOW_SRC_FILES})

    target_include_directories(${test_name} PUBLIC "src")
    target_include_directories(${test_name} PUBLIC "src/core")
    target_include_directories(${test_name} PUBLIC "src/db")
    target_include_directories(${test_name} PUBLIC "src/gui")
    target_include_directories(${test_name} PUBLIC "src/gui/theme")
    target_include_directories(${test_name} PUBLIC "src/gui/view")
    target_include_directories(${test_name} PUBLIC "src/gui/window")

    target_compile_definitions(${test_name} PRIVATE TEST_ANIMATIONS=1)
endforeach(ts)
