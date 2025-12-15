cmake_minimum_required(VERSION 3.20)

#Core library - deterministic engine
    project(aegis_core VERSION 0.1.0 LANGUAGES CXX)

#Collect all source files
        file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#Exclude demo files from library
            list(FILTER CORE_SOURCES EXCLUDE REGEX ".*demo.*\\.cpp$")

                file(GLOB_RECURSE CORE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

#Create static library
                    add_library(aegis_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

#Set C++ 23 standard
                        target_compile_features(aegis_core PUBLIC cxx_std_23)

#Include directory
                            target_include_directories(
                                aegis_core PUBLIC
                                    $<BUILD_INTERFACE : ${CMAKE_CURRENT_SOURCE_DIR} /..>
                                        $<INSTALL_INTERFACE : include>)

#Compiler - specific flags
                                if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") target_compile_options(
                                    aegis_core PRIVATE - Wall - Wextra - Wpedantic - Werror $ < $ <
                                        CONFIG
                                    : Release >
                                    : -O3 > $ < $ < CONFIG
                                    : Release >
                                    : -march =
                                          native >) elseif(MSVC) target_compile_options(aegis_core PRIVATE /
                                                                                                W4 /
                                                                                                WX $ <
                                                                                            $ <
                                                                                            CONFIG
                                                                                        : Release >
                                                                                        : / O2 >) endif()

#Enable clang - tidy for static analysis
                                    option(ENABLE_CLANG_TIDY "Enable clang-tidy static "
                                                             "analysis" ON) if (
                                        ENABLE_CLANG_TIDY) find_program(CLANG_TIDY_EXE NAMES
                                                                        "clang-tidy") if (CLANG_TIDY_EXE)
                                        set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}") set_target_properties(
                                            aegis_core PROPERTIES CXX_CLANG_TIDY
                                            "${CLANG_TIDY_COMMAND}")
                                            message(STATUS "clang-tidy enabled for "
                                                           "aegis_core") else() message(
                                                WARNING
                                                "clang-tidy requested but not found") endif() endif()

#Optional : Build demo executable
                                                option(BUILD_MEMORY_DEMO "Build memory allocator "
                                                                         "demonstration" ON) if (
                                                    BUILD_MEMORY_DEMO) add_executable(memory_demo
                                                                                          memory /
                                                                                      demo_allocators
                                                                                          .cpp)
                                                    target_link_libraries(
                                                        memory_demo PRIVATE
                                                            aegis_core) target_compile_features(memory_demo PRIVATE
                                                                                                    cxx_std_23)

#Compiler - specific flags for demo
                                                        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|"
                                                                                          "Clang") target_compile_options(
                                                            memory_demo PRIVATE - Wall - Wextra -
                                                                    Wpedantic $ <
                                                                $ < CONFIG
                                                            : Release >
                                                            : -O3 >) elseif(MSVC)
                                                            target_compile_options(
                                                                memory_demo PRIVATE / W4 $ < $ <
                                                                    CONFIG
                                                                : Release >
                                                                : / O2 >) endif() endif()

#Optional : Build state demo executable
                                                                option(BUILD_STATE_DEMO "Build "
                                                                                        "immutable "
                                                                                        "state "
                                                                                        "demonstrat"
                                                                                        "io"
                                                                                        "n" ON) if (
                                                                    BUILD_STATE_DEMO) add_executable(state_demo
                                                                                                         state /
                                                                                                     demo_state
                                                                                                         .cpp)
                                                                    target_link_libraries(
                                                                        state_demo PRIVATE
                                                                            aegis_core)
                                                                        target_compile_features(
                                                                            state_demo PRIVATE
                                                                                cxx_std_23)

#Compiler - specific flags for demo
                                                                            if (CMAKE_CXX_COMPILER_ID MATCHES
                                                                                "GNU|Clang")
                                                                                target_compile_options(
                                                                                    state_demo PRIVATE -
                                                                                            Wall -
                                                                                            Wextra -
                                                                                            Wpedantic
                                                                                                $ <
                                                                                        $ < CONFIG
                                                                                    : Release >
                                                                                    : -O3 >) elseif(MSVC)
                                                                                    target_compile_options(
                                                                                        state_demo PRIVATE /
                                                                                                W4 $ <
                                                                                            $ <
                                                                                            CONFIG
                                                                                        : Release >
                                                                                        : / O2 >)
                                                                                        endif()
                                                                                            endif()

#Ensure no OS / runtime headers are included in core
#This is enforced by the compile - time checks and architecture contract
