cmake_minimum_required(VERSION 2.8)
 
project(mdlink)

set(CMAKE_INSTALL_PREFIX dist)

add_definitions(-DUSE_NAMESPACES=1)

set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps )

if (WIN32)
  add_definitions(
    -Dstrcasecmp=stricmp
    -Dsnprintf=_snprintf
    -D_CRT_NONSTDC_NO_WARNINGS
    -D_CRT_SECURE_NO_WARNINGS
    -D_CRT_SECURE_NO_DEPRECATE
    -D_CRT_NONSTDC_NO_DEPRECATE
  )
  include_directories(
    deps/windows/zeromq-4.0.7/include
    deps/windows/protobuf-2.5.0/src
    deps/windows/zlib-1.2.8
    deps/windows/port
    src/base/win32
  )

  # Remove warning of protobuf
  add_definitions(
    /wd4244 /wd4267 /wd4018 /wd4355 /wd4800 /wd4251 /wd4996 /wd4146 /wd4305 
  )

else ()
  include_directories(
    src/base/linux
  )
  add_definitions(
      -std=c++0x
      -finput-charset=GBK
      -fPIC
      -O2
      )
endif()

# Remove it?
add_definitions(-DUSE_PBMSG)

if (WIN32)
    set(BIN_PATH    bin)
    set(LIB_PATH    bin)
    set(DATA_PATH   data)
    set(ETC_PATH    etc)
else()
    set(BIN_PATH    bin)
    set(LIB_PATH    lib)
    set(DATA_PATH   data)
    set(ETC_PATH    etc)
endif()

if (WIN32)
  add_subdirectory(deps/windows/zeromq-4.0.7)
  add_subdirectory(deps/windows/protobuf-2.5.0)
  add_subdirectory(deps/windows/zlib-1.2.8)
endif()

add_subdirectory(deps/glog-0.3.4)
if (WIN32)
  include_directories( deps/glog-0.3.4/src/windows )
  add_definitions( -DGOOGLE_GLOG_DLL_DECL= )
  add_definitions( -DGLOG_NO_ABBREVIATED_SEVERITIES)
else ()
  include_directories( deps/glog-0.3.4/src/linux )
endif()

add_subdirectory(src/base)
add_subdirectory(src/msgbus)
add_subdirectory(src/mdlink/mdlink_merge)
add_subdirectory(src/mdlink/mdlink_ctp)
add_subdirectory(src/mdlink/mdlink_tdf)
add_subdirectory(src/qms)
add_subdirectory(src/client)
add_subdirectory(src/protocol)

# TODO: move to each module

install(FILES
    src/protocol/py/jzs_pb2.py
    src/protocol/py/qms_pb2.py
    src/protocol/py/md_pb2.py
    DESTINATION client/py/jzs )

install(DIRECTORY
    src/protocol/cpp
    DESTINATION client/cpp/include/protocol/ )

install (FILES
    etc/jzs.json
    etc/market.csv
    etc/instrument.csv
    etc/calendar.csv
    DESTINATION ${ETC_PATH}
)

install (PROGRAMS
    scripts/set-env.sh
    scripts/jztsctrl
    scripts/stop.sh
    scripts/start.sh
    scripts/start.bat
    scripts/prepare_data.sh
    scripts/prepare_data.bat
    scripts/download.py
    DESTINATION scripts
)

install(DIRECTORY data/flow     DESTINATION ${DATA_PATH})
install(DIRECTORY data/log      DESTINATION ${DATA_PATH})
install(DIRECTORY data/tk       DESTINATION ${DATA_PATH})
install(DIRECTORY data/ct       DESTINATION ${DATA_PATH})
install(DIRECTORY data/tmp      DESTINATION ${DATA_PATH})

if (UNIX)
  install (FILES
    deps/ctp/v6.3.6_linuxx64/libthostmduserapi.so
    deps/ctp/v6.3.6_linuxx64/libthosttraderapi.so
    deps/tdf/linux64/lib/libTDFAPI30.so

    DESTINATION ${LIB_PATH}
  )

else()
  install (FILES
    deps/ctp/v6.3.6_win/thostmduserapi.dll
    deps/tdf/win32/lib/TDFAPI30.dll

    DESTINATION ${LIB_PATH}
  )

endif()

