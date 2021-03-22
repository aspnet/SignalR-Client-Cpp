if(@USE_CPPRESTSDK@)
  find_dependency(cpprestsdk)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/microsoft-signalr-targets.cmake")