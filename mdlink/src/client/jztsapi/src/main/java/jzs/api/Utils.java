package jzs.api;

import java.util.zip.*;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;


public class Utils {
	
	public static byte[] zip(byte[] bytes, int offset, int len) {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            OutputStream out = new DeflaterOutputStream(baos);
            out.write(bytes, offset, len);
            out.close();
        } catch (IOException e) {
            throw new AssertionError(e);
        }
        return baos.toByteArray();		
	}

    public static byte[] zip(byte[] bytes) {
    	return zip(bytes, 0, bytes.length);
    }


    public static byte[] unzip(byte[] bytes) {
    	return unzip(bytes, 0, bytes.length);
    }

    public static byte[] unzip(byte[] bytes, int offset, int length) {
        InputStream in = new InflaterInputStream(new ByteArrayInputStream(bytes, offset, length));
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            byte[] buffer = new byte[8192];
            int len = 0;
            while((len = in.read(buffer))>0)
                baos.write(buffer, 0, len);
            return baos.toByteArray();
        } catch (IOException e) {
            throw new AssertionError(e);
        }
    }
    
//    static void int2bytes(byte[] buf, int offset, int value){
//	  buf[offset+0] = (byte)(value & 0xFF);
//	  buf[offset+1] = (byte)((value>>8) & 0xFF);
//	  buf[offset+2] = (byte)((value>>16) & 0xFF);
//	  buf[offset+3] = (byte)((value>>24) & 0xFF);
//    }
//  
//    static int bytes2int(byte[] buf, int offset) {
//    	return  buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24);
//    }
    
    static byte[] combine(byte[] a, byte[] b) {
    	byte[] c = new byte[a.length + b.length];
    	System.arraycopy(a, 0, c, 0, a.length);
    	System.arraycopy(b, 0, c, a.length, b.length);
    	return c;
    }    
}


