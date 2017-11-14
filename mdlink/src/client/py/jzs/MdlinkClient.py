import sys
#sys.path.append( './jzsapi')
import jzs

import zmq
import time
import datetime
import threading
import traceback


class MdlinkClient (threading.Thread):

    def __init__(self, addr, callback) :
        threading.Thread.__init__(self)
        self._addr = addr
        self._callback = callback
        pass
    
    def run(self):
        ctx = zmq.Context()  
        sock = ctx.socket(zmq.SUB)
        sock = ctx.socket(zmq.SUB)
        sock.setsockopt(zmq.SUBSCRIBE, '')
        sock.connect(self._addr)
        
        while True:
            try:
                data = sock.recv()
                if not data:
                    time.sleep(0.1)
                    continue
            except :
                traceback.print_exc()
                time.sleep(0.1)
                continue
            msg = jzs.jzs_pb2.Msg()
            msg.ParseFromString(data[4:])
    
            if msg.head.tid != jzs.jzs_pb2.MSG_MD_MARKETDATA_IND:
                print "Unexpect msg type ", msg.head.tid
                continue

            ind = jzs.md_pb2.MarketDataInd()
            ind.ParseFromString(msg.body)
            
            if self._callback:
                self._callback(ind)
            else:
                #print ind
                if ind.HasField('fut'):
                    print ind.fut.jzcode, ind.fut.last
                elif ind.HasField('stk'):
                    print ind.stk.jzcode, ind.stk.last

        self._ctx = context
        self._sock = socket
        self._dst_node = dst_node
        
def test():
    mdlink = MdlinkClient('tcp://127.0.0.1:8700', None)
    mdlink.start()
    mdlink.join()

if __name__ == '__main__':
    test()



