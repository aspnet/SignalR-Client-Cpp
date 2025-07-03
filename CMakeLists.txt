cmake_minimum_required (VERSION 3.14)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project (microsoft-signalr)

include(CTest)

set(WERROR true CACHE BOOL "Enable warnings as errors.")
set(WALL true CACHE BOOL "Enable all warnings.")
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries")

if(NOT WIN32)
  set(EXTRA_FLAGS "-std=c++11 -fPIC -DNO_SIGNALRCLIENT_EXPORTS")
else()
  set(EXTRA_FLAGS "-DSIGNALRCLIENT_EXPORTS")
  # cpprestsdk hasn't updated to fix these warnings
  string(APPEND EXTRA_FLAGS " -D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")
endif()

if(USE_MSGPACK)
  string(APPEND EXTRA_FLAGS " -DUSE_MSGPACK")
endif()

if(INJECT_HEADER_AFTER_STDAFX)
  string(APPEND EXTRA_FLAGS " -DINJECT_HEADER_AFTER_STDAFX=${INJECT_HEADER_AFTER_STDAFX}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_LDFLAGS}")

if(USE_CPPRESTSDK)
  find_package(cpprestsdk REQUIRED)
  set(CPPREST_LIB "cpprestsdk::cpprest")
endif()

find_package(jsoncpp REQUIRED)
# jsoncpp has different target names depending on whether it
# was built as a shared or static library. We make this override-able
# so we can be resilient to the changes between versions.
if(WIN32)
set(JSONCPP_LIB "jsoncpp_lib" CACHE STRING "jsoncpp target name")
else()
set(JSONCPP_LIB "jsoncpp_static" CACHE STRING "jsoncpp target name")
endif()

if(USE_MSGPACK)
  find_package(msgpack CONFIG REQUIRED)
  set(MSGPACK_LIB "msgpackc-cxx")
endif()

include_directories (include)

if(USE_CPPRESTSDK)
  if(NOT WIN32)
    if(APPLE)
      file(GLOB OPENSSL_ROOT_DIR /usr/local/Cellar/openssl/*)
      list(REVERSE OPENSSL_ROOT_DIR)
    endif()

    find_package(OpenSSL 1.0.0 REQUIRED)
    find_package(Boost REQUIRED COMPONENTS system thread chrono)
  endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src/signalrclient)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

if(BUILD_SAMPLES)
  add_subdirectory(samples/HubConnectionSample)
endif()

install(DIRECTORY include/ DESTINATION include/)