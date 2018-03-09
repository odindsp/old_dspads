import java.io.UnsupportedEncodingException;  
import javax.crypto.Mac;  
import javax.crypto.spec.SecretKeySpec;
import java.util.Arrays;
import java.nio.ByteBuffer;
import java.util.Base64;  
import javax.xml.bind.DatatypeConverter;
import java.math.BigInteger;

class PriceDec
{
    private byte[] encKey;
    private byte[] sigKey;

    public void Price()
    {

    }

    public void setEncKey(String encKey)
    {
        this.encKey = hex2bin(encKey);
    }

    public void setSigKey(String sigKey)
    {
        this.sigKey = hex2bin(sigKey);
    }

    public double decode(String base64Str)
    {
        byte[] temp = base64UrlDecode(base64Str);

        if (temp.length != 16) {
            throw new RuntimeException("base64UrlDecode string length less 16"); 
        }

        byte[] timeStamp = new byte[4];
        byte[] encPrice = new byte[8];
        byte[] sig = new byte[4];
        
        timeStamp = Arrays.copyOfRange(temp,0,4);
        encPrice = Arrays.copyOfRange(temp, 4,12);
        sig = Arrays.copyOfRange(temp, 12,16);

        byte[] pad = hex2bin(hmac_sha1(timeStamp, this.encKey));
        byte[] decPrice = xorBytes(pad, encPrice, 8);

        double d = byteArrToDouble(decPrice);

        int i=0;
        byte[] calByte = new byte[12];
        for (i=0;i<8;i++) {
            calByte[i] = decPrice[i];
        }
        for (i=0;i<4;i++) {
            calByte[8+i] = timeStamp[i];
        }

        byte[] osig = hex2bin(hmac_sha1(calByte, this.sigKey));
        byte[] compareSig = Arrays.copyOfRange(osig, 0,4);

        if (!Arrays.equals(sig, compareSig)) {
            throw new RuntimeException("sig not equal");
        }

        return d;
    }

    public byte[] xorBytes(byte[] padStr, byte[] encPrice, int n)
    {
        int i;  
        byte[] retArr = new byte[n];
        for (i=0;i<n;i++) {
            retArr[i] = (byte)(padStr[i]^encPrice[i]);
        }
        return retArr;
    }

    public byte[] base64UrlDecode(String base64Str)
    {
        base64Str = base64Str.replaceAll("-", "+");
        base64Str = base64Str.replaceAll("_", "/");
        int length = base64Str.length();
        int pad = 4-(length%4);
        int i;
        for (i=0;i<pad;i++) {
            base64Str += "=";
        }
        byte[] bytes = base64Str.getBytes();
        byte[] decoded = Base64.getDecoder().decode(base64Str);
        return decoded;
    }

    public double byteArrToDouble(byte[] b) {   
        long l;   
        l = b[0];   
        l &= 0xff;   
        l |= ((long) b[1] << 8);   
        l &= 0xffff;   
        l |= ((long) b[2] << 16);   
        l &= 0xffffff;   
        l |= ((long) b[3] << 24);   
        l &= 0xffffffffl;  
        l |= ((long) b[4] << 32);   
        l &= 0xffffffffffl;   
        l |= ((long) b[5] << 40);   
        l &= 0xffffffffffffl;   
        l |= ((long) b[6] << 48);   
        l &= 0xffffffffffffffl;   
        l |= ((long) b[7] << 56);   
        return Double.longBitsToDouble(l);  
    }

    public byte[] hex2bin(String hex) {
        char[] hex2char = hex.toCharArray();
        byte[] bytes = new byte[hex.length() / 2];
        int temp;
        for (int i = 0; i < bytes.length; i++) {
            temp = Character.digit(hex2char[2 * i], 16) * 16;
            temp += Character.digit(hex2char[2 * i + 1],16);
            bytes[i] = (byte) (temp & 0xff);
        }
        return bytes;
    }

    private String hmac_sha1(byte[] value, byte[] key) {  
        try {  
            byte[] keyBytes = key;          
            SecretKeySpec signingKey = new SecretKeySpec(keyBytes, "HmacSHA1");  
            Mac mac = Mac.getInstance("HmacSHA1");  
            mac.init(signingKey);  
            byte[] rawHmac = mac.doFinal(value);  
            String hexBytes = DatatypeConverter.printHexBinary(rawHmac); 
            return hexBytes;  
        } catch (Exception e) {  
            throw new RuntimeException(e);  
        }  
    }  
}


/**----------------begin使用范例---------------------**/
public class pricedec{
    public static void main(String args[]) {
        
        PriceDec p = new PriceDec();
        p.setEncKey("7183e751829e457488a8f9e13cdb1292");
        p.setSigKey("b4056881520040a0af16fdccb99d68bb");

        String base64Str = "JR9YVr-YW3HcCETOOcw5nA";
        System.out.println(p.decode(base64Str));
    }
}
/**----------------end  使用范例---------------------**/
