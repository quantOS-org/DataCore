cmake_minimum_required(VERSION 2.8)

project(jzs)
set(CMAKE_INSTALL_PREFIX dist)
add_definitions(
    -DUSE_PBMSG
    -DISLIB 
)

set(SRC
    ../../../deps/zmq/zhelpers.h
    ../../../deps/jsoncpp/jsoncpp.cpp
    ../public/MapTables.h
    ../public/MapTables.cpp
    ../public/MdServer.cpp
    ../public/MdServer.h    
    SinaApi.cpp
    SinaApi.h
    SinaMdServer.cpp
    SinaMdServer.h
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

add_executable(mdlink_sina ${SRC})

if (WIN32) 
target_link_libraries( mdlink_sina
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

target_link_libraries( mdlink_sina 
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

install (TARGETS mdlink_sina  RUNTIME DESTINATION ${BIN_PATH})
