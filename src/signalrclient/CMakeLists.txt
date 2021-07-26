set (SOURCES
  callback_manager.cpp
  cancellation_token.cpp
  cancellation_token_source.cpp
  connection.cpp
  connection_impl.cpp
  default_http_client.cpp
  default_websocket_client.cpp
  handshake_protocol.cpp
  hub_connection.cpp
  hub_connection_builder.cpp
  hub_connection_impl.cpp
  json_helpers.cpp
  json_hub_protocol.cpp
  logger.cpp
  negotiate.cpp
  signalr_client_config.cpp
  signalr_value.cpp
  stdafx.cpp
  trace_log_writer.cpp
  transport.cpp
  transport_factory.cpp
  url_builder.cpp
  websocket_transport.cpp
  signalr_default_scheduler.cpp
  ../../third_party_code/cpprestsdk/uri.cpp
  ../../third_party_code/cpprestsdk/uri_builder.cpp
)

if(USE_MSGPACK)
  list (APPEND SOURCES
    binary_message_formatter.cpp
    binary_message_parser.cpp
    messagepack_hub_protocol.cpp
  )
endif()

include_directories(
  ../../third_party_code/cpprestsdk
)

add_library (microsoft-signalr ${SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(WERROR)
        target_compile_options(microsoft-signalr PRIVATE /WX)
    endif()
    if(WALL)
        target_compile_options(microsoft-signalr PRIVATE /Wall)
    endif()
    target_compile_options(microsoft-signalr PRIVATE
        /wd4820 # padding added after data member
        /wd4514 # unreferenced inline function removed
        /wd5045 # compiler will insert Spectre mitigation if /Qspectre switch is added
        /wd4464 # relative include paths
        /wd4711 # function 'x' selected for automatic inline expansion
        /wd4710 # function not inlined
        /experimental:external /external:anglebrackets /external:templates- /external:W0
        # /GR- # do not disable run-time type information, it can be needed by libraries we depend on, like boost
        /guard:cf # enable control-flow guard
        /EHa # enable C++ EH (w/ SEH exceptions)
    )
else()
    if(WERROR)
        target_compile_options(microsoft-signalr PRIVATE -Werror)
    endif()
    if(WALL)
        target_compile_options(microsoft-signalr PRIVATE -Wall)
    endif()

    target_compile_options(microsoft-signalr PRIVATE -Wextra -Wpedantic -Wno-unknown-pragmas)
endif()

if(INCLUDE_JSONCPP)
  target_sources(microsoft-signalr PRIVATE ../../third_party_code/jsoncpp/jsoncpp.cpp)
  target_include_directories(microsoft-signalr PRIVATE ../../third_party_code/jsoncpp)
else()
  target_link_libraries(microsoft-signalr PUBLIC ${JSONCPP_LIB})
endif()

if(NOT USE_CPPRESTSDK)
  target_link_libraries(microsoft-signalr)
else()
  set_target_properties(microsoft-signalr PROPERTIES COMPILE_FLAGS "-DUSE_CPPRESTSDK" )

  if(APPLE)
    target_link_libraries(microsoft-signalr
      PUBLIC ${CPPREST_LIB}
      PRIVATE OpenSSL::SSL Boost::boost Boost::system Boost::chrono Boost::thread
    )
  elseif(NOT WIN32)
    target_link_libraries(microsoft-signalr
      PUBLIC ${CPPREST_LIB} Boost::system
      PRIVATE OpenSSL::SSL
    )
  else()
    target_link_libraries(microsoft-signalr
      PUBLIC ${CPPREST_LIB}
    )
  endif()
endif() # USE_CPPRESTSDK

if(USE_MSGPACK)
  target_link_libraries(microsoft-signalr
    PRIVATE ${MSGPACK_LIB}
  )
endif() # USE_MSGPACK

include(GNUInstallDirs)

install(TARGETS microsoft-signalr
  # Creates the microsoft-signalr-targets.cmake file which allows find_package() to work
  EXPORT microsoft-signalr-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Create the microsoft-signalr-config.cmake with resolved values for variables used to build
configure_file(microsoft-signalr-config.in.cmake "${CMAKE_CURRENT_BINARY_DIR}/microsoft-signalr-config.cmake" @ONLY)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/microsoft-signalr-config.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/share/microsoft-signalr
)

install(EXPORT microsoft-signalr-targets
  FILE microsoft-signalr-targets.cmake
  NAMESPACE microsoft-signalr::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/share/microsoft-signalr
)