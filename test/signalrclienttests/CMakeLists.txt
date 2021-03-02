

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
  websocket_transport_tests.cpp)

include_directories(
    ../../src/signalrclient)

add_executable (signalrclienttests ${SOURCES})

target_link_libraries(signalrclienttests gtest signalrclient)
add_test(NAME signalrclienttests COMMAND signalrclienttests)
