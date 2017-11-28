cmake_minimum_required(VERSION 2.8)

project(jzs)
set(CMAKE_INSTALL_PREFIX dist)
add_definitions(
    -DUSE_PBMSG
    -DISLIB 
)

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

set(SRC
    ../../../deps/zmq/zhelpers.h
    ../../../deps/jsoncpp/jsoncpp.cpp
    ../public/MapTables.h
    ../public/MapTables.cpp
    ../public/MdServer.cpp
    ../public/MdServer.h    
    TencentApi.cpp
    TencentApi.h
    TencentMdServer.cpp
    TencentMdServer.h
    MdLinkMain.cpp
)

include_directories(
    ../../
    ../../../deps 
    ../../../deps/zmq    
    ../../../deps/jsoncpp
    ../../../deps/libcurl/include/curl
)


if (WIN32)
link_directories(    
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/libcurl/lib  
)

include_directories(
    ../../../deps/windows/zlib-1.2.8
)

else()

add_definitions(-std=c++11)


endif()

add_executable(mdlink_tencent ${SRC})

if (WIN32) 
target_link_libraries( mdlink_tencent
    libcurl
    base
    msgbus
    zmq   
    protobuf
    z
    glog
    ws2_32
)

else()

target_link_libraries( mdlink_tencent 
    libcurl.so
    base
    msgbus
    zmq   
    protobuf  
    glog  
    z 
)

endif()

if (WIN32)
  install (FILES
    ../../../deps/libcurl/lib/libcurl.dll
    DESTINATION ${LIB_PATH}
 )
endif()

install (TARGETS mdlink_tencent  RUNTIME DESTINATION ${BIN_PATH})
