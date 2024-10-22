#set the minimum cmake version required
cmake_minimum_required(VERSION 3.16.0)

#set the name of the project
project(TCG)

#enable c++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#produce clang tidy file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB MAIN_SOURCES
    *.cpp
    *.h
    *.hpp
)
add_executable(TCG ${MAIN_SOURCES})
set_target_properties(TCG PROPERTIES LINKER_LANGUAGE CXX)

#set the root source directory as include directory
target_include_directories(TCG PUBLIC ${CMAKE_SOURCE_DIR})

#enable warning and treat them as errors
if (MSVC)
    target_compile_options(TCG PRIVATE /W4 /WX)
else()
    target_compile_options(TCG PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

#copy needed files
file(GLOB FILES
    "../resources/*"
)
file(COPY ${FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
