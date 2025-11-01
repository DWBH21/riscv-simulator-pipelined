cmake_minimum_required(VERSION 3.21)

project(vm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

set(SRC_DIR "src")
set(INCLUDE_DIR "include")
set(TEST_DIR "test")

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")

set(MAIN_SRC "${SRC_DIR}/main.cpp")
set(ASSEMBLER_MAIN_SRC "${SRC_DIR}/test/main_assembler.cpp")
set(RVSS_MAIN_SRC "${SRC_DIR}/test/main_rvss.cpp")
set(RV5S_MAIN_SRC "${SRC_DIR}/test/main_rv5s.cpp")

# Create a list of all library files    
set(LIB_SRC_FILES ${SRC_FILES})

# Remove all the main entry points -> this gives the list of only the library source files
list(REMOVE_ITEM LIB_SRC_FILES
    ${MAIN_SRC}
    ${ASSEMBLER_MAIN_SRC}
    ${RVSS_MAIN_SRC}
    ${RV5S_MAIN_SRC}
)

file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")

# A single library that all the executables will link against
add_library(vm_core ${LIB_SRC_FILES})

target_include_directories(vm_core PUBLIC ${INCLUDE_DIR})
target_compile_options(vm_core PUBLIC -Wall -Wextra -pedantic -frounding-math -ffloat-store -g -O3)
target_link_libraries(vm_core PUBLIC m)

# define all executables
add_executable(${PROJECT_NAME} ${MAIN_SRC})            # the original vm
add_executable(assembler_binary ${ASSEMBLER_MAIN_SRC})
add_executable(rvss_binary ${RVSS_MAIN_SRC})
add_executable(rv5s_binary ${RV5S_MAIN_SRC})

# link all executalbes against the core library
set(ALL_EXECUTABLES ${PROJECT_NAME} assembler_binary rvss_binary rv5s_binary)
foreach(exec ${ALL_EXECUTABLES})
    target_link_libraries(${exec} PRIVATE vm_core)
endforeach()

if(ENABLE_TESTS)
    find_package(GTest REQUIRED)
    
    # Build tests using the *library* files and the test files
    add_executable(tests ${LIB_SRC_FILES} ${TEST_FILES})
    
    # Link tests against vm_core and GTest
    target_link_libraries(tests PRIVATE vm_core GTest::GTest GTest::Main pthread)
    
    add_custom_target(test_run
        COMMAND ./tests
        DEPENDS tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()


# tests
option(ENABLE_TESTS "Build tests" OFF)

if(ENABLE_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    list(REMOVE_ITEM SRC_FILES "${CMAKE_SOURCE_DIR}/src/main.cpp")
    add_executable(tests ${SRC_FILES} ${TEST_FILES})
    target_link_libraries(tests GTest::GTest GTest::Main pthread)
    add_custom_target(test_run
        COMMAND ./tests
        DEPENDS tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()


add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(debug
    COMMAND gdb ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(valgrind
    COMMAND valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)



find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen"
    )
endif()

message(STATUS "Done")
