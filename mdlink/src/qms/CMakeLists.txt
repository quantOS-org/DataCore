cmake_minimum_required(VERSION 2.8)
 
project(qms)


set(SRC
    Qms.h
    QmsData.cpp
    QmsData.h
    QmsMain.cpp
    QmsSvr.cpp
    QmsSvr.h
    QmsBar.h
    QmsBar.cpp
    main.cpp
)


include_directories(
    ../
)

add_executable(qms2 ${SRC})

target_link_libraries(qms2
    msgbus
    base
    zmq
    z
    glog
    protocol
)

install (TARGETS qms2 RUNTIME DESTINATION ${BIN_PATH})

