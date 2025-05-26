cmake_minimum_required(VERSION 3.30)
project(simpleEncoder LANGUAGES CXX)

find_package(Threads REQUIRED)

option(CMAKE_CXX_NO_RTTI "Disable C++ RTTI" off)
option(ENABLE_FILE_LOG "Enable file log" on)
option(BUILD_TESTS "Build tests (gtest)" on)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find MP3LAME library
find_path(MP3LAME_INCLUDE_DIR lame/lame.h)
find_library(LAME_LIB
    NAMES mp3lame
    PATHS ${CMAKE_LIBRARY_PATH}
)

# Find Opus library
find_path(OPUS_INCLUDE_DIR opus/opus.h)
find_library(OPUS_LIB
    NAMES opus
    PATHS ${CMAKE_LIBRARY_PATH}
)

find_path(OPUSENC_INCLUDE_DIR opus/opusenc.h)
find_library(OPUSENC_LIB
    NAMES opusenc
    PATHS ${CMAKE_LIBRARY_PATH}
)

# Find FLAC library
find_path(FLAC_INCLUDE_DIR FLAC/stream_encoder.h)
find_library(FLAC_LIB
    NAMES FLAC
    PATHS ${CMAKE_LIBRARY_PATH}
)

find_path(CLI_INCLUDE_DIR CLI/CLI.hpp)

cmake_policy(SET CMP0167 OLD)

set(CORE_SRC
    core/DecoderWAV.cpp
)

set(UTILS_SRC
    utils/FileSystemHelper.cpp
    utils/Helper.cpp
    utils/Mp3FileWrapper.cpp
    utils/WaveFileWrapper.cpp
)

set(SOURCE
    main.cpp
)

include_directories(${LAME_BIN}/include)

add_executable(${PROJECT_NAME} ${CORE_SRC} ${UTILS_SRC} ${SOURCE})

if(ENABLE_FILE_LOG)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE ENABLE_LOG
        PRIVATE ENCODER_LOG_FILE="./encoding.log"
        PRIVATE DECODER_LOG_FILE="./decoding.log")
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${LAME_LIB}
    PRIVATE ${OPUS_LIB}
    PRIVATE ${OPUSENC_LIB}
    PRIVATE ${FLAC_LIB}
    PRIVATE ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC ${MP3LAME_INCLUDE_DIR}
    SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}
    SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}/opus
    SYSTEM PUBLIC ${OPUSENC_INCLUDE_DIR}
    SYSTEM PUBLIC ${FLAC_INCLUDE_DIR}
    SYSTEM PUBLIC ${CLI_INCLUDE_DIR}/CLI
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Testing setup
if(BUILD_TESTS)
    # Enable testing
    enable_testing()

    # Find GTest package
    find_package(GTest REQUIRED)

    # Create a list of test files
    set(TEST_SRC
        tests/FileSystemHelperTest.cpp
    )

    # Create test executable
    add_executable(fileSystemHelper_test ${TEST_SRC} ${UTILS_SRC})

    # Link against GTest and other required libraries
    target_link_libraries(fileSystemHelper_test
        PRIVATE GTest::GTest
        PRIVATE GTest::Main
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    )

    # Include directories
    target_include_directories(fileSystemHelper_test
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${GTEST_INCLUDE_DIRS}
    )

    # Add tests to CTest
    add_test(NAME UnitTests COMMAND fileSystemHelper_test)

    message(STATUS "Tests are enabled and will be built")
endif()
