cmake_minimum_required(VERSION 2.8)
 
project(msgbus)

set(SRC
    ../../deps/zmq/zhelpers.h
    ../../deps/zmq/zhelpers.hpp

    Client.h
    Server.h
    Publisher.h
    Subscriber.h
    Event.h

    impl/ClientImpl.h
    impl/ClientImpl.cpp
    impl/ServerImpl.h
    impl/ServerImpl.cpp
    impl/PublisherImpl.h
    impl/PublisherImpl.cpp
    impl/SubscriberImpl.h
    impl/SubscriberImpl.cpp
    impl/MsgZmqCtx.cpp
    impl/MsgZmqCtx.h
)


include_directories(
    ..
    ../../deps/zmq
    impl
    .
)

if (WIN32)
  include_directories(
    ../../deps/protobuf-2.5.0/src
    ../../deps/zeromq-4.0.7/include
  )

  # Disable warning of protobuf
  add_definitions( /wd4244 )

endif()



add_definitions(-fPIC)

add_library(msgbus STATIC ${SRC})

target_link_libraries(msgbus
    zmq
    z
    protobuf
    protocol
    glog
)


install(FILES
    Event.h
    Client.h
    Server.h
    Publisher.h
    Subscriber.h
    DESTINATION client/cpp/include/msgbus)
    

install(TARGETS msgbus
    DESTINATION client/cpp/lib)
