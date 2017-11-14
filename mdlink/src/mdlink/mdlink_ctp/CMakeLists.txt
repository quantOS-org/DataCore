cmake_minimum_required(VERSION 2.8)
 
project(jzs)

set(CMAKE_INSTALL_PREFIX dist)

add_definitions(
    -DUSE_PBMSG
    -DISLIB # CTP library
)

set(SRC
    ../../../deps/zmq/zhelpers.h
    ../../../deps/jsoncpp/jsoncpp.cpp
    ../public/MdServer.cpp
    ../public/MdServer.h
    MdLinkMain.cpp    
    CtpMdServer.cpp
    CtpMdServer.h 
    ../public/MapTables.h
    ../public/MapTables.cpp
)

include_directories(
    ../../
    ../../../deps   
    ../../../deps/ctp
    ../../../deps/zmq   
    ../../../deps/jsoncpp
)

if (WIN32)
  include_directories(
    ../../../deps/ctp/v6.3.6_win/   
  )
else()
  include_directories(
    ../../../deps/ctp/v6.3.6_linuxx64/  
  )
endif()

link_directories(    
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/ctp/v6.3.6_win
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/ctp/v6.3.6_linuxx64   
)

add_executable(mdlink_ctp ${SRC})

target_link_libraries(mdlink_ctp
    base
    msgbus
    zmq
    thostmduserapi
    protobuf   
    glog   
)

#file(MAKE_DIRECTORY db)

install (TARGETS mdlink_ctp RUNTIME DESTINATION ${BIN_PATH})


