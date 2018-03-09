

import java.util.Arrays;

public class Base64 {
	/** 
    * 将原始数据编码为base64编码 
    */  
    static public char[] encode(byte[] data)  
    {
    	int len_in = data.length;
    	int len_out = ((len_in + 2) / 3) * 4;
    	
    	short[] in = new short[len_in];
    	for(int i=0;i<len_in;i++)
    	{
    		if(data[i] < 0)
    		{
    			in[i] = (short) (256 + data[i]);
    		}
    		else
    		{
    			in[i] = (short) data[i];
    		}
    		
    	}
    	
    	char[] out = new char[len_out];
    	int j = 0;
    	int k = 0;
    	int len = len_in;
    	int index = 0;
    	
    	
    	while (len > 2) {
    		
    		index = ((int)in[k+0] >> 2) & 0x3f;
    		out[j++] = alphabet.charAt(index);
    		index = (((int)in[k+0] & 3) << 4) | ((int)in[k+1] >>> 4);
    		out[j++] = alphabet.charAt(index);
    		index = (((int)in[k+1] & 0x0f) << 2) | ((int)in[k+2] >>> 6);
    		out[j++] = alphabet.charAt(index);
    		index = (int)in[k+2] & 0x3f;
    		out[j++] = alphabet.charAt(index);

    		k += 3;
    		len -= 3;
    	}
    	
    	//字节数%3
    	if (len > 0) {
    		out[j++] = alphabet.charAt((in[k+0] >> 2) & 0x3f);

    		if (len == 1) {
    			out[j++] = alphabet.charAt((in[k+0] & 3) << 4);
    			out[j++] = '=';

    		} else {
    			out[j++] = alphabet.charAt(((in[k+0] & 3) << 4) | (in[k+1] >> 4));
    			out[j++] = alphabet.charAt((in[k+1] & 0x0f) << 2);
    		}

    		out[j++] = '=';
    	}
    	
        return out;  
    }  
    /** 
    * 将base64编码的数据解码成原始数据 
    */  
    static public byte[] decode(char[] data)  
    {
    	int in_len = data.length;
    	int i = 0;
    	int j = 0;
    	int in_ = 0;
    	char[] char_array_4 = new char[4];
    	int[] int_array_4 = new int[4];
    	byte[] byte_array_3 = new byte[3];
    	
        int len = ((data.length + 3) / 4) * 3;   
        byte[] out = new byte[len];
        int i_out = 0;
        char ch = '\0';
    	
    	while ((in_len-- > 0) && ( data[in_] != '=') && is_base64(data[in_] )) {
    		char_array_4[i++] = data[in_]; in_++;
    		if (i ==4) {
    			for (i = 0; i <4; i++)
    			{
    				int_array_4[i] = alphabet.indexOf(char_array_4[i]);
    			}

    			byte_array_3[0] = (byte) ((int_array_4[0] << 2) + ((int_array_4[1] & 0x30) >>> 4));
    			byte_array_3[1] = (byte) (((int_array_4[1] & 0xf) << 4) + ((int_array_4[2] & 0x3c) >>> 2));
    			byte_array_3[2] = (byte) (((int_array_4[2] & 0x3) << 6) + int_array_4[3]);

    			for (i = 0; (i < 3); i++)
    			{
    				out[i_out++] = byte_array_3[i];
    			}
    				
    			i = 0;
    		}
    	}
    	
    	//等号将被忽略
    	if (i > 0) {
    		for (j = i; j <4; j++)
    			char_array_4[j] = ch;//空字符 "" ASKII : 00000000

    		for (j = 0; j <4; j++)
    		{
    			int elm = alphabet.indexOf(char_array_4[j]);
    			int_array_4[j] = elm;
    		}
    			

    		byte_array_3[0] = (byte) ((int_array_4[0] << 2) + ((int_array_4[1] & 0x30) >> 4));
    		byte_array_3[1] = (byte) (((int_array_4[1] & 0xf) << 4) + ((int_array_4[2] & 0x3c) >> 2));
    		byte_array_3[2] = (byte) (((int_array_4[2] & 0x3) << 6) + int_array_4[3]);

    		for (j = 0; (j < i - 1); j++) 
    		{
    			out[i_out++] = byte_array_3[j];
    		}
    	}
    	
    	byte[] bt_Res = new byte[i_out];  
        System.arraycopy(out, 0, bt_Res, 0, i_out);  

    	return bt_Res;
    }  
  
    static boolean is_base64(char c) {
    	return (Character.isLetterOrDigit(c) || (c == '+') || (c == '/'));
    }
      
    static private String alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    static private byte[] codes = new byte[256];  
    static {  
            for (int i = 0; i < 256; i++)  
                    codes[i] = -1;  
            for (int i = 'A'; i <= 'Z'; i++)  
                    codes[i] = (byte) (i - 'A');  
            for (int i = 'a'; i <= 'z'; i++)  
                    codes[i] = (byte) (26 + i - 'a');  
            for (int i = '0'; i <= '9'; i++)  
                    codes[i] = (byte) (52 + i - '0');  
            codes['+'] = 62;  
            codes['/'] = 63;  
    }  
  
    public static void main(String[] args) throws Exception  
    {  
		//加密成base64
		String strSrc = "asdfasdfasdfasdfasfasdf";  
		String strOut = new String(Base64.encode(strSrc.getBytes()));  
		System.out.println("after Base64.decode strOut2="+strOut);
		System.out.println("after Base64.decode strOut2.length="+strOut.length());
		byte[] res = strOut.getBytes("UTF-8");
		System.out.println("after Base64.decode strOut2 bytes="+Arrays.toString(res));

		
		strOut = "5NnCrhhEc4OWfVMG/MQzpg==";  
		//String strOut2 = new String(Base64.decode(strOut.toCharArray()));
		byte[] resDecode = Base64.decode(strOut.toCharArray());
		System.out.println("after Base64.decode strOut2="+resDecode);
		System.out.println("after Base64.decode strOut2.length="+resDecode.length);
		System.out.println("after Base64.decode strOut2 bytes="+Arrays.toString(resDecode));
 
    }  
}
