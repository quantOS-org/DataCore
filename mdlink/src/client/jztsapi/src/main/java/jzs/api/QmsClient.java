package jzs.api;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import org.zeromq.*;

import com.google.protobuf.InvalidProtocolBufferException;

import jzs.msg.Jzs;
import jzs.msg.Jzs.MsgType;
import jzs.msg.md.Md;
import jzs.msg.qms.Qms;

import static jzs.api.Utils.*;

public class QmsClient {
    
	private ZContext ctx = null;
	private ZMQ.Socket sock = null;
	
    private String addr = null;

	public QmsClient() {
		
	}
    public void connect(String addr) {
    	
    	this.addr = addr;
    	reconnect();
    }
    
    public void close() {
      
    	if (this.sock != null) {
    		this.sock.close();
    		this.sock = null;
    	}
    	if (this.ctx != null) {
    		this.ctx.close();
    		this.ctx = null;
    	}
    }
    
    private void reconnect() {
    	close();

        ctx = new ZContext();  
        sock = ctx.createSocket(ZMQ.REQ);
        sock.setReceiveTimeOut(2000);
        sock.setSendTimeOut(2000);
        sock.connect(addr);        
    	
    }

    
    public boolean sendRequest(byte[] data, int type) {

        try {
            byte[] head = new byte[8];
            ByteBuffer bb = ByteBuffer.wrap(head).order(ByteOrder.LITTLE_ENDIAN);
            bb.putInt(type);
            bb.putInt(data.length);

            return this.sock.send( combine(head, data));

        } catch (Exception e) {
			e.printStackTrace();
			reconnect();
			return false;
		}
    }

    interface MsgParser<T> {
    	public T parse(byte[] data) throws InvalidProtocolBufferException;
    }

    <T> T expect(int msgType, MsgParser<T> parser){
    	
    	try {
	    	byte[] data = this.sock.recv();
	    	if (data == null || data.length==0)
	    		return null;
	    	
	    	//System.out.println("qms recv: " + data.length);
	        
	        ByteBuffer bb = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
	        
	        int t_type = bb.getInt();
	        if ( t_type != msgType) {
	            System.out.println("Unexpected msg type:" + t_type);
	            return null;
	        }
	
	        int body_len = bb.getInt();
	
	        if ( (body_len & 0x80000000) != 0) {
	            body_len &= ~0x80000000;
	            assert data.length == (8 + body_len);
	            try {
	            	byte[] unzip_data = Utils.unzip(data, 8, data.length -8);
	            	return parser.parse(unzip_data);
	            }catch(Exception e) {
	            	System.out.println(e.getMessage());
	            	return null;
	            }
	        } else {
	        	return parser.parse( Arrays.copyOfRange(data, 8, data.length) );
	        }
    	} catch (Exception e) {
    		e.printStackTrace();
    		reconnect();
    		return null;
    	}
    }

    public Qms.StrategySubscribeRsp subscribe(int strategyID, String[] symbols) {
    	Qms.StrategySubscribeReq req = Qms.StrategySubscribeReq.newBuilder()
    					.setReqId(0)
    					.setStrategyId(strategyID)
    					.addAllSymbols(Arrays.asList(symbols)).build();
    	if (!sendRequest(req.toByteArray(), Jzs.MsgType.MSG_QMS_STRATEGY_SUBSCRIBE_REQ_VALUE))
    		return null;
    
	    try {
	    	Qms.StrategySubscribeRsp rsp = expect( MsgType.MSG_QMS_STRATEGY_SUBSCRIBE_RSP_VALUE, new MsgParser<Qms.StrategySubscribeRsp>() {
	    			public Qms.StrategySubscribeRsp parse(byte[] data ) throws InvalidProtocolBufferException {
	    				return Qms.StrategySubscribeRsp.parseFrom(data);
	    			} 
	    			} );
	
	    	return rsp; 
	    } catch (Exception e) {
	    	e.printStackTrace();
	    	return null;
	    }
    }


    public Qms.StrategyMarketQuotesRsp getStrategyQuotes(int strategy_id) {

        Qms.StrategyMarketQuotesReq req = Qms.StrategyMarketQuotesReq.newBuilder()
        					.setReqId(0)
        					.setStrategyId(strategy_id).build();

        if (!sendRequest(req.toByteArray(), Jzs.MsgType.MSG_QMS_STRATEGY_MARKETQUOTES_REQ_VALUE)) {
        	return null;
        }
        
	    try {
	    	Qms.StrategyMarketQuotesRsp rsp = expect( MsgType.MSG_QMS_STRATEGY_MARKETQUOTES_RSP_VALUE, new MsgParser<Qms.StrategyMarketQuotesRsp>() {
	    			public Qms.StrategyMarketQuotesRsp parse(byte[] data ) throws InvalidProtocolBufferException {
	    				return Qms.StrategyMarketQuotesRsp.parseFrom(data);
	    			} 
	    			} );
	
	    	return rsp; 
	    } catch (Exception e) {
	    	e.printStackTrace();
	    	return null;
	    }
    }


    public Md.MarketQuote getMarketQuote(String code) {
    	
    	Qms.MarketQuoteReq.Builder req = Qms.MarketQuoteReq.newBuilder();
    	
    	req.setSymbol(code);
        
        byte[] body = req.build().toByteArray();

        if (!sendRequest(body, Jzs.MsgType.MSG_QMS_MARKETQUOTE_REQ_VALUE)) {
        	System.out.println("Failed to send request");
        	return null;
        }

        try {
        	Qms.MarketQuoteRsp rsp = expect( MsgType.MSG_QMS_MARKETQUOTE_RSP_VALUE, new MsgParser<Qms.MarketQuoteRsp>() {
        			public Qms.MarketQuoteRsp parse(byte[] data ) throws InvalidProtocolBufferException {
        				return Qms.MarketQuoteRsp.parseFrom(data);
        			} 
        			} );

        	return (rsp!=null)? rsp.getData(): null;
        } catch (Exception e) {
        	e.printStackTrace();
        	return null;
        }
    }
    
    public Qms.Bar1M getBar1M(String code) {
    	
    	Qms.Bar1MReq req = Qms.Bar1MReq.newBuilder().setSymbol(code).build();
    	
    	if (!sendRequest(req.toByteArray(), Jzs.MsgType.MSG_QMS_BAR_1M_REQ_VALUE)) {
    		return null;
    	}

    	try {
        	Qms.Bar1MRsp rsp = expect( MsgType.MSG_QMS_BAR_1M_RSP_VALUE, new MsgParser<Qms.Bar1MRsp>() {
        			public Qms.Bar1MRsp parse(byte[] data ) throws InvalidProtocolBufferException {
        				return Qms.Bar1MRsp.parseFrom(data);
        			} 
        			} );

        	return (rsp!=null)? rsp.getData(): null;
        } catch (Exception e) {
        	e.printStackTrace();
        	return null;
        }
    }

	static public void main(String[] args) throws InterruptedException{
		QmsClient qms = new QmsClient();
		qms.connect("tcp://10.12.0.121:9000");
		
		long begin = System.currentTimeMillis();
		int loop = 1;
		for ( int i = 0; i < loop; i++)
			System.out.println(qms.getBar1M("jd1709.DCE"));
		
		System.out.println("used time: " + (System.currentTimeMillis() - begin )/1000);
		
	}
    
}
