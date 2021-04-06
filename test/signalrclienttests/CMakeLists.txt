set (SOURCES
  callback_manager_tests.cpp
  case_insensitive_comparison_utils_tests.cpp
  connection_impl_tests.cpp
  handshake_tests.cpp
  hub_connection_tests.cpp
  hub_exception_tests.cpp
  json_hub_protocol_tests.cpp
  logger_tests.cpp
  memory_log_writer.cpp
  negotiate_tests.cpp
  signalrclienttests.cpp
  stdafx.cpp
  test_http_client.cpp
  test_utils.cpp
  test_websocket_client.cpp
  url_builder_tests.cpp
  websocket_transport_tests.cpp
)

include_directories(
  ../../src/signalrclient
)

# include main library sources for "internals visible to"
list (APPEND SOURCES
  ../../src/signalrclient/callback_manager.cpp
  ../../src/signalrclient/connection.cpp
  ../../src/signalrclient/connection_impl.cpp
  ../../src/signalrclient/default_http_client.cpp
  ../../src/signalrclient/default_websocket_client.cpp
  ../../src/signalrclient/handshake_protocol.cpp
  ../../src/signalrclient/hub_connection.cpp
  ../../src/signalrclient/hub_connection_builder.cpp
  ../../src/signalrclient/hub_connection_impl.cpp
  ../../src/signalrclient/json_helpers.cpp
  ../../src/signalrclient/json_hub_protocol.cpp
  ../../src/signalrclient/logger.cpp
  ../../src/signalrclient/negotiate.cpp
  ../../src/signalrclient/signalr_client_config.cpp
  ../../src/signalrclient/signalr_value.cpp
  ../../src/signalrclient/trace_log_writer.cpp
  ../../src/signalrclient/transport.cpp
  ../../src/signalrclient/transport_factory.cpp
  ../../src/signalrclient/url_builder.cpp
  ../../src/signalrclient/websocket_transport.cpp
  ../../third_party_code/cpprestsdk/uri.cpp
  ../../third_party_code/cpprestsdk/uri_builder.cpp
)

include_directories(
  ../../third_party_code/cpprestsdk
)

add_executable (signalrclienttests ${SOURCES})

if(INCLUDE_JSONCPP)
  target_sources(signalrclienttests PRIVATE ../../third_party_code/jsoncpp/jsoncpp.cpp)
  target_include_directories(signalrclienttests PRIVATE ../../third_party_code/jsoncpp)
else()
  target_link_libraries(microsoft-signalr PUBLIC ${JSONCPP_LIB})
endif()

target_link_libraries(signalrclienttests gtest)
add_test(NAME signalrclienttests COMMAND signalrclienttests)
