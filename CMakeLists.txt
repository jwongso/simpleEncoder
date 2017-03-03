project(simpleEncoder)
cmake_minimum_required(VERSION 3.2)

find_package(Threads REQUIRED)

set(LAME_DIR lame-3.99.5)

option(CMAKE_CXX_NO_RTTI "Disable C++ RTTI" off)
option(ENABLE_FILE_LOG "Enable file log" on)
option(BUILD_TESTS_APP "Build tests (gtest)" off)

# We want to use c++11 features
set(CMAKE_CXX11_EXTENSION_COMPILE_OPTION "-std=gnu++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX11_EXTENSION_COMPILE_OPTION}")

IF(MSVC)
    set(LAME_BIN ${CMAKE_CURRENT_SOURCE_DIR}/externals/win)
    set(LAME_PREFIX "")
    set(LAME_SUFFIX ".lib")
else()
    if(APPLE)
        set(LAME_BIN ${CMAKE_CURRENT_SOURCE_DIR}/externals/mac)
    else()
        if(CMAKE_SYSTEM_NAME STREQUAL "QNX") # Needs to be set via toolchain file.
            set(LAME_BIN ${CMAKE_CURRENT_SOURCE_DIR}/externals/qnx)
        else()
            set(LAME_BIN ${CMAKE_CURRENT_SOURCE_DIR}/externals/linux)
        endif()
    endif()
    set(LAME_PREFIX "lib")
    set(LAME_SUFFIX ".a")
endif()

if(CMAKE_CXX_NO_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

include(ExternalProject)
ExternalProject_Add(libmp3lame
    URL https://sourceforge.net/projects/lame/files/lame/3.99/lame-3.99.5.tar.gz/download
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/externals/${LAME_DIR}
    CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/externals/${LAME_DIR}/configure --prefix=${LAME_BIN}
    BUILD_COMMAND ${MAKE})

set(SOURCE
    main.cpp)

aux_source_directory(common SRC_COMMON)
aux_source_directory(utils SRC_UTILS)
aux_source_directory(core SRC_CORE)

if(BUILD_TESTS_APP)
    message(STATUS "-- Tests are not yet implemented! --")
endif()

list(APPEND ALL_FILES
    ${SOURCE}
    ${SRC_COMMON}
    ${SRC_CORE}
    ${SRC_UTILS})

include_directories(${LAME_BIN}/include)

add_executable(${PROJECT_NAME} ${ALL_FILES})

if(ENABLE_FILE_LOG)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE ENABLE_LOG
        PRIVATE ENCODER_LOG_FILE="./encoding.log"
        PRIVATE DECODER_LOG_FILE="./decoding.log")
endif()

set(LAME_LIBRARY ${LAME_BIN}/lib/${LAME_PREFIX}mp3lame${LAME_SUFFIX})

add_dependencies(${PROJECT_NAME} libmp3lame)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${LAME_LIBRARY}
    PRIVATE ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${PROJECT_NAME}
    SYSTEM PUBLIC ${LAME_BIN}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
