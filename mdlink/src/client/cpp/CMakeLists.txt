project (jzs_api)


include_directories(include)
include_directories(../../)


if (WIN32)
  add_library(jzsapi STATIC src/QmsClient.cpp)
else()
  add_library(jzsapi SHARED src/QmsClient.cpp)
endif()

target_link_libraries(jzsapi base msgbus protocol zmq)

set(LIBS jzsapi base msgbus protocol zmq)

install (FILES
    include/jzs/QmsClient.h
    DESTINATION client/cpp/include/jzs
)

if (UNIX)
  install(TARGETS jzsapi LIBRARY DESTINATION ${LIB_PATH})
endif()

install(TARGETS jzsapi DESTINATION client/cpp/lib)
    


