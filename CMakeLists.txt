cmake_minimum_required(VERSION 3.30)
project(ReADOFAIMacro)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(include)

add_executable(ReADOFAIMacro
        src/Main.cpp
        src/level.cpp
        src/PlayScript.cpp
        src/WinAPIController.cpp
)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

target_link_libraries(ReADOFAIMacro PRIVATE nlohmann_json::nlohmann_json)