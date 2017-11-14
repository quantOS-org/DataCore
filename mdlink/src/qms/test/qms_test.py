file:///home/txu/work/jzs/trunk/src/qms/test/test.py#!/usr/bin/python

import zmq
import zlib

import qms_pb2

def int2bytes(buf, offset, value) :
    buf[offset+0] = value & 0xFF
    buf[offset+1] = (value>>8) & 0xFF
    buf[offset+2] = (value>>16) & 0xFF
    buf[offset+3] = (value>>24) & 0xFF
        
def bytes2int(buf, offset):
    return  buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24)


class QmsClient :
    
    def __init__(self) :
        self._ctx = None
        self._sock = None
     
    def connect(self, addr) :
        context = zmq.Context()  
        socket = context.socket(zmq.REQ)
        socket.connect(addr)
        
        self._ctx = context
        self._sock = socket

    def expect(self, msg_type, msg_class) :
        msg = self._sock.recv()
        
        head = bytearray(msg[:8]) #, encoding="utf8")

        t_type = bytes2int(head, 0)
        if t_type != msg_type :
            print "Unexpected msg type:", t_type

        body_len = bytes2int(head, 4)

        rsp = msg_class()

        if body_len & 0x80000000:
            body_len &= ~0x80000000
            print "body_len ", body_len, len(msg)
            assert len(msg) == (8 + body_len)
            rsp.ParseFromString( zlib.decompress(msg[8:]) )
        else:
            rsp.ParseFromString(msg[8:])
       
        return rsp

    def getMarketQuote(self, code) :
        a = code.split('.')
        if len(a)!=2 :
            print "code is not correct."
            return None
        req = qms_pb2.QmsMarketQuoteReq()
        req.symbol = a[0] #"IF1506"
        req.market = a[1] #"CF"

        body = req.SerializeToString()

        head = bytearray(8)
        int2bytes(head, 0, qms_pb2.MSG_QMS_MARKETQUOTE_REQ)
        int2bytes(head, 4, len(body))

        self._sock.send(head + body)

        #for i in range(0, len(data)) : print i, "=", ord(data[i])


        rsp = self.expect(qms_pb2.MSG_QMS_MARKETQUOTE_RSP, qms_pb2.QmsMarketQuoteRsp)
        if rsp:
            return rsp.data

    def getBar1M(self, code) :
        a = code.split('.')
        if len(a)!=2 :
            print "code is not correct."
            return None
        req = qms_pb2.QmsBar1MReq()
        req.symbol = a[0] #"IF1506"
        req.market = a[1] #"CF"

        body = req.SerializeToString()

        head = bytearray(8)
        int2bytes(head, 0, qms_pb2.MSG_QMS_BAR_1M_REQ)
        int2bytes(head, 4, len(body))

        self._sock.send(head + body)

        #for i in range(0, len(data)) : print i, "=", ord(data[i])
        rsp = self.expect(qms_pb2.MSG_QMS_BAR_1M_RSP, qms_pb2.QmsBar1MRsp)
        if rsp:
            return rsp.data




if __name__ == "__main__" :
        import sys
        if len(sys.argv)==1 :
            print "Usage:",sys.argv[0], " IF1506.CF ..."
            sys.exit(0)

        qms = QmsClient()
        qms.connect("tcp://localhost:8800")
        
        bar1M = qms.getBar1M(sys.argv[1])
        for bar in bar1M.bar:
            print bar.date, bar.time, bar.open, bar.high, bar.low, bar.close, bar.volume, bar.turnover, bar.interest

