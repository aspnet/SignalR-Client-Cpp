set (SOURCES
  HubConnectionSample.cpp
)

include_directories(
    ../../include/signalrclient)

add_executable (HubConnectionSample ${SOURCES})

target_link_libraries(HubConnectionSample microsoft-signalr)
