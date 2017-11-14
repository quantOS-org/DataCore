#!/usr/bin/python

import zmq
import zlib
from datetime import datetime
import traceback

import qms_pb2
import jzs_pb2

def int2bytes(buf, offset, value) :
    buf[offset+0] = value & 0xFF
    buf[offset+1] = (value>>8) & 0xFF
    buf[offset+2] = (value>>16) & 0xFF
    buf[offset+3] = (value>>24) & 0xFF
        
def bytes2int(buf, offset):
    return  buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24)


class QmsClient :
    
    def __init__(self) :
        self._ctx = zmq.Context()  
        self._sock = None
     
    def connect(self, addr) :
        self._addr = addr
        self._check_connect()
        
    def _check_connect(self):
        
        if self._sock != None:
            return

        socket = self._ctx.socket(zmq.REQ)
        socket.setsockopt(zmq.RCVTIMEO, 2000)
        socket.setsockopt(zmq.SNDTIMEO, 2000)
        socket.connect(self._addr)
        
        self._sock = socket

    def close(self):
        if self._sock :
            self._sock.close()
            self._sock = None

    def _send(self, body, msg_type) :
        self._check_connect()

        head = bytearray(8)
        int2bytes(head, 0, msg_type)
        int2bytes(head, 4, len(body))

        try:
            return self._sock.send(head + body)
        except Exception, e:
            print "catch exception: " + str(e)
            self.close()
            raise e

    def _expect(self, msg_type, msg_class) :
        if not self._sock: return None

        try:
            msg = self._sock.recv()
            if msg is None or len(msg) <= 8:
                self.close()
                return None
        except Exception, e:
            self.close()
            raise e
        
        head = bytearray(msg[:8]) #, encoding="utf8")

        t_type = bytes2int(head, 0)
        if t_type != msg_type :
            print "Unexpected msg type:", t_type

        body_len = bytes2int(head, 4)

        rsp = msg_class()

        if body_len & 0x80000000:
            body_len &= ~0x80000000
            assert len(msg) == (8 + body_len)
            rsp.ParseFromString( zlib.decompress(msg[8:]) )
        else:
            rsp.ParseFromString(msg[8:])
       
        return rsp

    def subscribe(self, strategy_id, symbol_list):
        req = qms_pb2.StrategySubscribeReq()
        req.req_id = 0
        req.strategy_id = strategy_id
        for s in symbol_list:
            req.symbols.append(s)

        body = req.SerializeToString()

        self._send(body, jzs_pb2.MSG_QMS_STRATEGY_SUBSCRIBE_REQ)
        rsp = self._expect(jzs_pb2.MSG_QMS_STRATEGY_SUBSCRIBE_RSP, qms_pb2.StrategySubscribeRsp)
        if rsp:
            return rsp.result, rsp.err_symbols
        else:
            return false, symbol_list

    def getStrategyQuotes(self, strategy_id):
        req = qms_pb2.StrategyMarketQuotesReq()
        req.req_id = 0
        req.strategy_id = strategy_id

        body = req.SerializeToString()

        self._send(body, jzs_pb2.MSG_QMS_STRATEGY_MARKETQUOTES_REQ)
        rsp = self._expect(jzs_pb2.MSG_QMS_STRATEGY_MARKETQUOTES_RSP, qms_pb2.StrategyMarketQuotesRsp)
        if rsp:
            info = { }
            if rsp.HasField('middle_age'):
                info['MIDDLE_AGE'] = rsp.middle_age
            return rsp.quotes, info
        else:
            return None, None

    def getMarketQuote(self, code) :
        req = qms_pb2.MarketQuoteReq()
        req.symbol = code

        body = req.SerializeToString()

        self._send(body, jzs_pb2.MSG_QMS_MARKETQUOTE_REQ)

        rsp = self._expect(jzs_pb2.MSG_QMS_MARKETQUOTE_RSP, qms_pb2.MarketQuoteRsp)
        if rsp:
            return rsp.data

    def getBar1M(self, code) :

        req = qms_pb2.Bar1MReq()
        req.symbol = code

        body = req.SerializeToString()
        self._send(body, jzs_pb2.MSG_QMS_BAR_1M_REQ)

        #for i in range(0, len(data)) : print i, "=", ord(data[i])
        rsp = self._expect(jzs_pb2.MSG_QMS_BAR_1M_RSP, qms_pb2.Bar1MRsp)
        if rsp:
            return rsp.data


if __name__ == "__main__" :
    import sys
    if len(sys.argv)==1 :
        print "Usage:",sys.argv[0], " IF1508.CFE ..."
        sys.exit(0)

    addr = "tcp://localhost:9000"
    print "Connect to QMS ", addr
    qms = QmsClient()
    qms.connect(addr)
    
    bar1M = qms.getBar1M(sys.argv[1])
    for bar in bar1M.bar:
        print bar.date, bar.time, bar.open, bar.high, bar.low, bar.close, bar.volume, bar.turnover, bar.interest

