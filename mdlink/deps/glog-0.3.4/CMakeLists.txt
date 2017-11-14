project(glog)

set(SRC
    src/logging.cc
    src/raw_logging.cc
    src/utilities.cc
    src/vlog_is_on.cc
    src/symbolize.cc
    src/demangle.cc
)

if (WIN32)
  list(APPEND SRC
    src/windows/port.cc
  )
  include_directories( src/windows )
  add_definitions( -DGOOGLE_GLOG_DLL_DECL= )
else()
  include_directories(
    src/linux
    src)
  add_definitions (
    -DHAVE_CONFIG_H -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare
    -DNO_FRAME_POINTER
    -DNDEBUG)
endif()

add_library(glog STATIC ${SRC})

if (UNIX)
  target_link_libraries( glog pthread)
endif()


