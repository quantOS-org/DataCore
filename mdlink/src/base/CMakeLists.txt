cmake_minimum_required(VERSION 2.8)
 
project(jzs)

set(CMAKE_INSTALL_PREFIX dist)

add_definitions(-fPIC)

set(SRC
    Init.h
    Init.cpp
    Logger.h
    StringUtil.cpp
    StringUtil.h
    TimeUtil.h
    TimeUtil.cpp
    TypeUtil.cpp
    TypeUtil.h
    Calendar.h
    Calendar.cpp
	csv-reader.h
	csv-reader.cpp
    ../config/CXML.cpp
    ../config/CXML.h
    ../config/SysConfig.cpp
    ../config/SysConfig.h
    ../config/XMLAction.cpp
    ../config/XMLAction.h
    ../../deps/jsoncpp/jsoncpp.cpp  
    ../../etc/jzs.json
)

if (WIN32)
list(APPEND SRC
    win32/getopt.c
)
else()
list(APPEND SRC
    linux/shared_ptr_atomic.cpp
    linux/shared_ptr_atomic.h

)
endif()

include_directories(
    ../../deps/jsoncpp
    ../
    ./
)

add_library(base STATIC ${SRC})

install(FILES
    Logger.h
    perf.h
    StringUtil.h
    TimeUtil.h
    DESTINATION client/cpp/include/base
)

install(TARGETS base
    DESTINATION client/cpp/lib )


