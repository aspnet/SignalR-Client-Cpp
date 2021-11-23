include(CMakeFindDependencyMacro)

if(@USE_CPPRESTSDK@)
  find_dependency(cpprestsdk)
endif()

if(NOT @INCLUDE_JSONCPP@)
  find_dependency(@JSONCPP_LIB@)
endif()

if(@USE_MSGPACK@)
  find_dependency(msgpack)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/microsoft-signalr-targets.cmake")