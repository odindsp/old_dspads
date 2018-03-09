using System;
using System.Linq;
using System.Security.Cryptography;

class demo
{
	public static void Main(string[] args)
	{
		PriceDec a = new PriceDec();
		Console.WriteLine(a.decode("7183e751829e457488a8f9e13cdb1292","b4056881520040a0af16fdccb99d68bb","JR9YVr-YW3HcCETOOcw5nA"));
		Console.ReadKey(true);
	}
}
	
public class PriceDec
{
	
	public double decode(string enc_key, string sig_key, string price)
	{
		byte[] data = this.base64UrlDecode(price);
		if (data==null|| data.Length != 16)
		{
			throw new ArgumentException("The price is not a valid string");
		}
		
		byte[] encKey = hex2Bin(enc_key);
		byte[] sigKye = hex2Bin(sig_key);
		
		byte[] timeMsStamp = data.Take(4).ToArray();
		byte[] encPrice = data.Skip(4).Take(8).ToArray();
		byte[] sig = data.Skip(12).Take(4).ToArray();
		
		byte[] pad = hash_hmac(timeMsStamp, encKey);
		byte[] dec_price= xorBytes(pad, encPrice, 8);

		byte[] osigData = new byte[dec_price.Length + timeMsStamp.Length];
		dec_price.CopyTo(osigData,0);
		timeMsStamp.CopyTo(osigData, dec_price.Length);
		byte[] osig = hash_hmac(osigData, sigKye);
		
		if(!sig.SequenceEqual(osig.Take(4).ToArray())){
			throw new ArgumentException("The price is not a complete string");
		}
		
		return BitConverter.ToDouble(dec_price, 0);
	}
	
	private byte[] hex2Bin(string hexdata)
	{
		if (hexdata == null)
			throw new ArgumentNullException("hexdata");
		if (hexdata.Length % 2 != 0)
			throw new ArgumentException("hexdata should have even length");

		byte[] bytes = new byte[hexdata.Length / 2];
		for (int i = 0; i < hexdata.Length; i += 2)
			bytes[i / 2] = (byte)(HexValue(hexdata[i]) * 0x10
			                      + HexValue(hexdata[i + 1]));
		return bytes;
	}

	static int HexValue(char c)
	{
		int ch = (int)c;
		if (ch >= (int)'0' && ch <= (int)'9')
			return ch - (int)'0';
		if (ch >= (int)'a' && ch <= (int)'f')
			return ch - (int)'a' + 10;
		if (ch >= (int)'A' && ch <= (int)'F')
			return ch - (int)'A' + 10;
		throw new ArgumentException("Not a hexadecimal digit.");
	}


	private byte[] hash_hmac(byte[] signatureString, byte[] secretKey, bool raw_output = false)
	{
		HMACSHA1 hmac = new HMACSHA1(secretKey);
		hmac.ComputeHash(signatureString);
		return hmac.Hash;
	}

	protected byte[] xorBytes(byte[] s1, byte[] s2, int length)
	{
		byte[] result = new byte[length];
		for (int i = 0; i < length; i++)
		{
			result[i] = Convert.ToByte((int)s1[i] ^  (int)s2[i]);
		}
		return result;
	}

	protected byte[] base64UrlDecode(string base64Str)
	{
		base64Str = base64Str.Replace("-", "+");
		base64Str = base64Str.Replace("_", "/");
		int base64Len = base64Str.Length;
		int pad = 4 - (base64Len % 4);
		if (pad < 4) base64Str = base64Str.PadRight(base64Len + pad, '=');
		return Convert.FromBase64String(base64Str);
	}
}