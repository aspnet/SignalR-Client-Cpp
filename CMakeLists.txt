cmake_minimum_required (VERSION 3.5)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project (signalrclient)

include(CTest)

if(NOT WIN32)
  set(EXTRA_FLAGS "-std=c++11 -fPIC -DNO_SIGNALRCLIENT_EXPORTS")
else()
  set(EXTRA_FLAGS "-DSIGNALRCLIENT_EXPORTS")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_LDFLAGS}")

find_package(cpprestsdk REQUIRED)
set(CPPREST_LIB "cpprestsdk::cpprest")

include_directories (include)

# TODO: We shouldn't use this, it makes the dll/lib export all symbols
# We need this for testing, but we might just add all the files to the test project manually
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(NOT WIN32)
  if(APPLE)
    file(GLOB OPENSSL_ROOT_DIR /usr/local/Cellar/openssl/*)
    list(REVERSE OPENSSL_ROOT_DIR)
  endif()

  find_package(OpenSSL 1.0.0 REQUIRED)
  find_package(Boost REQUIRED COMPONENTS system thread chrono)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src/signalrclient)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

install(DIRECTORY include/ DESTINATION include/)