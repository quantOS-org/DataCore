import jzs
import random
import pandas as pd


def connect(host, port, id):
    tdlink = jzs.TdlinkClient()
    tdlink.connect("tcp://%s:%d" % (host, port), id)
    return tdlink

    
if __name__ == "__main__" :
    td = connect("10.1.0.66", 8801, "tdlink_tdx")
    rsp = td.query_positions(212, want_symbol=True)
    print jzs.TdlinkClient.stocks_to_df(rsp.stocks)
    
