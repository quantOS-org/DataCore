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

    TdfMdServer.cpp
    TdfMdServer.h  
    ../public/MapTables.h
    ../public/MapTables.cpp
)

include_directories(
    ../../
    ../../../deps 
    ../../../deps/zmq
    ../../../deps/jsoncpp
)

if (WIN32)
  include_directories(   
    ../../../deps/tdf/win32/include
  )
else()
  include_directories(   
    ../../../deps/tdf/linux64/include
  )
endif()

link_directories(
    
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/tdf/linux64/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/tdf/win32/lib
    
)

add_executable(mdlink_tdf ${SRC})

target_link_libraries(mdlink_tdf
    base
    msgbus
    zmq   
    protobuf
    TDFAPI30
    glog   
)

#file(MAKE_DIRECTORY db)

install (TARGETS mdlink_tdf RUNTIME DESTINATION ${BIN_PATH})


