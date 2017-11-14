package jzs.api;

import jzs.msg.*;
import jzs.msg.md.Md;
import jzs.msg.md.Md.MarketDataInd;

import java.util.Arrays;

import org.zeromq.*;
import org.zeromq.ZMQ;

public class MdlinkClient extends java.lang.Thread {

	public interface Callback {
		public void onMarketDataInd(Md.MarketDataInd ind);
	}

	String addr = "";
	Callback cb = null;

	public MdlinkClient(String addr, Callback cb) {
		this.addr = addr;
		this.cb = cb;
	}
	
	public void run(){
		ZContext ctx = new ZContext();
		
		ZMQ.Socket sock = ctx.createSocket(ZMQ.SUB);
		byte[] tid = new byte[4];
		for (int i=0; i < 4; i++) {
			tid[i] = (byte)((Jzs.MsgType.MSG_MD_MARKETDATA_IND_VALUE >> (i*8)));
		}
		sock.subscribe(new byte[0]);//tid);
		sock.connect(this.addr);
		
		while (true) {
			try {
				byte[] data = sock.recv();
				if ( data == null || data.length == 0) {
					Thread.sleep(10);
					continue;
				}
				
				byte[] tmp = Arrays.copyOfRange(data, 4, data.length);
				Jzs.Msg msg = Jzs.Msg.parseFrom(tmp);
				
				if (msg==null)
					continue;
				
				if (msg.getHead().getTid() != Jzs.MsgType.MSG_MD_MARKETDATA_IND_VALUE)
					continue;

				Md.MarketDataInd ind = Md.MarketDataInd.parseFrom(msg.getBody());
				if (ind != null && this.cb!=null) {
					this.cb.onMarketDataInd(ind);
				}
					
			}catch(Exception e){
				e.printStackTrace();
			}
		}
	}
	
	
	public static void Test(String addr) {
        MdlinkClient mdlink = new MdlinkClient(addr, new MdlinkClient.Callback() {

            @Override
            public void onMarketDataInd(MarketDataInd ind) {
                System.out.println(ind);
            }
            
        });
        mdlink.start();
    }
    
    static public void main(String[] args) throws InterruptedException{
        Test("tcp://127.0.0.1:8700");
    }
}
