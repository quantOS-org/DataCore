project(protocol)


set(SRC
    cpp/jzs.pb.cc
    cpp/jzs.pb.h
    cpp/md.pb.cc
    cpp/md.pb.h
    cpp/qms.pb.cc
    cpp/qms.pb.h
	cpp/misc.pb.h
	cpp/misc.pb.cc

    jzs.proto
    qms.proto
    md.proto
	misc.proto
)

add_library(protocol STATIC ${SRC})


