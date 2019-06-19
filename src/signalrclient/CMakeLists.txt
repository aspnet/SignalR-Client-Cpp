set (SOURCES
  callback_manager.cpp
  connection.cpp
  connection_impl.cpp
  default_http_client.cpp
  default_websocket_client.cpp
  http_sender.cpp
  hub_connection.cpp
  hub_connection_impl.cpp
  logger.cpp
  negotiate.cpp
  signalr_client_config.cpp
  stdafx.cpp
  trace_log_writer.cpp
  transport.cpp
  transport_factory.cpp
  url_builder.cpp
  web_request.cpp
  web_request_factory.cpp
  websocket_transport.cpp
)

add_library (signalrclient SHARED ${SOURCES})

if(APPLE)
  target_link_libraries(signalrclient
    PUBLIC ${CPPREST_LIB}
    PRIVATE OpenSSL::SSL Boost::boost Boost::system Boost::chrono Boost::thread
  )
elseif(NOT WIN32)
  target_link_libraries(signalrclient
    PUBLIC ${CPPREST_LIB}
    PRIVATE OpenSSL::SSL
  )
else()
  target_link_libraries(signalrclient
    PUBLIC ${CPPREST_LIB}
  )
endif()

include(GNUInstallDirs)
install(TARGETS signalrclient LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
