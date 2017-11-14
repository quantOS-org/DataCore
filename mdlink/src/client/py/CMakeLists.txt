project(demo)


INSTALL(FILES
    demo1.py
    DESTINATION client/py/ )


INSTALL(FILES
    jzs/__init__.py
    jzs/QmsClient.py
    jzs/MdlinkClient.py
    DESTINATION client/py/jzs )
