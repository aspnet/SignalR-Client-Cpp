include(CMakeFindDependencyMacro)

if(@USE_CPPRESTSDK@)
  find_dependency(cpprestsdk)
endif()

find_dependency(jsoncpp)

if(@USE_MSGPACK@)
  find_dependency(msgpack)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/microsoft-signalr-targets.cmake")