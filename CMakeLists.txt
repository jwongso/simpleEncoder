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

# Find Vorbis libraries
find_path(VORBIS_INCLUDE_DIR vorbis/vorbisenc.h)
find_library(VORBIS_LIB
    NAMES vorbis
    PATHS ${CMAKE_LIBRARY_PATH}
)
find_library(VORBISENC_LIB
    NAMES vorbisenc
    PATHS ${CMAKE_LIBRARY_PATH}
)
find_library(OGG_LIB
    NAMES ogg
    PATHS ${CMAKE_LIBRARY_PATH}
)

# Find FFmpeg libraries
find_path(FFMPEG_INCLUDE_DIR ffmpeg/libavcodec/avcodec.h)
find_library(AVCODEC_LIB
    NAMES avcodec
    PATHS ${CMAKE_LIBRARY_PATH}
)
find_library(AVFORMAT_LIB
    NAMES avformat
    PATHS ${CMAKE_LIBRARY_PATH}
)
find_library(AVUTIL_LIB
    NAMES avutil
    PATHS ${CMAKE_LIBRARY_PATH}
)
find_library(SWRESAMPLE_LIB
    NAMES swresample
    PATHS ${CMAKE_LIBRARY_PATH}
)

find_path(CLI_INCLUDE_DIR CLI/CLI.hpp)

cmake_policy(SET CMP0167 OLD)

set(CORE_SRC
    core/DecoderWAV.cpp
)

set(UTILS_SRC
    utils/FileSystemHelper.cpp
    utils/Mp3FileWrapper.cpp
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
    PRIVATE ${VORBIS_LIB}
    PRIVATE ${VORBISENC_LIB}
    PRIVATE ${OGG_LIB}
    PRIVATE ${AVCODEC_LIB}
    PRIVATE ${AVFORMAT_LIB}
    PRIVATE ${AVUTIL_LIB}
    PRIVATE ${SWRESAMPLE_LIB}
    PRIVATE ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC ${MP3LAME_INCLUDE_DIR}
    SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}
    SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}/opus
    SYSTEM PUBLIC ${OPUSENC_INCLUDE_DIR}
    SYSTEM PUBLIC ${FLAC_INCLUDE_DIR}
    SYSTEM PUBLIC ${VORBIS_INCLUDE_DIR}
    SYSTEM PUBLIC ${FFMPEG_INCLUDE_DIR}/ffmpeg
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
        tests/HelperTest.cpp
        tests/WaveFileWrapperTest.cpp
        tests/EncoderFunctionalTest.cpp
    )

    # Create test executable for FileSystemHelper
    add_executable(fileSystemHelper_test tests/FileSystemHelperTest.cpp ${UTILS_SRC})

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
    add_test(NAME FileSystemHelperTests COMMAND fileSystemHelper_test)

    # Create test executable for Helper
    add_executable(helper_test tests/HelperTest.cpp utils/FileSystemHelper.cpp)

    # Link against GTest and other required libraries
    target_link_libraries(helper_test
        PRIVATE GTest::GTest
        PRIVATE GTest::Main
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    )

    # Include directories
    target_include_directories(helper_test
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${GTEST_INCLUDE_DIRS}
    )

    # Add tests to CTest
    add_test(NAME HelperTests COMMAND helper_test)

    # Create test executable for WaveFileWrapper
    add_executable(waveFileWrapper_test tests/WaveFileWrapperTest.cpp utils/FileSystemHelper.cpp)

    # Link against GTest and other required libraries
    target_link_libraries(waveFileWrapper_test
        PRIVATE GTest::GTest
        PRIVATE GTest::Main
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    )

    # Include directories
    target_include_directories(waveFileWrapper_test
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${GTEST_INCLUDE_DIRS}
    )

    # Add tests to CTest
    add_test(NAME WaveFileWrapperTests COMMAND waveFileWrapper_test)

    # Create comprehensive encoder functional test executable
    add_executable(encoder_functional_test tests/EncoderFunctionalTest.cpp ${CORE_SRC} ${UTILS_SRC})

    # Link against GTest and all encoder libraries
    target_link_libraries(encoder_functional_test
        PRIVATE GTest::GTest
        PRIVATE GTest::Main
        PRIVATE ${LAME_LIB}
        PRIVATE ${OPUS_LIB}
        PRIVATE ${OPUSENC_LIB}
        PRIVATE ${FLAC_LIB}
        PRIVATE ${VORBIS_LIB}
        PRIVATE ${VORBISENC_LIB}
        PRIVATE ${OGG_LIB}
        PRIVATE ${AVCODEC_LIB}
        PRIVATE ${AVFORMAT_LIB}
        PRIVATE ${AVUTIL_LIB}
        PRIVATE ${SWRESAMPLE_LIB}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    )

    # Include directories for encoder test
    target_include_directories(encoder_functional_test
        SYSTEM PUBLIC ${MP3LAME_INCLUDE_DIR}
        SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}
        SYSTEM PUBLIC ${OPUS_INCLUDE_DIR}/opus
        SYSTEM PUBLIC ${OPUSENC_INCLUDE_DIR}
        SYSTEM PUBLIC ${FLAC_INCLUDE_DIR}
        SYSTEM PUBLIC ${VORBIS_INCLUDE_DIR}
        SYSTEM PUBLIC ${FFMPEG_INCLUDE_DIR}/ffmpeg
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${GTEST_INCLUDE_DIRS}
    )

    # Add encoder functional tests to CTest
    add_test(NAME EncoderFunctionalTests COMMAND encoder_functional_test)

    # Create audio validation test executable
    add_executable(audio_validation_test tests/AudioValidationTest.cpp ${UTILS_SRC})

    # Link against GTest and required libraries
    target_link_libraries(audio_validation_test
        PRIVATE GTest::GTest
        PRIVATE GTest::Main
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    )

    # Include directories for validation test
    target_include_directories(audio_validation_test
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${GTEST_INCLUDE_DIRS}
    )

    # Add audio validation tests to CTest
    add_test(NAME AudioValidationTests COMMAND audio_validation_test)

    message(STATUS "Tests are enabled and will be built")
    message(STATUS "Encoder functional tests included")
    message(STATUS "Audio validation tests included")
endif()
