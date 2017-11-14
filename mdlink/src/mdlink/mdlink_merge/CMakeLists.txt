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
    ../public/MapTables.h
    ../public/MapTables.cpp
    MdLinkMain.cpp   
    MergeMdServer.cpp
    MergeMdServer.h
)

include_directories(
     ../../
    ../../../deps  
    ../../../deps/zmq   
    ../../../deps/jsoncpp
)

add_executable(mdlink2 ${SRC})

target_link_libraries(mdlink2
    base
    msgbus
    zmq   
    protobuf  
    glog   
)

#file(MAKE_DIRECTORY db)

install (TARGETS mdlink2 RUNTIME DESTINATION ${BIN_PATH})


