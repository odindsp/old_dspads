#include <iostream>
#include <string>
#include "blowfish.h"
#include "base64.h"

using namespace std;

static int replace_str(std::string& str, const char * oldpart, const char * newpart)
{
	int	nReplaced		= 0;

	std::string::size_type nIdx			= 0;
	std::string::size_type nOldLen		= strlen(oldpart);
	if ( 0 == nOldLen )
		return 0;

	static const char ch = 0x00;
	std::string::size_type nNewLen		= strlen(newpart);
	const char* szRealNew	= newpart == 0 ? &ch : newpart;

	while ( (nIdx=str.find(oldpart, nIdx)) != std::string::npos )
	{
		str.replace(str.begin()+nIdx, str.begin()+nIdx+nOldLen, szRealNew);
		nReplaced++;
		nIdx += nNewLen;
	}
	return nReplaced;
}

bool g_encode_64(const string &str_src, string &str_des)
{
	int encode_len = ngx_base64_encoded_length(str_src.size());
	ngx_str_t dst;
	char *ds = new char[encode_len + 1];
	if (!ds)
	{
		return false;
	}
	memset(ds, 0x00, encode_len + 1);
	dst.data = (u_char*)ds;
	dst.len = encode_len;

	ngx_str_t src;
	src.data = (u_char*)str_src.c_str();
	src.len = str_src.size();

	ngx_encode_base64(&dst, &src);
	ds[encode_len] = '\0';
	str_des = ds;
	if (ds)
	{
		delete [] ds;
		ds = NULL;
	}
	return true;
}

bool g_decode_64(const string &str_src, string &str_des)
{

	string  str_tmp_code = base64_decode(str_src);
	int n_tmp_code_len = str_tmp_code.size();

	char *des = new char[2*n_tmp_code_len + 1];
	if (!des)
	{
		return false;
	}
	char *destmp = des;
	memset(des, 0x00, 2*n_tmp_code_len + 1);
	char *src = new char[2*n_tmp_code_len + 1];
	if (!src)
	{
		return false;
	}
	char *srctmp = src;
	memset(src, 0x00, 2*n_tmp_code_len + 1);
	strcpy(src, str_tmp_code.c_str());
	ngx_unescape_uri((unsigned char **)&destmp, (unsigned char **)(&srctmp), n_tmp_code_len, 0);
	str_des = des;

	if(src)
	{
		delete src;
		src = NULL;
	}

	if (des)
	{
		delete des;
		des = NULL;
	}
	return true;
}

void blowfish_decode(const string& token, const string& en_price, string& price)
{
		CBlowFish oBlowFish((unsigned char*)token.c_str(), token.size());

		string str_base64 = en_price;
		//base64_decode
		string str_blowfish("");

		g_decode_64(str_base64, str_blowfish);

		string::size_type lsize=((str_blowfish.size()+7)/8)*8;
		char* src_buf, *dst_buf;

		if (0 == str_blowfish.size())
		{
			return;
		}
		src_buf = new char[lsize];
		dst_buf = new char[lsize];
		memset(src_buf, 0x00, lsize);
		memset(dst_buf, 0x00, lsize);
		memcpy(src_buf, str_blowfish.c_str(),str_blowfish.size());
		oBlowFish.Decrypt((unsigned char*)src_buf, (unsigned char*)dst_buf, lsize, CBlowFish::CBC);
		price = string(dst_buf, lsize);

		delete src_buf;
		delete dst_buf;
}

void blowfish_encode(const string& token, const string& price, string& en_price)
{
		CBlowFish oBlowFish((unsigned char*)token.c_str(), token.size());
		string::size_type lsize=((price.size()+7)/8)*8;
		char* src_buf, *dst_buf;
		string str_blowfish("");

		if (0 == price.size())
		{
			return;
		}
		src_buf = new char[lsize];
		dst_buf = new char[lsize];
		memset(src_buf, 0x00, lsize);
		memset(dst_buf, 0x00, lsize);
		memcpy(src_buf, price.c_str(),price.size());

		oBlowFish.Encrypt((unsigned char*)src_buf, (unsigned char*)dst_buf, lsize, CBlowFish::CBC);
		str_blowfish = string(dst_buf, lsize);

		g_encode_64(str_blowfish, en_price);
		
		replace_str(en_price,"+","-");
		replace_str(en_price,"/","_");

		delete src_buf;
		delete dst_buf;
}



int main()
{
	string input("550123456000-550123456000-12312312312313123");
	string output("");
	string token("ADDDJJSSDDSSSJJK12345678901234567890123456789012345678901234567890-ADDDJJSSDDSSSJJK12345678901234567890123456789012345678901234567890");
	string decode("");
	cout<<"key="<<token<<endl;
	cout<<"sourcecode="<<input<<endl;
	blowfish_encode(token,input,output);
	cout<<"encode_result="<<output<<endl;
	cout<<"encode_len="<<output.length()<<endl;

	blowfish_decode(token,output,decode);
	cout<<"decode="<<decode<<endl;

	return 0;
}

