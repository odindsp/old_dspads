#include <string.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <hiredis/hiredis.h>
#include "tracker.h"
#include "../common/json_util.h"

using namespace std;

extern pthread_mutex_t g_mutex_id, g_mutex_ur, g_mutex_group;
extern vector<string> cmds_id;
extern vector<GROUP_FREQUENCY_INFO> v_group_frequency;
extern vector<USER_INFO> v_user_frequency;
extern vector<bool> clk_redirect_status;	//点击重定向状态，执行302跳转后该状态置ture

extern uint64_t g_logid_flume, g_logid_kafka;
extern uint64_t g_logid_local;

extern double price_ceiling;

extern "C"
{
#include <dlfcn.h>
// #include <ccl/ccl.h>
}

//20170708: add for new tracker
string make_error_message(const string& bmsg, const string& emsg, const string& ecode_str)
{
	// 因为 sendLog() 有加时间戳的程序，所以不需要加时间错
	string result = "";
	if (emsg == "")
	{
		return result;
	}
	struct  timespec ts;
	uint64_t cm, hlcm;
	stringstream ss;
	//string timestamp = "";

	//getTime(&ts);
	//cm = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	//20170706: 改为发送明文，为了便于确认，测试完成后会改回来
	//hlcm = htonl64(cm);
	//ss << cm;
	//ss >> timestamp;
	//ss.clear();

	// bmsg(request base message) : like "bid:[%s], impid:[%s], adx:[%d], at:[%d], mtype[%c]"
	// emsg(error message) : like "Parse parameter is undefine"
	// 1) time=XXX|emsg=XXX
	// 2) time=XXX|bmsg=XXX|emsg=XXX
	// 3) time=XXX|emsg=XXX|ecode=XXX
	// 4) time=XXX|bmsg=XXX|emsg=XXX|ecode=XXX
	if (ecode_str == "")
	{
		// 302 和 app 激活日志
		if (bmsg == "")
		{
			// 1) 
			cout << "1)" << endl;
			//result = "time=" + string((char*)&hlcm, sizeof(uint64_t)) + "|" + emsg;
			//result = "time=" + timestamp + "|" + emsg;
			result = emsg + "\n";
		}
		else
		{
			// 2)
			cout << "2)" << endl;
			//result = "time=" + string((char*)&hlcm, sizeof(uint64_t)) + "|" + bmsg + "|" + emsg;
			//result = "time=" + timestamp + "|" + bmsg + "|" + emsg;
			result = bmsg + "|" + emsg + "\n";
		}
	}
	else
	{
		if (bmsg == "")
		{
			// 3)
			cout << "3)" << endl;
			//result = "time=" + string((char*)&hlcm, sizeof(uint64_t)) + "|" + emsg + "|" + ecode_str; 
			//result = "time=" + timestamp + "|" + emsg + "|" + ecode_str; 
			result = emsg + ecode_str + "\n"; 
		}
		else
		{
			// 4)
			cout << "4)" << endl;
			//result = "time=" + string((char*)&hlcm, sizeof(uint64_t)) + "|" + bmsg + "|" + emsg + "|" + ecode_str;
			//result = "time=" + timestamp + "|" + bmsg + "|" + emsg + "|" + ecode_str;
			result = bmsg + "|" + emsg + ecode_str + "\n";
		} 
	}
	cout << "flume_data[" << result << "]" << endl;
	result = "|" + result;
	return result;
}

//20170708: add for new tracker
string price_normalization(const double price, const int adx)
{
	string ret = "";
	double tmp_price = 0.00;
	char buf[1024];
	memset(buf, 0, 1024);
	if (price <= 0)
	{
		return ret;
	}
	if (adx == 14)
	{
		tmp_price = price / 100;
	}
	else if (adx == 3 | adx == 13 | adx == 24)
	{
		tmp_price = price * 100;
	}
	else if (adx == 16)
	{
		tmp_price = price / 10;
	}
	else
	{
		tmp_price = price;
	}
	sprintf(buf, "%.2lf", tmp_price);
	ret = buf;
	return ret;
}

//20170724: add for new tracker
double parse_notice_price(IN const char *requestparam_, IN DATATRANSFER *datatransfer_f_, IN MONITORCONTEXT *pctx_, IN const string& bid_str_, IN const string& impid_str_, IN const string& mapid_str_, IN const int i_adx_, IN const int i_at_, IN const string& mtype_str_, INOUT int& price_errcode_)
{
	char *price = NULL;
	double d_price = -1.00;
	
	string price_flume_err_data = "";
	string price_flume_base_data = "";
	
	price = getParamvalue(requestparam_, "price");
	if (price != NULL)
	{
		// 未加载该 adx 的价格解密动态库
		// only for test
		//if (i_adx_ > 0)
		if (i_adx_ > pctx_->adx_path.size())
		{
			price_errcode_ = E_TRACK_INVALID_PRICE_DECODE_HANDLE;
			price_flume_err_data = "emsg=Load handle failed";
			price_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + string(price);
			writeLog(g_logid_local, LOGERROR, "Load handle failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], price:[%s], on:[%s], in:[%d]", price_errcode_, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], price, __FUNCTION__, __LINE__);
			// 发送 flume, 返回 -1.00， E_TRACK_INVALID_PRICE_DECODE_HANDLE
			if (send_flume_log(price_flume_base_data, price_flume_err_data, price_errcode_, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Load handle failed, err:[0x%x], in:[%s], on:[%d]", price_errcode_, __FUNCTION__, __LINE__);
			}
		}
		else
		{
			cout << "before Decode, price:[" << price << "]" << endl;
			price_errcode_ = Decode(pctx_->adx_path[i_adx_ - 1], price, d_price);
			cout << "after Decode, price:[" << d_price << "], errcode:[" << price_errcode_ << "]" << endl;
			if (price_errcode_ != E_SUCCESS)
			{
				price_flume_err_data = "emsg=Decode price failed";
				price_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + string(price);
				writeLog(g_logid_local, LOGERROR, "Decode price failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], price:[%s], on:[%s], in:[%d]", price_errcode_, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], price, __FUNCTION__, __LINE__);
				// 发送 flume, 返回 -1.00，E_DL_DECODE_FAILED or E_DL_GET_FUNCTION_FAIL 
				if (send_flume_log(price_flume_base_data, price_flume_err_data, price_errcode_, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Decode price failed, err:[0x%x], in:[%s], on:[%d]", price_errcode_, __FUNCTION__, __LINE__);
				}
				d_price = -1.00;
			}
		}
	}
	else
	{
		// price == NULL
		price_errcode_ = E_TRACK_EMPTY_PRICE;
		price_flume_err_data = "emsg=Parse parameter is empty";
		price_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=";
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], price:[null]", price_errcode_, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0]);
		// 发送 flume, 返回 -1.00,  E_TRACK_EMPTY_PRICE
		if (send_flume_log(price_flume_base_data, price_flume_err_data, price_errcode_, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", price_errcode_, __FUNCTION__, __LINE__);
		}
	}
	IFFREENULL(price);
	return d_price;
} 

//20170726: add for new tracker
int parse_notice_deviceidinfo(IN const char *requestparam_, IN DATATRANSFER *datatransfer_f_, IN const string& bid_str_, IN const string& impid_str_, IN const string& mapid_str_, IN const int i_adx_, IN const int i_at_, IN const string& mtype_str_, INOUT string& deviceid_str_, INOUT string& deviceidtype_str_, INOUT vector<int>& rtb_errcode_list_)
{
	int result = 0;
	int device_errcode = E_SUCCESS;
	char* deviceid = NULL, *deviceidtype = NULL;
	
	string device_flume_base_data = "";
	string device_flume_err_data = "";
	
	// Check incoming parameters
	if (datatransfer_f_ == NULL)
	{
		device_errcode = E_TRACK_INVALID_DATATRANSFER_F;
		rtb_errcode_list_.push_back(device_errcode);
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", device_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
		goto exit;
	}
	if (requestparam_ == NULL)
	{
		device_errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		rtb_errcode_list_.push_back(device_errcode);
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		device_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", device_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(device_flume_base_data, device_flume_err_data, device_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	} 
	
	// parse device info	
	deviceid = getParamvalue(requestparam_, "deviceid");
	if (deviceid == NULL)
	{
		result = -1;
		device_errcode = E_TRACK_EMPTY_DEVICEID;
		rtb_errcode_list_.push_back(device_errcode);	
		device_flume_err_data = "emsg=Parse parameter is empty";
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|deviceid=";
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], deviceid:[null]", device_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0]);
		// 发送 flume, 继续
		if (send_flume_log(device_flume_base_data, device_flume_err_data, device_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
	}
	else if (strlen(deviceid) > PXENE_DEVICEID_LEN)
	{
		result = -1;
		deviceid_str_ = deviceid;
		device_errcode = E_TRACK_UNDEFINE_DEVICEID;
		rtb_errcode_list_.push_back(device_errcode);
		device_flume_err_data = "emsg=Parse parameter is undefine";
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|deviceid=" + deviceid_str_;
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefine, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], deviceid:[%s]", device_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], deviceid_str_.c_str());	
		// 发送 flume, 继续
		if (send_flume_log(device_flume_base_data, device_flume_err_data, device_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log faield, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
	}
	else
	{
		deviceid_str_ = deviceid;
	}
	deviceidtype = getParamvalue(requestparam_, "deviceidtype");
	if (deviceidtype == NULL)
	{
		result = -1;
		device_errcode = E_TRACK_EMPTY_DEVICEIDTYPE;
		rtb_errcode_list_.push_back(device_errcode);
		device_flume_err_data = "emsg=Parse parameter is empty";
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|deviceid=" + deviceid_str_ + "|deviceidtype=";
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefine, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], deviceid:[%s], deviceidtype:[null]", device_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], deviceid_str_.c_str());	
		// 发送 flume, 继续
		if (send_flume_log(device_flume_base_data, device_flume_err_data, device_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log faield, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
	}
	else if (strlen(deviceidtype) > PXENE_DEVICEIDTYPE_LEN)
	{
		result = -1;
		deviceidtype_str_ = deviceidtype;
		device_errcode = E_TRACK_UNDEFINE_DEVICEIDTYPE;
		rtb_errcode_list_.push_back(device_errcode);
		device_flume_err_data = "emsg=Parse parameter is undefine";
		device_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|deviceid=" + deviceid_str_ + "|deviceidtype=" + deviceidtype_str_;
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefine, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], deviceid:[%s], deviceidtype:[%s]", device_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], deviceid_str_.c_str(), deviceidtype_str_.c_str());	
		// 发送 flume, 继续
		if (send_flume_log(device_flume_base_data, device_flume_err_data, device_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log faield, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
	}
	else
	{
		deviceidtype_str_ = deviceidtype;
	}
	
exit:
	// free
	IFFREENULL(deviceid);
	IFFREENULL(deviceidtype);
	cout << "parse_notice_deviceidinfo finished..." << endl;
	
	return result;
}

// 20170724: send_flume_log（单个报错信息）
int send_flume_log(IN const string& base_data, IN const string& err_data, IN const int& errcode, IN DATATRANSFER *datatransfer_f_)
{
	int result = 0;
	int errcode_f = E_SUCCESS;
	stringstream ss;
	string errcode_str = "";
	string flume_data = "";

	if (datatransfer_f_ == NULL)
	{
		errcode_f = E_TRACK_INVALID_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], data:[%s], in:[%s], on:[%d]", errcode_f, base_data.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	else
	{
		// errcode _list is input parameter
		ss << std::hex << errcode;
		ss >> errcode_str;
		ss.clear();
		ss.str("");
		errcode_str = "|ecode=0x" + errcode_str;		
		flume_data = make_error_message(base_data, err_data, errcode_str);
		if (flume_data != "")
		{
			sendLog(datatransfer_f_, flume_data);
		}
		else
		{
			result = -1;
		}
	}
	return result;
}

// 20170725: send_flume_log（多个报错信息）
int send_flume_log(IN const string& base_data, IN const string& err_data, IN const vector<int>& errcode_list, IN DATATRANSFER *datatransfer_f_)
{
	int result = 0;
	int errcode_f = E_SUCCESS;
	stringstream ss;
	string tmp_errcode = "";
	string errcode_str = "";
	string flume_data = "";

	if (datatransfer_f_ == NULL)
	{
		errcode_f = E_TRACK_INVALID_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], data:[%s], in:[%s], on:[%d]", errcode_f, base_data.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	else
	{
		// errcode _list is input parameter
		if (errcode_list.size() > 0)
		{
			for (int i = 0; i < errcode_list.size(); i++)
			{
				ss << std::hex << errcode_list[i];
				ss >> tmp_errcode;
				ss.clear();
				ss.str("");
				tmp_errcode = "0x" + tmp_errcode;
				if (i == 0)
				{
					errcode_str = "|ecode=" + tmp_errcode;
				}
				else
				{
					errcode_str = errcode_str + "," + tmp_errcode;
				}
			}
		}
		else
		{
			errcode_str = "|ecode=0x0"; 
		}
		flume_data = make_error_message(base_data, err_data, errcode_str);
		if (flume_data != "")
		{
			sendLog(datatransfer_f_, flume_data);
		}
		else
		{
			result = -1;
		}
	}
	return result;
}

// 20170724: send_kafka_log
int send_kafka_log(IN const char *requestaddr_,IN const char *requestparam_, IN const string& win_kafka_base_data_, IN const bool& is_valid_, IN const vector<int>& rtb_errcode_list_, IN DATATRANSFER *datatransfer_k_)
{
	int result = 0;
	int errcode_k = E_SUCCESS;
	
	char *ip = NULL, *impt = NULL, *impm = NULL, *w = NULL, *h = NULL, *appid = NULL;
	char *nw = NULL, *os = NULL, *gp = NULL, *tp = NULL, *mb = NULL, *md = NULL, *op = NULL, *ds = NULL;
	string ip_str = "", impt_str = "", impm_str = "", w_str = "", h_str = "", appid_str = "";
	string nw_str = "", os_str = "", gp_str = "", tp_str = "", mb_str = "", md_str = "", op_str = "", ds_str = "";
	
	string is_valid_str = "";
	string kafka_err_data = "";
	string other_kafka_data = "";
	stringstream ss;
	string tmp_ecode_str = "";
	
	if (datatransfer_k_ == NULL)
	{
		errcode_k = E_TRACK_INVALID_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], data:[%s], in:[%s], on:[%d]", errcode_k, win_kafka_base_data_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	if (requestparam_ == NULL)
	{
		errcode_k = E_TRACK_INVALID_REQUEST_PARAMETERS;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], data:[%s], in:[%s], on:[%d]", errcode_k, win_kafka_base_data_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	if (is_valid_ == true)
	{
		is_valid_str = "|iv=1";
	}
	else
	{
		is_valid_str = "|iv=0";
	}

	// parse other parameters
	ip = getParamvalue(requestparam_, "ip");
	if (ip != NULL)
	{
		ip_str = ip;
		IFFREENULL(ip);
	}
	else {
		ip_str = requestaddr_;
	}
	impt = getParamvalue(requestparam_, "impt");
	if (impt != NULL)
	{
		impt_str = impt;
	}
	if (impt_str != "2")
	{
		// 广告展示机会类型为视频时，因为只有一种展示形式，impm 不用解析了
		impm = getParamvalue(requestparam_, "impm");
		if (impm != NULL)
		{
			impm_str = impm;
		}
	}
	w = getParamvalue(requestparam_, "w");
	if (w != NULL)
	{
		w_str = w;
	}
	h = getParamvalue(requestparam_, "h");
	if (h != NULL)
	{
		h_str = h;
	}
	appid = getParamvalue(requestparam_, "appid");
	if (appid != NULL)
	{
		appid_str = appid;
	}
	nw = getParamvalue(requestparam_, "nw");
	if (nw != NULL)
	{
		nw_str = nw;
	}
	os = getParamvalue(requestparam_, "os");
	if (os != NULL)
	{
		os_str = os;
	}
	gp = getParamvalue(requestparam_, "gp");
	if (gp != NULL)
	{
		gp_str = gp;
	}
	tp = getParamvalue(requestparam_, "tp");
	if (tp != NULL)
	{
		tp_str = tp;
	}
	mb = getParamvalue(requestparam_, "mb");
	if (mb != NULL)
	{
		mb_str = mb;
	}
	md = getParamvalue(requestparam_, "md");
	if (md != NULL)
	{
		md_str = md;
	}
	op = getParamvalue(requestparam_, "op");
	if (op != NULL)
	{
		op_str = op;
	}
	ds = getParamvalue(requestparam_, "ds");
	if (ds != NULL)
	{
		ds_str = ds;
	}
	other_kafka_data = "|ip=" + ip_str + "|impt=" + impt_str + "|impm=" + impm_str + "|w=" + w_str + "|h=" + h_str + "|appid=" + appid_str + "|nw=" + nw_str + "|os=" + os_str + "|gp=" + gp_str + "|tp=" + tp_str + "|mb=" + mb_str + "|md=" + md_str + "|op=" + op_str + "|ds=" + ds_str + is_valid_str;
	
	// make errcode string
	if (rtb_errcode_list_.size() == 0)
	{
		// E_SUCCESS
		kafka_err_data = "|ic=0x0";
	}
	else
	{
		kafka_err_data = "|ic=";
		for (int i = 0; i < rtb_errcode_list_.size(); i++)
		{
			ss << std::hex << rtb_errcode_list_[i];
			ss >> tmp_ecode_str;
			ss.clear();	
			ss.str("");
			if (i != rtb_errcode_list_.size() - 1)
			{
				kafka_err_data = kafka_err_data + "0x" + tmp_ecode_str + ",";
			}
			else
			{
				kafka_err_data = kafka_err_data + "0x" + tmp_ecode_str;
			}
			tmp_ecode_str = "";
		} // end of for
	}
	
	// send log
	sendLogToKafka(datatransfer_k_, win_kafka_base_data_ + other_kafka_data + kafka_err_data);

	// free
	IFFREENULL(impt);
	IFFREENULL(impm);
	IFFREENULL(w);
	IFFREENULL(h);
	IFFREENULL(appid);
	IFFREENULL(nw);
	IFFREENULL(os);
	IFFREENULL(gp);
	IFFREENULL(tp);
	IFFREENULL(mb);
	IFFREENULL(md);
	IFFREENULL(op);
	IFFREENULL(ds);

	return result;
}

void init_group_frequency_info(GROUP_FREQUENCY_INFO &gr_fre_info)
{
	gr_fre_info.adx = gr_fre_info.type = gr_fre_info.interval = 0;
}

void writeUserRestriction_impclk(uint64_t logid_local, USER_FREQUENCY_COUNT &ur, string &command)
{
	char *text = NULL;
	json_t *root = NULL, *label_gr = NULL, 
		*label_cr = NULL, *array_gr = NULL, 
		*array_cr = NULL, *entry_gr = NULL, 
		*entry_cr = NULL;
	map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::iterator user_fre;
	root = json_new_object();
	if (root == NULL)
		goto exit;

	jsonInsertString(root, "userid", ur.userid.c_str());
	label_gr = json_new_string("group_restriction");
	array_gr = json_new_array();

	user_fre  = ur.user_group_frequency_count.begin();
	while (user_fre != ur.user_group_frequency_count.end())
	{
		string groupid = user_fre->first;
		entry_gr = json_new_object();
		jsonInsertString(entry_gr, "groupid", groupid.c_str());

		map<string, USER_CREATIVE_FREQUENCY_COUNT>::iterator creative_fre = (user_fre->second).begin();
		label_cr = json_new_string("creative_restriction");
		array_cr = json_new_array();
		while (creative_fre != (user_fre->second).end())
		{
			string mapid = creative_fre->first;
			USER_CREATIVE_FREQUENCY_COUNT *cr = &creative_fre->second;
			entry_cr = json_new_object();
			jsonInsertString(entry_cr, "mapid", mapid.c_str());
			jsonInsertInt(entry_cr, "starttime", cr->starttime);
			jsonInsertInt(entry_cr, "imp", cr->imp);
			json_insert_child(array_cr, entry_cr);
			++creative_fre;
		}
		json_insert_child(label_cr, array_cr);
		json_insert_child(entry_gr, label_cr);
		json_insert_child(array_gr, entry_gr);
		++user_fre;
	}
	json_insert_child(label_gr, array_gr);
	json_insert_child(root, label_gr);
	json_tree_to_string(root, &text);

	if (root != NULL)
		json_free_value(&root);

	command = text;
	/*command = "SETEX dsp_user_frequency_" + ur.userid + " " + intToString(DAYSECOND - getCurrentTimeSecond()) + " " + string(text); 

	pthread_mutex_lock(&g_mutex_ur);
	cmds_ur.push_back(command);
	pthread_mutex_unlock(&g_mutex_ur);*/

exit:
	if (text != NULL)
		free(text);
}

int GroupRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, GROUP_FREQUENCY_INFO *gr_frequency_info)
{
	string value = "";
	char *text = NULL;
	json_t *root = NULL, *label = NULL;
	string set_command = "", enpire_command = "";
	int errcode = E_SUCCESS;

	int period_array[] = {15, 30, 60};
	const char *dst_array[] = {"error", "dstimp", "dstclk"};
	const char *sum_array[] = { "error", "sumimp", "sumclk" };
	const char *finish_array[] = { "error", "s_imp_t", "s_clk_t" };
	const char *total_array[] = { "error", "sumimp", "sumclk" };
	const char *interval_array[] = { "error", "imp", "clk" };

	uint8_t period = gr_frequency_info->interval;
	const char *mtype = gr_frequency_info->mtype.c_str();
	struct timespec ts1, ts2;
	long lasttime = 0;
	getTime(&ts1);

	char keys_name[MIN_LENGTH] = "", cur_interval[MIN_LENGTH] = "", 
		sum_imp_num[MIN_LENGTH] = "", sum_clk_num[MIN_LENGTH] = "",
		dst_impclk[MIN_LENGTH] = "", finish_impclk_time[MIN_LENGTH] = "",
		sum_impclk_str[MIN_LENGTH] = "", dst_impclk_url[MIN_LENGTH] = "";

	struct timeval timeofday;
	struct tm tm = {0};
	time_t timep;
	time(&timep);
	localtime_r(&timep, &tm);

	int url_type = 0;
	if (mtype[0] == 'i')   //20170713: changge m to i, for new tracker
	{
	   url_type = PXENE_DELIVER_TYPE_IMP;
	}
	else if(mtype[0] == 'c')
	{
	   url_type = PXENE_DELIVER_TYPE_CLICK;
	}
	else  // wrong url 
	{
		writeLog(logid_local, LOGDEBUG, "wrong mtype:%c", mtype[0]);
		goto resource_free;	
	}

	/* Create all of the keys */
	sprintf(keys_name, "dsp_group_frequency_%02d_%02d", gr_frequency_info->adx, tm.tm_mday);

	if (period != RESTRICTION_PERIOD_DAY)
	{
		sprintf(cur_interval, "%02d%02d-%02d", tm.tm_hour,
			period_array[gr_frequency_info->interval] * (tm.tm_min / period_array[gr_frequency_info->interval]),
			period_array[gr_frequency_info->interval]);
		sprintf(finish_impclk_time, "%s_%02d", finish_array[gr_frequency_info->type], tm.tm_hour);
		sprintf(dst_impclk, "%s-%02d", dst_array[gr_frequency_info->type], tm.tm_hour);
		
		sprintf(dst_impclk_url, "%s-%02d", dst_array[url_type], tm.tm_hour);

		sprintf(sum_imp_num, "sumimp-%02d", tm.tm_hour);
		sprintf(sum_clk_num, "sumclk-%02d", tm.tm_hour);
		sprintf(sum_impclk_str, "%s-%02d", sum_array[url_type] , tm.tm_hour);
	}
	else
	{
		sprintf(dst_impclk, "%s", dst_array[gr_frequency_info->type]);
	}

	errcode = hgetRedisValue(ctx_r, logid_local, string(keys_name), gr_frequency_info->groupid, value);
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		writeLog(logid_local, LOGDEBUG, "hget fre context invalid");
		errcode = E_REDIS_CONNECT_R_INVALID;  // read context invalid
		goto resource_free;
	}

	if (value == "")
	{
		int imp = 1, clk = 0;												//为该项目创建初始粒度的imp or clk
		json_t *entry_gr = NULL, *entry_interval = NULL;

		root = json_new_object();
		if (root == NULL)
		{
			writeLog(logid_local, LOGDEBUG, "json_new_object is null");
			goto resource_free;
		}
		if (mtype[0] == 'c')
		{
		   imp = 0;
		   clk = 1;
		}

		jsonInsertInt(root, total_array[1], imp);
		jsonInsertInt(root, total_array[2], clk);

		if (period == RESTRICTION_PERIOD_DAY)
			jsonInsertInt(root, dst_impclk, gr_frequency_info->frequency[0]);//if the restriction was per-day, then choose the first members
		else
		{
			entry_interval = json_new_string(cur_interval);
			entry_gr = json_new_object();

			jsonInsertInt(root, sum_imp_num, imp);
			jsonInsertInt(root, sum_clk_num, clk);

			jsonInsertInt(entry_gr, interval_array[1], imp);
			jsonInsertInt(entry_gr, interval_array[2], clk);

			jsonInsertInt(root, dst_impclk, gr_frequency_info->frequency[tm.tm_hour]);
			
			json_insert_child(entry_interval, entry_gr);
			json_insert_child(root, entry_interval);
		}

		json_tree_to_string(root, &text);

		errcode = hsetRedisValue(ctx_w, logid_local, keys_name, gr_frequency_info->groupid, string(text));
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}
		errcode = expireRedisValue(ctx_w, logid_local, keys_name, DAYSECOND*20 - getCurrentTimeSecond());
		if (errcode == E_REDIS_CONNECT_INVALID)
		{
			errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			goto resource_free;
		}
		goto resource_free;	
	}
	
	json_parse_document(&root, value.c_str());
	if (root == NULL)
	{
	    writeLog(logid_local, LOGDEBUG, "json_new_object is null");
		goto resource_free;	
	}

	//全天总数
	if (((label = json_find_first_label(root, total_array[url_type])) != NULL))
	{
		uint64_t sum_imp_clk = atoi(label->child->text);
		json_t *label_temp = NULL;

		char *new_sum_imp_clk = (char *)malloc(10 * sizeof(char));
		memset(new_sum_imp_clk, 0 , 10 * sizeof(char));
		sprintf(new_sum_imp_clk, "%d", sum_imp_clk + 1);

		free(label->child->text);
		label->child->text = new_sum_imp_clk;
	}
	else
	{
		jsonInsertInt(root, total_array[url_type], 1);
	}


	//Dst Number 每个小时的目标数，会根据上个时段的投放量进行滚动投放
	if ((label = json_find_first_label(root, dst_impclk)) == NULL)
	{
		json_t *label_temp = NULL;
		int64 dst_impclk_cur = 0;

		if (period == RESTRICTION_PERIOD_DAY)	//支持修改粒度
		{
			int i = tm.tm_hour;

			while (i >= 0)
			{
				int64 sum_impclk_num = 0;
				char sum_impclk_old[MIN_LENGTH] = "";

				sprintf(sum_impclk_old, "%s-%02d", sum_array[gr_frequency_info->type], i);

				if ((label_temp = json_find_first_label(root, sum_impclk_old)) != NULL)
					sum_impclk_num = atoi(label_temp->child->text);

				dst_impclk_cur += sum_impclk_num;

				i--;
			}

			dst_impclk_cur = gr_frequency_info->frequency[0] - dst_impclk_cur;

			if (gr_frequency_info->frequency[0] == 0)
				dst_impclk_cur = 0;
		}
		else
		{
			int i = tm.tm_hour - 1;

			while(i > 0)
			{
				int64 dst_impclk_num = 0, sum_impclk_num = 0;
				char dst_impclk_old[MIN_LENGTH] = "", sum_impclk_old[MIN_LENGTH] = "";

				sprintf(dst_impclk_old, "%s-%02d", dst_array[gr_frequency_info->type], i);
				sprintf(sum_impclk_old, "%s-%02d", sum_array[gr_frequency_info->type], i);

				if ((label_temp = json_find_first_label(root, dst_impclk_old)) != NULL)
					dst_impclk_num = atoi(label_temp->child->text);
				if ((label_temp = json_find_first_label(root, sum_impclk_old)) != NULL)
					sum_impclk_num = atoi(label_temp->child->text);

				if (dst_impclk_num)
				{
					dst_impclk_cur += dst_impclk_num - sum_impclk_num;	
					break;
				}

				i--;
			}

			dst_impclk_cur += gr_frequency_info->frequency[tm.tm_hour];
			
			if ((dst_impclk_cur <= 0) && (gr_frequency_info->frequency[tm.tm_hour] != 0))
				dst_impclk_cur = 1;
			
			if (gr_frequency_info->frequency[tm.tm_hour] == 0)
				dst_impclk_cur = 0;
		}
		jsonInsertInt(root, dst_impclk, dst_impclk_cur);//只记录type里面的展现或者点击的目前值
	}

	//Sum_imp_num and Sum_click_num 每小时的总数
	if (((label = json_find_first_label(root, sum_impclk_str)) != NULL))
	{
		uint64_t sum_imp = atoi(label->child->text);
		json_t *label_temp = NULL;

		char *new_sum_imp = (char *)malloc(10 * sizeof(char));
		memset(new_sum_imp, 0 , 10 * sizeof(char));
		sprintf(new_sum_imp, "%d", sum_imp + 1);

		free(label->child->text);
		label->child->text = new_sum_imp;

		++sum_imp;

		if (gr_frequency_info->type == url_type && ((label_temp = json_find_first_label(root, dst_impclk_url)) != NULL))
		{
			json_t *label_finish = NULL;
			uint64_t dst_cur_imp_num = 0;
			dst_cur_imp_num = atoi(label_temp->child->text);
			va_cout("%s,sum_impclk:%d,dst_cur_imp_num:%d", dst_impclk_url, sum_imp, dst_cur_imp_num);
			
			if ((dst_cur_imp_num != 0) && (dst_cur_imp_num <= sum_imp))
			{
				va_cout("finish_time:%s", finish_impclk_time);
				if ((label_finish = json_find_first_label(root, finish_impclk_time)) == NULL)
				{
					jsonInsertInt(root, finish_impclk_time, tm.tm_min);
					writeLog(logid_local, LOGDEBUG, "finish_time,%02d,%s,%s:%d", gr_frequency_info->adx, gr_frequency_info->groupid.c_str(), finish_impclk_time, tm.tm_min);
				}	
			}
		}
	}
	else
	{
		if (period != RESTRICTION_PERIOD_DAY)
		{
			jsonInsertInt(root, sum_impclk_str, 1);
		}
	}

	//修改某个interval中的imp or click
	if ((label = json_find_first_label(root, cur_interval)) != NULL)
	{
		json_t *temp_value = label->child;

		if (temp_value != NULL)
		{
			if (((label = json_find_first_label(temp_value, interval_array[url_type])) != NULL))
			{
				uint64_t num_imp = atoi(label->child->text);

				char *new_num_imp = (char *)malloc(10 * sizeof(char));
				memset(new_num_imp, 0 , 10 * sizeof(char));

				sprintf(new_num_imp, "%d", num_imp + 1);

				free(label->child->text);
				label->child->text = new_num_imp;
			}
		}
	}
	else//如果没有interval 那么就添加
	{
		int imp = 0, clk = 0;

		if (period != RESTRICTION_PERIOD_DAY)
		{
			json_t *entry_gr = NULL, *entry_interval = NULL;

			entry_interval = json_new_string(cur_interval);
			entry_gr = json_new_object();

			if(mtype[0] == 'i')  //20170713: changge m to i, for new tracker
				imp = imp + 1;
			else
				clk = clk + 1;

			jsonInsertInt(entry_gr, "imp", imp);
			jsonInsertInt(entry_gr, "clk", clk);

			json_insert_child(entry_interval, entry_gr);
			json_insert_child(root, entry_interval);
		}
	}

exit:
	json_tree_to_string(root, &text);

	errcode = hsetRedisValue(ctx_w, logid_local, keys_name, gr_frequency_info->groupid, string(text));
	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
	}

resource_free:
	if (root != NULL)
		json_free_value(&root);

	if (text != NULL)
		free(text);

	return errcode;
}

int check_group_frequency_capping(redisContext *ctx_r, redisContext *ctx_w, GROUP_FREQUENCY_INFO *gr_frequency_info)
{
	int errcode = E_SUCCESS;

	if (gr_frequency_info == NULL)
		goto exit;
	
	if(gr_frequency_info->type != 0)
	{
		errcode = GroupRestrictionCount(ctx_r, ctx_w, g_logid_local, gr_frequency_info);
	}

exit:
	return errcode;
}

void create_new_frequency(FREQUENCY_RESTRICTION_USER &fru, string groupid, string s_mapid, USER_FREQUENCY_COUNT &ur)
{
	USER_CREATIVE_FREQUENCY_COUNT cr_new;
	map<string, USER_CREATIVE_FREQUENCY_COUNT> creaive_temp;

	string mapid;
	if (fru.type == USER_FREQUENCY_CREATIVE_1 || fru.type == USER_FREQUENCY_CREATIVE_2)
		mapid = s_mapid;
	else
		mapid = groupid;

	cr_new.starttime = getStartTime(fru.period);
	cr_new.imp = 1;

	creaive_temp.insert(make_pair<string, USER_CREATIVE_FREQUENCY_COUNT>(mapid, cr_new));
	ur.user_group_frequency_count.insert(make_pair<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >(groupid, creaive_temp));
	return;
}

int UserRestrictionCount(redisContext *ctx_r, redisContext *ctx_w, uint64_t logid_local, USER_INFO *ur_frequency_info)
{
	struct timespec ts1;
	int errcode = E_SUCCESS;
	USER_FREQUENCY_COUNT ur;
	USER_CREATIVE_FREQUENCY_COUNT *cr = NULL;
	string command;
	int intervaltime = 0;
	int fre_capping = 0;

	// 用户频次
	getTime(&ts1);
	errcode = readUserRestriction(ctx_r, logid_local, string(ur_frequency_info->deviceid), ur);
	record_cost(logid_local, true, ur_frequency_info->deviceid.c_str(), "readUserRestriction after", ts1);

	if (errcode == E_REDIS_CONNECT_INVALID)
	{
		writeLog(logid_local, LOGDEBUG, "readUserRestriction  context invalid");
		errcode = E_REDIS_CONNECT_R_INVALID;  // read context invalid
		goto exit;
	}
	{
		map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> >::iterator group_user_fre = ur.user_group_frequency_count.find(ur_frequency_info->groupid);

		if (group_user_fre == ur.user_group_frequency_count.end())
		{
			create_new_frequency(ur_frequency_info->fru, ur_frequency_info->groupid, ur_frequency_info->mapid, ur);
			goto writeur;
		}
		else
		{
			map<string, USER_CREATIVE_FREQUENCY_COUNT> &creaive_temp = group_user_fre->second;
			cr = NULL;
			string str_mapid;
			if ((ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_2) || (ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_1))
			{
				str_mapid = ur_frequency_info->mapid;
				if(ur_frequency_info->fru.type == USER_FREQUENCY_CREATIVE_2)
				{
					map<string, uint32_t>::iterator m_capping_it = ur_frequency_info->fru.capping.find(string(str_mapid));
					if(m_capping_it == ur_frequency_info->fru.capping.end())
					{
						writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,frequencycapping not find", ur_frequency_info->deviceid.c_str(), ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
						goto exit;
					}
					else
					{
						fre_capping = m_capping_it->second;
					}
				}
				else
				{
					fre_capping = ur_frequency_info->fru.capping.begin()->second;
				}
			}
			else
			{
				str_mapid = ur_frequency_info->groupid;
				fre_capping = ur_frequency_info->fru.capping.begin()->second;
			}

			map<string, USER_CREATIVE_FREQUENCY_COUNT>::iterator p_creative = creaive_temp.find(str_mapid);
			if (p_creative != creaive_temp.end())
			{
				cr =  &(p_creative->second);
				if (cr != NULL)
				{
					if (cr->imp >= fre_capping)
					{
						if(getCurrentTimeSecond() - cr->starttime < PERIOD_TIME(ur_frequency_info->fru.period))
						{
							writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,has been shown enough time!",ur_frequency_info->deviceid.c_str(),ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
							goto exit;
						}

						cr->starttime = getStartTime(ur_frequency_info->fru.period);
						cr->imp = 1;
						goto writeur;
					}

					intervaltime = getCurrentTimeSecond() - cr->starttime - PERIOD_TIME(ur_frequency_info->fru.period);
					if (intervaltime >= 0)
					{
						cr->starttime = getStartTime(ur_frequency_info->fru.period);
						cr->imp = 1;
					}
					else
					{
						++cr->imp;
					}
				}
				else
				{
					writeLog(logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,mapid:%s,cr is NULL!",ur_frequency_info->deviceid.c_str(),ur_frequency_info->groupid.c_str(),ur_frequency_info->mapid.c_str());
					goto exit;
				}
			}
			else
			{
				USER_CREATIVE_FREQUENCY_COUNT creative_r;
				creative_r.starttime = getStartTime(ur_frequency_info->fru.period);
				creative_r.imp = 1;
				creaive_temp.insert(make_pair<string, USER_CREATIVE_FREQUENCY_COUNT>(str_mapid, creative_r));
				goto writeur;
			}
		}
writeur:
		writeUserRestriction_impclk(g_logid_local, ur, command);
		if (command != "")
		{
			errcode = setexRedisValue(ctx_w ,logid_local, "dsp_user_frequency_" + ur.userid,
				DAYSECOND - getCurrentTimeSecond(), command);
			if (errcode == E_REDIS_CONNECT_INVALID)
			{
				errcode = E_REDIS_CONNECT_W_INVALID;  // write context invalid
			}
		}
	}
exit:
	return errcode;
}

bool check_frequency_capping(uint64_t gctx, int index, REDIS_INFO *cache, uint8_t adx, char *mapid, char *deviceid, char *mtype, double price)
{
	USER_CREATIVE_FREQUENCY_COUNT *cr = NULL;
	bool ret = false;
	GROUP_FREQUENCY_INFO gr_frequency_info;
	MONITORCONTEXT *pctx = (MONITORCONTEXT *)gctx;

	if ((gctx == 0) || (cache == NULL) || (mapid == NULL) || (deviceid == NULL) || (mtype == NULL))
	{
		ret = false;
		writeLog(g_logid_local, LOGERROR, "gctx|cache|mapid|deviceid|mtype is invalid");
		goto exit;
	}
	{
		map<string, IMPCLKCREATIVEOBJECT>::iterator m_creative_it = cache->creativeinfo.find(string(mapid));

		if (m_creative_it == cache->creativeinfo.end())
		{
			ret = false;
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,can't find mapid:%s in creativeinfo",deviceid,mapid);
			goto exit;
		}

		IMPCLKCREATIVEOBJECT &creativeinfo = m_creative_it->second;
		/*if (price)
		{
		ADX_PRICE_INFO temp_price;
		temp_price.adx = adx;
		temp_price.groupid = creativeinfo.groupid;
		temp_price.mapid = mapid;
		temp_price.price = price;

		pthread_mutex_lock(&pctx->mutex_price_info);
		pctx->v_price_info.push_back(temp_price);
		pthread_mutex_unlock(&pctx->mutex_price_info);
		}*/

		map<string, FREQUENCYCAPPINGRESTRICTIONIMPCLK>::iterator m_frequency_it = cache->fc.find(creativeinfo.groupid);

		if (m_frequency_it == cache->fc.end())
		{
			ret = false;
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,can't find groupid:%s",deviceid,creativeinfo.groupid.c_str());
			goto exit;
		}

		FREQUENCYCAPPINGRESTRICTIONIMPCLK &frequencycapping = m_frequency_it->second;

		//将period/mtype/groupid插入vecter
		map<uint8_t, FREQUENCY_RESTRICTION_GROUP>::iterator m_frequency_group_it = frequencycapping.frg.find(adx);

		if (m_frequency_group_it == frequencycapping.frg.end())
		{
			writeLog(g_logid_local, LOGERROR, "deviceid:%s,groupid:%s,mapid:%s,adx:%d,not find adx frequency",deviceid,creativeinfo.groupid.c_str(),mapid,adx);
			ret = false;
			goto exit;
		}

		// 插入项目频次
		FREQUENCY_RESTRICTION_GROUP &group_frequency = m_frequency_group_it->second;

		init_group_frequency_info(gr_frequency_info);
		gr_frequency_info.interval = group_frequency.period;
		gr_frequency_info.groupid = creativeinfo.groupid;
		gr_frequency_info.mtype = string(mtype);
		gr_frequency_info.type = group_frequency.type;
		gr_frequency_info.frequency = group_frequency.frequency;
		gr_frequency_info.adx = adx;
		pthread_mutex_lock(&g_mutex_group);
		v_group_frequency.push_back(gr_frequency_info);
		pthread_mutex_unlock(&g_mutex_group);

		//从这里区分i和c
		if(mtype[0] == 'i') //20170713: changge m to i, for new tracker
		{
			if (frequencycapping.fru.type == USER_FREQUENCY_INFINITY)   // 不限制
			{
				ret = true;
				writeLog(g_logid_local, LOGDEBUG, "deviceid:%s,groupid:%s,doesn't limit user frequence", deviceid, creativeinfo.groupid.c_str());
				goto exit;
			}

			USER_INFO user_info;
			user_info.deviceid = deviceid;
			user_info.groupid = creativeinfo.groupid;
			user_info.mapid = mapid;
			user_info.fru = frequencycapping.fru;
			pthread_mutex_lock(&g_mutex_ur);
			v_user_frequency.push_back(user_info);
			pthread_mutex_unlock(&g_mutex_ur);
		}
	}
	ret = true;	
exit:
	return ret;
}

// 20170724: add for new tracker
int handing_not_rtb_process(void)
{
	// 留待以后开发
	int result = 0;
	return result;
}

// 20170724: add for new tracker
int handing_rtb_winnotice(IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_winnotice_, INOUT const bool& repeated_winnotice_, INOUT vector<int>& rtb_errcode_list_)
{
	int result = 0;
	int win_errcode = E_SUCCESS;
	int price_errcode = E_SUCCESS;
	
	stringstream ss;
	double d_win_price = 0.00;
	string win_price_str = "";
	
	string deviceid_str = "", deviceidtype_str = "";
	
	string win_flume_base_data = "";
	string win_flume_err_data = "";	
	string win_kafka_base_data = "";
	
	//5.3.1.0  Check incoming parameters
	if (datatransfer_f_ == NULL)
	{
		win_errcode = E_TRACK_INVALID_DATATRANSFER_F;
		rtb_errcode_list_.push_back(win_errcode);
		win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", win_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
		goto exit;
	}
	if (datatransfer_k_ == NULL)
	{
		win_errcode = E_TRACK_INVALID_DATATRANSFER_K;
		rtb_errcode_list_.push_back(win_errcode);
		win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		win_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", win_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (pctx_ == NULL)
	{
		win_errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		rtb_errcode_list_.push_back(win_errcode);
		win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		win_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", win_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (ctxinfo_ == NULL)
	{
		win_errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
		rtb_errcode_list_.push_back(win_errcode);
		win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		win_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", win_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}	
	if (requestparam_ == NULL)
	{
		win_errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		rtb_errcode_list_.push_back(win_errcode);
		win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		win_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", win_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	} 
	
	//5.3.1.1 winnotice parsed and save price
	d_win_price = parse_notice_price(requestparam_, datatransfer_f_, pctx_, bid_str_, impid_str_, mapid_str_, i_adx_, i_at_, mtype_str_, price_errcode);
	cout << "d_win_price:[" << d_win_price << "], price_errcode:[" << price_errcode << "]" << endl;  // test
	if (price_errcode != E_SUCCESS)
	{
		rtb_errcode_list_.push_back(price_errcode);
		result = -1;
	}
	else
	{
		if (d_win_price == 0)
		{
			result = -1;
			win_errcode = E_TRACK_FAILED_DECODE_PRICE;
			rtb_errcode_list_.push_back(win_errcode);
			win_flume_err_data = "emsg=Decode price is zero";
			win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=0";
			writeLog(g_logid_local, LOGERROR, "Decode price is zero, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[0], on:[%s], in:[%d]", win_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], __FUNCTION__, __LINE__);
			// 发送 flume, 继续，发送 kafka
			if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}
		
		// 价格单位和精度处理
		win_price_str = price_normalization(d_win_price, i_adx_);
		cout << "win_price_str:[" << win_price_str << "]" << endl; //test
		// only for test
		//win_price_str = "";
		if (win_price_str == "")
		{
			cout << "win_price_str == null" << endl;  //test
			result = -1;
			win_errcode = E_TRACK_FAILED_NORMALIZATION_PRICE;
			rtb_errcode_list_.push_back(win_errcode);
			win_flume_err_data = "emsg=Price normalization is failed";
			win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + doubleToString(d_win_price) + "|decode_price=";
			writeLog(g_logid_local, LOGERROR, "Price normalization is failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[%lf], decode_price:[null], on:[%s], in[%d]", win_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], d_win_price, __FUNCTION__, __LINE__);
			// 发送 flume，继续
			if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}
		
		// 如果价格超过上限，发送警告给 flume, 继续
		// 从配置文件获取上限警告参数
		d_win_price = atof(win_price_str.c_str());
		if (d_win_price > price_ceiling)
		{
			win_errcode = E_TRACK_OTHER_UPPER_LIMIT_ALERT;
			rtb_errcode_list_.push_back(win_errcode);
			win_flume_err_data = "emsg=Decode price is larger than upper limit";
			win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + doubleToString(d_win_price);
			writeLog(g_logid_local, LOGWARNING, "Decode price is larger than upper limit, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], price:[%lf], on:[%s], in[%d]", win_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], d_win_price, __FUNCTION__, __LINE__);
			// 发送 flume 继续
			if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}

		// 只有重复的赢价通知不需要缓存价格
		if (repeated_winnotice_ == false && win_price_str != "")
		{
			// 将价格写入价格缓存 redis 备查, key = dsp_id_price_(bid)_(impid) , value = d_price_str, REQUESTIDPRICELIFE = 3600s
			// 价格缓存 redis 在 common/init_context.cpp 的 global_initialize() 中创建
			// setexRedisVlaue() 在 common/util.cpp
			// 设置有效期为 3600s
			win_errcode = setexRedisValue(ctxinfo_->redis_price_master, g_logid_local,  "dsp_id_price_" + bid_str_ + "_" + impid_str_, REQUESTIDPRICELIFE, win_price_str);
			cout << "setexRedisValue dsp_id_price, key:[dsp_id_price_" << bid_str_ << "_" << impid_str_  << " " << REQUESTIDPRICELIFE << "], value:[" << win_price_str << "],  errcode:[" << win_errcode << "], price:[" << win_price_str << "]" << endl;			
			if (win_errcode != E_SUCCESS)
			{
				// 缓存赢价通知的价格失败
				rtb_errcode_list_.push_back(win_errcode);
				win_flume_err_data = "emsg=Set bid_price failed";
				win_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + win_price_str;
				writeLog(g_logid_local, LOGERROR, "Set bid_price failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[%s], d_price:[%lf]", win_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], win_price_str.c_str());
				// 发送 flume, 继续
				if (send_flume_log(win_flume_base_data, win_flume_err_data, win_errcode, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
				}
			}			
		}	
	} // end of else paser_notice_price success
		
	// 5.3.1.2 winnotice parse deviceinfo for kafka
	if (parse_notice_deviceidinfo(requestparam_, datatransfer_f_, bid_str_, impid_str_, mapid_str_, i_adx_, i_at_, mtype_str_, deviceid_str, deviceidtype_str, rtb_errcode_list_) < 0)
	{
		result = -1;
		writeLog(g_logid_local, LOGERROR, "Parse deviceid and deviceidtype failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_PARSE_DEVICEINFO, bid_str_.c_str(), __FUNCTION__, __LINE__);
	}

	// 5.3.1.3 send kafka
	cout << "winnotice sendkafka" << endl;  // test
	win_kafka_base_data = "mtype=" + mtype_str_ + "|bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|at=" + intToString(i_at_) + "|adx=" + intToString(i_adx_) + "|price=" + win_price_str + "|deviceid=" + deviceid_str + "|deviceidtype=" + deviceidtype_str;
	if (send_kafka_log(requestaddr_, requestparam_, win_kafka_base_data, is_valid_, rtb_errcode_list_, datatransfer_k_) < 0)
	{
		writeLog(g_logid_local, LOGERROR, "Send kafka log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_KAFKA, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	
exit:

	return result;
}

// 20170724: add for new tracker
int handing_rtb_impnotice(IN const int index_, IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_impnotice_, INOUT const bool& repeated_impnotice_, INOUT vector<int>& rtb_errcode_list_)
{
	int result = 0;
	int imp_errcode = E_SUCCESS;
	int price_errcode = E_SUCCESS;
	
	stringstream ss;
	double d_imp_price = 0.00;
	string imp_price_str = "";	
	
	char* deviceid = NULL;
	string deviceid_str = "", deviceidtype_str = "";
	char *mapid_p = const_cast<char*>(mapid_str_.c_str());  // for frequency count
	char *mtype_p = const_cast<char*>(mtype_str_.c_str());  // for frequency count
 	uint64_t redis_info_addr = 0;
	sem_t sem_id;
	uint64_t gctx_ = (uint64_t)pctx_;
	
	string imp_flume_base_data = "";
	string imp_flume_err_data = "";	
	string imp_kafka_base_data = "";
	
	//5.3.2.0  Check incoming parameters
	if (datatransfer_f_ == NULL)
	{
		imp_errcode = E_TRACK_INVALID_DATATRANSFER_F;
		rtb_errcode_list_.push_back(imp_errcode);
		imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", imp_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
		goto exit;
	}
	if (datatransfer_k_ == NULL)
	{
		imp_errcode = E_TRACK_INVALID_DATATRANSFER_K;
		rtb_errcode_list_.push_back(imp_errcode);
		imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		imp_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", imp_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (pctx_ == NULL)
	{
		imp_errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		rtb_errcode_list_.push_back(imp_errcode);
		imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		imp_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", imp_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (ctxinfo_ == NULL)
	{
		imp_errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
		rtb_errcode_list_.push_back(imp_errcode);
		imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		imp_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", imp_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}	
	if (requestparam_ == NULL)
	{
		imp_errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		rtb_errcode_list_.push_back(imp_errcode);
		imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		imp_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", imp_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	} 
	
	//5.3.2.1  impnotice parsed or get price
	d_imp_price = parse_notice_price(requestparam_, datatransfer_f_, pctx_, bid_str_, impid_str_, mapid_str_, i_adx_, i_at_, mtype_str_, price_errcode);
	cout << "d_imp_price:[" << d_imp_price << "]" << endl;  // test
	if (price_errcode != E_SUCCESS)
	{
		d_imp_price = 0.00;
		rtb_errcode_list_.push_back(price_errcode);
		if (price_errcode == E_TRACK_EMPTY_PRICE)
		{
			// price 字段为空，从价格缓存 redis 中读取该 bid 对应赢价通知缓存的价格
			cout << "impnotice price == NULL" << endl;
			imp_errcode = getRedisValue(ctxinfo_->redis_price_master, g_logid_local, "dsp_id_price_" + bid_str_ + "_" + impid_str_, imp_price_str);
			cout << "getRedisValue price errcode:[" << imp_errcode << "], imp_price_str:[" << imp_price_str << "]" << endl; 
			if (imp_errcode != E_SUCCESS)
			{
				// 获取缓存价格失败
				rtb_errcode_list_.push_back(imp_errcode); 
				imp_flume_err_data = "emsg=Get bid_price from redis failed";
				imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
				writeLog(g_logid_local, LOGERROR, "Get bid_price from redis failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[null]", imp_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0]);
				// 发送 flume, 继续
				if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
				}
				imp_price_str = "";
				result = -1;
			}
		}
	}
	else
	{
		if (d_imp_price == 0.00)
		{	
			result = -1;
			imp_errcode = E_TRACK_FAILED_DECODE_PRICE;
			rtb_errcode_list_.push_back(imp_errcode);
			imp_flume_err_data = "emsg=Decode price is zero";
			imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=0";
			writeLog(g_logid_local, LOGERROR, "Decode price is zero, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[0], on:[%s], in:[%d]", imp_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], __FUNCTION__, __LINE__);
			// 发送 flume, 继续，发送 kafka
			if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}
		
		// 价格单位和精度处理
		imp_price_str = price_normalization(d_imp_price, i_adx_);
		cout << "imp_price_str:[" << imp_price_str << "]" << endl; //test
		// only for test
		//imp_price_str = "";
		if (imp_price_str == "")
		{
			cout << "imp_price_str == null" << endl;  //test
			result = -1;
			imp_errcode = E_TRACK_FAILED_NORMALIZATION_PRICE;
			rtb_errcode_list_.push_back(imp_errcode);
			imp_flume_err_data = "emsg=Price normalization is failed";
			imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + doubleToString(d_imp_price) + "|decode_price=";
			writeLog(g_logid_local, LOGERROR, "Price normalization is failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], price:[%lf], decode_price:[null], on:[%s], in[%d]", imp_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], d_imp_price, __FUNCTION__, __LINE__);
			// 发送 flume，继续
			if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}	

		// 如果价格超过上限，发送警告给 flume, 继续
		// 从配置文件获取上限警告参数
		d_imp_price = atof(imp_price_str.c_str());
		if (d_imp_price > price_ceiling)
		{
			imp_errcode = E_TRACK_OTHER_UPPER_LIMIT_ALERT;
			rtb_errcode_list_.push_back(imp_errcode);
			imp_flume_err_data = "emsg=Decode price is larger than upper limit";
			imp_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|price=" + doubleToString(d_imp_price);
			writeLog(g_logid_local, LOGWARNING, "Decode price is larger than upper limit, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], price:[%lf], on:[%s], in[%d]", imp_errcode, bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], d_imp_price, __FUNCTION__, __LINE__);
			// 发送 flume 继续
			if (send_flume_log(imp_flume_base_data, imp_flume_err_data, imp_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
			}
		}
	} // end of else paser_notice_price success
	
	//5.3.2.2 impnotice parse deviceinfo for write imp frequency count
	// need deviceid
	if (parse_notice_deviceidinfo(requestparam_, datatransfer_f_, bid_str_, impid_str_, mapid_str_, i_adx_, i_at_, mtype_str_, deviceid_str, deviceidtype_str, rtb_errcode_list_) < 0)
	{
		result = -1;
		writeLog(g_logid_local, LOGERROR, "Parse deviceid and deviceidtype failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_PARSE_DEVICEINFO, bid_str_.c_str(), __FUNCTION__, __LINE__);
	}
	else
	{
		deviceid = const_cast<char*>(deviceid_str.c_str());
	}
	
	//5.3.2.3 write imp frequency count
	// 只有重复的不进行计数
	if (repeated_impnotice_ != true)
	{
		get_semaphore(pctx_->cache, &redis_info_addr, &sem_id);
		if (semaphore_release(sem_id) == true) //semaphore add 1
		{
			REDIS_INFO *redis_cashe = (REDIS_INFO *)redis_info_addr;
			writeLog(g_logid_local, LOGINFO, "Load redis_info_addr success");
			if (check_frequency_capping(gctx_, index_, redis_cashe, i_adx_, mapid_p, deviceid, mtype_p, d_imp_price) == true)
			{
				writeLog(g_logid_local, LOGINFO, "Bid:[%s], mtype:[%c], frequencycapping add succeed", bid_str_.c_str(), mtype_str_[0]);
			}
			else
			{
				imp_errcode = E_TRACK_FAILED_FREQUENCYCAPPING;
				rtb_errcode_list_.push_back(imp_errcode);
				writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], frequencycapping add failed", bid_str_.c_str(), mtype_str_[0]);
			}
			if (semaphore_wait(sem_id) == false)
			{	
				imp_errcode = E_TRACK_FAILED_SEMAPHORE_WAIT;
				rtb_errcode_list_.push_back(imp_errcode);
				writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], semaphore_wait failed", bid_str_.c_str(), mtype_str_[0]);
			}
		} // end of (semaphore_release(sem_id))
		else
		{
			imp_errcode = E_TRACK_FAILED_SEMAPHORE_RELEASE;
			rtb_errcode_list_.push_back(imp_errcode);
			writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], semaphore add 1 failed", bid_str_.c_str(), mtype_str_[0]);
		}		
	}
	
	//5.3.2.4 send kafka
	cout << "impnotice sendkafka" << endl;  // test
	imp_kafka_base_data = "mtype=" + mtype_str_ + "|bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|at=" + intToString(i_at_) + "|adx=" + intToString(i_adx_) + "|price=" + imp_price_str + "|deviceid=" + deviceid_str + "|deviceidtype=" + deviceidtype_str;
	if (send_kafka_log(requestaddr_, requestparam_, imp_kafka_base_data, is_valid_, rtb_errcode_list_, datatransfer_k_) < 0)
	{
		writeLog(g_logid_local, LOGERROR, "Send kafka log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_KAFKA, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	
exit:
	
	return result;
}

// 20170724: add for new tracker
int handing_rtb_clknotice(IN const int index_, IN FCGX_Request request_, IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_, IN const string& bid_str_, IN const string& impid_str_, IN const int i_at_, INOUT const bool& is_valid_, INOUT const bool& not_bid_clknotice_, INOUT const bool& repeated_clknotice_, INOUT vector<int>& rtb_errcode_list_)
{
	int result = 0;
	int clk_errcode = E_SUCCESS;
	
	char *curl = NULL, *gp = NULL, *tp = NULL, *mb = NULL, *op = NULL, *nw = NULL;
	string curl_content= "";    // for flume data
	string curl_str = "0";      // for kafka data
	string redirect_data = "";  // for flume data
	
	char *deviceid = NULL;
	string deviceid_str = "", deviceidtype_str = "";
	char *mapid_p = const_cast<char*>(mapid_str_.c_str());  // for frequency count
	char *mtype_p = const_cast<char*>(mtype_str_.c_str());  // for frequency count
 	uint64_t redis_info_addr = 0;
	sem_t sem_id;
	uint64_t gctx_ = (uint64_t)pctx_;
	
	string clk_flume_base_data = "";
	string clk_flume_err_data = "";	
	string clk_kafka_base_data = "";
	
	double d_price = 0.00; // 考虑成本累计，暂时不用

	//5.3.3.0  Check incoming parameters
	if (datatransfer_f_ == NULL)
	{
		clk_errcode = E_TRACK_INVALID_DATATRANSFER_F;
		rtb_errcode_list_.push_back(clk_errcode);
		clk_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", clk_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
		goto exit;
	}
	if (datatransfer_k_ == NULL)
	{
		clk_errcode = E_TRACK_INVALID_DATATRANSFER_K;
		rtb_errcode_list_.push_back(clk_errcode);
		clk_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		clk_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", clk_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(clk_flume_base_data, clk_flume_err_data, clk_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (pctx_ == NULL)
	{
		clk_errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		rtb_errcode_list_.push_back(clk_errcode);
		clk_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		clk_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", clk_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(clk_flume_base_data, clk_flume_err_data, clk_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	if (ctxinfo_ == NULL)
	{
		clk_errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
		rtb_errcode_list_.push_back(clk_errcode);
		clk_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
		clk_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", clk_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(clk_flume_base_data, clk_flume_err_data, clk_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}	
	if (requestparam_ == NULL)
	{
		clk_errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		rtb_errcode_list_.push_back(clk_errcode);
		clk_flume_base_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_;
	    clk_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], bid:[%s], in:[%s], on:[%d]", clk_errcode, bid_str_.c_str(), __FUNCTION__, __LINE__);
		if (send_flume_log(clk_flume_base_data, clk_flume_err_data, clk_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, bid_str_.c_str(), __FUNCTION__, __LINE__);
		}
		result = -1;
		goto exit;
	}
	
	//5.3.3.1 clknotice 302 redirect
	curl = getParamvalue(requestparam_, "curl");
	if (curl != NULL && strlen(curl) > 0)
	{
		curl_str = "1";
		static pthread_mutex_t redirect_mutex = PTHREAD_MUTEX_INITIALIZER;
		int decode_len = urldecode(curl, strlen(curl));
		string decode_curl = curl;
		gp = getParamvalue(requestparam_, "gp");
		tp = getParamvalue(requestparam_, "tp");
		mb = getParamvalue(requestparam_, "mb");
		op = getParamvalue(requestparam_, "op");
		nw = getParamvalue(requestparam_, "nw");
		if (gp != NULL)
		{
			replaceMacro(decode_curl, "#GP#", gp);
			if (tp != NULL)
			{
				replaceMacro(decode_curl, "#TP#", tp);
			}
			if (mb != NULL)
			{
				replaceMacro(decode_curl, "#MB#", mb);
			}
			if (op != NULL)
			{
				replaceMacro(decode_curl, "#OP#", op);
			}
			if (nw != NULL)
			{
				replaceMacro(decode_curl, "#NW#", nw);
			}
		}
		if (bid_str_ != "")
		{
			if (bid_str_.find("%") == string::npos)
			{
				if (decode_curl.find("admaster") != string::npos)
				{
					if (decode_curl.find(",h") != string::npos)
					{
						decode_curl.insert(decode_curl.find(",h")+ 2, bid_str_.c_str());
					}
				}
				replaceMacro(decode_curl, "#BID#", bid_str_.c_str());
			}
		}
		if (mapid_str_ != "")
		{
			replaceMacro(decode_curl, "#MAPID#", mapid_str_.c_str());
		}
		string destinationurl = "Location: " + string(decode_curl) + "\n\n";
		pthread_mutex_lock(&redirect_mutex);
		FCGX_PutStr(destinationurl.data(), destinationurl.size(), request_.out);
		pthread_mutex_unlock(&redirect_mutex);
		writeLog(g_logid_local, LOGINFO, "302 redirect finished, bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype:[%c], curl:[%s], in:[%s], on:[%d]", bid_str_.c_str(), impid_str_.c_str(), mapid_str_.c_str(), i_adx_, i_at_, mtype_str_[0], decode_curl.c_str(), __FUNCTION__, __LINE__);
		redirect_data = "bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at_) + "|mtype=" + mtype_str_ + "|curl=" + decode_curl;
		sendLog(datatransfer_f_, redirect_data);
		clk_redirect_status[index_] = true;
	} // end of (curl != NULL)
	
	//5.3.3.2 clknotice parse deviceinfo for write imp frequency count
	// need deviceid
	if (parse_notice_deviceidinfo(requestparam_, datatransfer_f_, bid_str_, impid_str_, mapid_str_, i_adx_, i_at_, mtype_str_, deviceid_str, deviceidtype_str, rtb_errcode_list_) < 0)
	{
		result = -1;
		writeLog(g_logid_local, LOGERROR, "Parse deviceid and deviceidtype failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_PARSE_DEVICEINFO, bid_str_.c_str(), __FUNCTION__, __LINE__);
	}
	else
	{
		deviceid = const_cast<char*>(deviceid_str.c_str());
	}
	
	//5.3.3.3 write clk frequency count
	// 只有重复的不进行计数
	if (repeated_clknotice_ != true)
	{
		get_semaphore(pctx_->cache, &redis_info_addr, &sem_id);
		if (semaphore_release(sem_id) == true) //semaphore add 1
		{
			REDIS_INFO *redis_cashe = (REDIS_INFO *)redis_info_addr;
			writeLog(g_logid_local, LOGINFO, "Load redis_info_addr success");
			if (check_frequency_capping(gctx_, index_, redis_cashe, i_adx_, mapid_p, deviceid, mtype_p, d_price) == true)
			{
				writeLog(g_logid_local, LOGINFO, "Bid:[%s], mtype:[%c], frequencycapping add succeed", bid_str_.c_str(), mtype_str_[0]);
			}
			else
			{
				clk_errcode = E_TRACK_FAILED_FREQUENCYCAPPING;
				rtb_errcode_list_.push_back(clk_errcode);
				writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], frequencycapping add failed", bid_str_.c_str(), mtype_str_[0]);
			}
			if (semaphore_wait(sem_id) == false)
			{	
				clk_errcode = E_TRACK_FAILED_SEMAPHORE_WAIT;
				rtb_errcode_list_.push_back(clk_errcode);
				writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], semaphore_wait failed", bid_str_.c_str(), mtype_str_[0]);
			}
		} // end of (semaphore_release(sem_id))
		else
		{
			clk_errcode = E_TRACK_FAILED_SEMAPHORE_RELEASE;
			rtb_errcode_list_.push_back(clk_errcode);
			writeLog(g_logid_local, LOGERROR, "Bid:[%s], mtype:[%c], semaphore add 1 failed", bid_str_.c_str(), mtype_str_[0]);
		}		
	}
	
	//5.3.3.4 send kafka
	cout << "clknotice sendkafka" << endl;  // test
	if (0 == strcmp(mapid_str_.c_str(), "06c5b927-f0cd-4a49-aec2-2089bae09701"))
	{
		result = -1;
		goto exit;
	}
	clk_kafka_base_data = "mtype=" + mtype_str_ + "|bid=" + bid_str_ + "|impid=" + impid_str_ + "|mapid=" + mapid_str_ + "|at=" + intToString(i_at_) + "|adx=" + intToString(i_adx_) + "|curl=" + curl_str + "|deviceid=" + deviceid_str + "|deviceidtype=" + deviceidtype_str;
	if (send_kafka_log(requestaddr_, requestparam_, clk_kafka_base_data, is_valid_, rtb_errcode_list_, datatransfer_k_) < 0)
	{
		writeLog(g_logid_local, LOGERROR, "Send kafka log failed, err:[0x%x], bid:[%s], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_KAFKA, bid_str_.c_str(), __FUNCTION__, __LINE__);
		result = -1;
	}
	
exit:
	IFFREENULL(curl);
	IFFREENULL(gp);
	IFFREENULL(tp);
	IFFREENULL(mb);
	IFFREENULL(op);
	IFFREENULL(nw);
	return result;
	
}

// 20170724: add for new tracker
int handing_rtb_process(IN FCGX_Request request_, IN const int index_, IN MONITORCONTEXT *pctx_, IN GETCONTEXTINFO *ctxinfo_, IN DATATRANSFER *datatransfer_f_, IN DATATRANSFER *datatransfer_k_, IN const char *requestaddr_, IN const char *requestparam_, IN const string& mtype_str_, IN const int i_adx_, IN const string& mapid_str_)
{
	int rtb_errcode = E_SUCCESS;
	vector<int> rtb_errcode_list;
	string rtb_flume_data = "";
	string rtb_flume_err_data = "";
	string rtb_flume_base_data = "";
	
	char *bid = NULL, *impid = NULL, *at = NULL;
	string bid_str = "", impid_str = "";
	int i_at = 0;
	
	int flag = 0;
	string str_flag = "";             // key: bid_impid  value: clk(点击) imp(展现) win(赢价) bid(竞价)
	bool valid_id_flag_redis = true;  // 默认去重 redis 是有效， 如果无效，则无法判断通知是否是重复的
	bool is_valid = false;            // 默认 win、imp、clk 通知无效
	bool repeated_winnotice = false;  // 默认 win 通知非重复
	bool repeated_impnotice = false;  // 默认 imp 通知非重复
	bool repeated_clknotice = false;  // 默认 clk 通知非重复	
	bool not_bid_winnotice = false;   // 默认赢价通知对应的 bid 参与了竞价（str_flag == ""）
	bool not_bid_impnotice = false;   // 默认展现通知对应的 bid 参与了竞价（str_flag == ""）
	bool not_bid_clknotice = false;   // 默认点击通知对应的 bid 参与了竞价（str_flag == ""）
	
	//char *advertiser = NULL, *appsid = NULL;     // 旧的追踪程序中有，去掉相关逻辑
	
	//5. rtb
	//5.0. Check incoming parameters
	if (datatransfer_f_ == NULL)
	{
		rtb_errcode = E_TRACK_INVALID_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (datatransfer_k_ == NULL)
	{
		rtb_errcode = E_TRACK_INVALID_DATATRANSFER_K;
		rtb_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	if (pctx_ == NULL)
	{
		rtb_errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		rtb_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	if (ctxinfo_ == NULL)
	{
		rtb_errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
		rtb_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}	
	if (requestaddr_ == NULL) 
	{
		rtb_errcode = E_TRACK_INVALID_REQUEST_ADDRESS;
		rtb_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	if (requestparam_ == NULL)
	{
		rtb_errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		rtb_flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	} 
	
	//5.1 rtb's basic parameters parsing and checking
	bid = getParamvalue(requestparam_, "bid");
	if (bid == NULL)
	{
		bid = (char*)calloc(2, 1);
		bid[0] = '1';
		rtb_errcode = E_TRACK_EMPTY_BID;
		rtb_flume_err_data = "emsg=Parse parameter is empty";
		rtb_flume_base_data = "remoteaddr=" + string(requestaddr_) + "|requestparam=" + string(requestparam_);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		//goto exit;
	}
	bid_str = bid;
	//google bid 单独解密处理
	if (16 == i_adx_) {
		char de_bid[512] = { 0 };
		memset(de_bid, 0, 512);
		memcpy(de_bid, bid, strlen(bid));
		urldecode(de_bid, strlen(bid));
		bid_str = de_bid;
	}
	impid = getParamvalue(requestparam_, "impid");
	if (impid == NULL)
	{
		impid = (char*)calloc(2,1);
		impid[0] = '1';
		rtb_errcode = E_TRACK_EMPTY_IMPID;
		rtb_flume_err_data = "emsg=Parse parameter is empty";
		rtb_flume_base_data = "remoteaddr=" + string(requestaddr_) + "|requestparam=" + string(requestparam_);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", rtb_errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		//goto exit;
	}
	impid_str = impid;
	//at value, 0 or null: bid in competition(竞价), 1: fixed price(定价)
	at = getParamvalue(requestparam_, "at");
	if (at == NULL)
	{
		// only for test
		cout << "at is null" << endl;
		i_at = 0; 
	}
	else
	{
		i_at = atoi(at);
		if (i_at != 0 && i_at != 1)
		{
			rtb_errcode = E_TRACK_UNDEFINE_AT;
			rtb_flume_err_data = "emsg=Parse parameter is undefine";
			rtb_flume_base_data = "remoteaddr=" + string(requestaddr_) + "|requestparam=" + string(requestparam_);
			writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], in:[%s], on:[%d]", rtb_errcode,__FUNCTION__, __LINE__);
			if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
			}
			i_at = 0;  // 物料回填错误，是别人的错误，发送 flume，继续
			rtb_errcode_list.push_back(rtb_errcode);
			rtb_errcode = E_SUCCESS;
			rtb_flume_err_data = "";
			rtb_flume_base_data = "";
		}
	}
	
	//5.2 de-duplication 根据 bid 去重
	// only for test
	//ctxinfo_->redis_id_slave = NULL;  // 1/2 两处中的第一处
	if (ctxinfo_->redis_id_slave == NULL)
	{
		// reconnect
		ctxinfo_->redis_id_slave = redisConnect(pctx_->slave_filter_id_ip.c_str(), pctx_->slave_filter_id_port);
		// only for test
		//ctxinfo_->redis_id_slave = NULL;  // 2/2 两处中的第二处
		if (ctxinfo_->redis_id_slave != NULL)
		{
			if (ctxinfo_->redis_id_slave->err)
			{
				rtb_errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
				rtb_errcode_list.push_back(rtb_errcode);
				rtb_flume_err_data = "emsg=Get id flag reconnect failed";
				rtb_flume_base_data = "msg=" + string(ctxinfo_->redis_id_slave->errstr) + "|bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
				writeLog(g_logid_local, LOGERROR, "Get id flag reconnect failed, err:[0x%x], msg:[%s], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], on:[%s], in:[%d]", rtb_errcode, ctxinfo_->redis_id_slave->errstr, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0], __FUNCTION__, __LINE__);
				redisFree(ctxinfo_->redis_id_slave);
				ctxinfo_->redis_id_slave = NULL;
				// 发送 flume, 继续
				if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
				}
				// 去重 redis 无效，导致无法判断通知是否是重复的, goto invalid redis
				valid_id_flag_redis = false;			     
			}
		}
		else
		{
			// ctxinfo_->redis_id_slave == NULL
			rtb_errcode = E_TRACK_EMPTY_GETCONTEXTINFO_HANDLE;
			rtb_errcode_list.push_back(rtb_errcode);
			rtb_flume_err_data = "emsg=Get id flag reconnect empty";
			rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
			writeLog(g_logid_local, LOGERROR, "Get id flag reconnect empty, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0]);
			redisFree(ctxinfo_->redis_id_slave);
			ctxinfo_->redis_id_slave = NULL;
			// 发送 flume, 继续
			if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
			}
			// 去重 redis 无效，导致无法判断通知是否是重复的, goto invalid redis
			valid_id_flag_redis = false;
		}
	}
	if (ctxinfo_->redis_id_slave != NULL)
	{
		// redis data: <bid_impid, flag>
		rtb_errcode = getRedisValue(ctxinfo_->redis_id_slave, g_logid_local, "dsp_id_flag_" + bid_str + "_" + impid_str, str_flag);
		cout << "str_flag:[" << str_flag << "], errcode:[" << rtb_errcode << "]" << endl;  // test
		// TODO: 确认 == E_REDIS_CONNECT_INVALID 和 != E_SUCCESS
		if (rtb_errcode == E_REDIS_CONNECT_INVALID)
		{
			rtb_errcode_list.push_back(rtb_errcode);
			rtb_flume_err_data = "emsg=Get id flag from redis failed";
			rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
			writeLog(g_logid_local, LOGERROR, "Get id flag from redis failed, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0]);
			redisFree(ctxinfo_->redis_id_slave);
			ctxinfo_->redis_id_slave = NULL;
			// 发送 flume, 继续
			if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
			}
			// 去重 redis 无效，导致无法判断通知是否是重复的, goto invalid redis
			valid_id_flag_redis = false;
		}
		//only for test
		//str_flag = "";
		if (str_flag != "")
		{
			string command = "";
			flag = atoi(str_flag.c_str()); 
			cout << "Get id flag from redis succeed, bid:[" << bid_str << "], flag:[" << flag << "]" << endl;  // test
			if (mtype_str_[0] == 'w')
			{
				if ((flag & DSP_FLAG_PRICE) == 0)
				{
					// TODO: 二期开发不做批量写操作了，来一个写一个，以避免去重失效的问题
					command = "INCRBY dsp_id_flag_" + bid_str + "_" + impid_str + " " + intToString(DSP_FLAG_PRICE);
					// 非重复的赢价通知，标记为有效，继续
					is_valid = true;
					pthread_mutex_lock(&g_mutex_id);
					cmds_id.push_back(command);
					pthread_mutex_unlock(&g_mutex_id);
				}
				else
				{
					// 重复的赢价通知，发送 flume, 继续，解密价格后，获取得到价格（发送 flume，发送 kafka 有效）， 获取不到价格（发送 flume ，退出） 
					rtb_errcode = E_TRACK_REPEATED_WINNOTICE;
					rtb_errcode_list.push_back(rtb_errcode);
					repeated_winnotice = true;
					rtb_flume_err_data = "emsg=Repeated winnotice";
					rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
					writeLog(g_logid_local, LOGERROR, "Repeated winnotice, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0]);
					// 发送 flume, 继续
					if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
					{
						writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
					}
				}
				cout << "goto winnotice" << endl; // test
				if (handing_rtb_winnotice(pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_winnotice, repeated_winnotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else if (mtype_str_[0] == 'i')
			{
				if ((flag & DSP_FLAG_SHOW) == 0)
				{
					// REQUESTIDFLAGLIFE 3600s
					int new_flag = flag | DSP_FLAG_SHOW;
					command = "SETEX dsp_id_flag_" + bid_str + "_" + impid_str + " " + intToString(REQUESTIDFLAGLIFE) + " " + intToString(new_flag);
					// 非重复的展现通知，标记为有效，继续
					is_valid = true;
					pthread_mutex_lock(&g_mutex_id);
					cmds_id.push_back(command);
					pthread_mutex_unlock(&g_mutex_id);
				}
				else
				{
					// 重复的展现通知，发送 flume, 继续，解密价格后，发送 flume， 发送 kafka（无效）
					rtb_errcode = E_TRACK_REPEATED_IMPNOTICE;
					rtb_errcode_list.push_back(rtb_errcode);
					repeated_impnotice = true;
					rtb_flume_err_data = "emsg=Repeated impnotice";
					rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
					writeLog(g_logid_local, LOGERROR, "Repeated impnotice, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0]);
					// 发送 flume, 继续
					if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
					{
						writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
					}
				}
				cout << "goto impnotice" << endl; // test
				if (handing_rtb_impnotice(index_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_impnotice, repeated_impnotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else if (mtype_str_[0] == 'c')
			{
				if (((flag & DSP_FLAG_CLICK) == 0 ) && (flag & DSP_FLAG_SHOW))
				{
					// REQUESTIDFLAGLIFE 3600s
					int new_flag = flag | DSP_FLAG_CLICK;
					command = "SETEX dsp_id_flag_" + bid_str + "_" + impid_str + " " + intToString(REQUESTIDFLAGLIFE) + " " + intToString(new_flag);
					// 非重复的赢价通知，标记为有效，继续
					is_valid = true;
					pthread_mutex_lock(&g_mutex_id);
					cmds_id.push_back(command);
					pthread_mutex_unlock(&g_mutex_id);
				}
				else
				{
					// 重复的点击通知，发送 flume，继续，发送 kafka（无效）
					rtb_errcode = E_TRACK_REPEATED_CLKNOTICE;
					rtb_errcode_list.push_back(rtb_errcode);
					repeated_clknotice = true;
					rtb_flume_err_data = "emsg=Repeated clknotice";
					rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
					writeLog(g_logid_local, LOGERROR, "Repeated clknotice, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0]);
					// 发送 flume, 继续
					if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
					{
						writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
					}
				}
				cout << "goto clknotice" << endl; //test
				if (handing_rtb_clknotice(index_, request_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_clknotice, repeated_clknotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else
			{
				rtb_errcode = E_TRACK_UNDEFINE_MTYPE;
				rtb_errcode_list.push_back(rtb_errcode);
				rtb_flume_err_data = "emsg=Parse parameter is undefined";
				rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
				writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], in:[%s], on:[%d]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0], __FUNCTION__, __LINE__);
				// 发送 flume, 退出
				if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
				}
				goto exit;
			}
		}
		else
		{
			// str_flag == "" 说明是通知没有参与竞价
			// bid_filter_response() 中向去重 redis 写入 dsp_id_flag_(bid)_(impid) crequest.imp[0].requestidlife  0001
			if (mtype_str_[0] == 'w')
			{
				// 赢价通知没有参与竞价，发送 flume，继续，解密价格后，发送 flume，发送 kafka
				rtb_errcode = E_TRACK_INVALID_WINNOTICE;
				rtb_errcode_list.push_back(rtb_errcode);
				not_bid_winnotice = true;
				cout << "goto winnotice" << endl; // test
				if (handing_rtb_winnotice(pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_winnotice, repeated_winnotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else if (mtype_str_[0] == 'i')
			{
				// 展现通知没有参与竞价，发送 flume，继续，解密价格后，发送 flume，发送 kafka
				rtb_errcode = E_TRACK_INVALID_IMPNOTICE;
				rtb_errcode_list.push_back(rtb_errcode);
				not_bid_impnotice = true;
				cout << "goto impnotice" << endl; // test
				if (handing_rtb_impnotice(index_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_impnotice, repeated_impnotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else if (mtype_str_[0] == 'c')
			{
				// 点击通知没有参与竞价，发送 flume，继续，解密价格后，发送 flume，发送 kafka
				rtb_errcode = E_TRACK_INVALID_CLKNOTICE;
				rtb_errcode_list.push_back(rtb_errcode);
				not_bid_clknotice = true;
				cout << "goto clknotice" << endl; //test
				if (handing_rtb_clknotice(index_, request_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_clknotice, repeated_clknotice, rtb_errcode_list) < 0)
				{
					rtb_errcode = rtb_errcode_list.back();
					goto exit;
				}
			}
			else
			{
				rtb_errcode = E_TRACK_UNDEFINE_MTYPE;
				rtb_errcode_list.push_back(rtb_errcode);
				rtb_flume_err_data = "emsg=Parse parameter is undefined";
				rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
				writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], in:[%s], on:[%d]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0], __FUNCTION__, __LINE__);
				// 发送 flume, 退出
				if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
				{
					writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
				}
				goto exit;
			}
		} // end of (str_flag == "")
	} // end of (ctxinfo->redis_id_slave != NULL)
	else
	{
		// goto invalid redis  >> ctxinfo_->redis_id_slave == NULL or ctxinfo_->redis_id_slave->err
		// valid_id_flag_redis == false;
		if (mtype_str_[0] == 'w')
		{
			// 赢价通知因去重 redis 无效，无法判断是否重复，继续，解密价格后，发送 flume，发送 kafka
			cout << "goto winnotice" << endl; // test
			if (handing_rtb_winnotice(pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_winnotice, repeated_winnotice, rtb_errcode_list) < 0)
			{
				rtb_errcode = rtb_errcode_list.back();
				goto exit;
			}
		}
		else if (mtype_str_[0] == 'i')
		{
			// 展现通知因去重 redis 无效，无法判断是否重复，继续，解密价格后，发送 flume，发送 kafka
			cout << "goto impnotice" << endl; // test
			if (handing_rtb_impnotice(index_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_impnotice, repeated_impnotice, rtb_errcode_list) < 0)
			{
				rtb_errcode = rtb_errcode_list.back();
				goto exit;
			}
		}
		else if (mtype_str_[0] == 'c')
		{
			// 点击通知因去重 redis 无效，无法判断是否重复，继续，发送 flume，发送 kafka
			cout << "goto clknotice" << endl; //test
			if (handing_rtb_clknotice(index_, request_, pctx_, ctxinfo_, datatransfer_f_, datatransfer_k_, requestaddr_, requestparam_, mtype_str_, i_adx_, mapid_str_, bid_str, impid_str, i_at, is_valid, not_bid_clknotice, repeated_clknotice, rtb_errcode_list) < 0)
			{
				rtb_errcode = rtb_errcode_list.back();
				goto exit;
			}
		}
		else
		{
			rtb_errcode = E_TRACK_UNDEFINE_MTYPE;
			rtb_errcode_list.push_back(rtb_errcode);
			rtb_flume_err_data = "emsg=Parse parameter is undefined";
			rtb_flume_base_data = "bid=" + bid_str + "|impid=" + impid_str + "|mapid=" + mapid_str_ + "|adx=" + intToString(i_adx_) + "|at=" + intToString(i_at) + "|mtype=" + mtype_str_;
			writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], bid:[%s], impid:[%s], mapid:[%s], adx:[%d], at:[%d], mtype[%c], in:[%s], on:[%d]", rtb_errcode, bid_str.c_str(), impid_str.c_str(), mapid_str_.c_str(), i_adx_, i_at, mtype_str_[0], __FUNCTION__, __LINE__);
			// 发送 flume, 退出
			if (send_flume_log(rtb_flume_base_data, rtb_flume_err_data, rtb_errcode, datatransfer_f_) < 0)
			{
				writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
			}
			goto exit;
		}
	}
	
exit:
	// 6. free
	IFFREENULL(bid);
	IFFREENULL(impid);
	IFFREENULL(at);
	return rtb_errcode;
}

int parseRequest(IN FCGX_Request request, IN  uint64_t gctx, IN int index, IN DATATRANSFER *datatransfer_f, IN DATATRANSFER *datatransfer_k, IN char *requestaddr, IN char *requestparam)
{
	int errcode = E_SUCCESS;
	
	char *mtype = NULL, *adx = NULL, *mapid = NULL, *nrtb = NULL;
	string mtype_str = "", mapid_str = "";
	int i_adx = 0;

	MONITORCONTEXT *pctx = NULL;
	GETCONTEXTINFO *ctxinfo = NULL;

	string redirect_data = "";
	string flume_data = "";
	string flume_err_data = "";
	string flume_base_data = "";

	//struct timespec ts1;  //TODO: for time consuming calculation
	//getTime(&ts1);  //TODO
	
	//1. Check incoming parameters
	if (gctx == 0)
	{
		errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit; 
	}
	if (datatransfer_f == NULL)
	{
		errcode = E_TRACK_INVALID_DATATRANSFER_F;
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		goto exit;
	}
	if (datatransfer_k == NULL)
	{
		errcode = E_TRACK_INVALID_DATATRANSFER_K;
		flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	if (requestaddr == NULL) 
	{
		errcode = E_TRACK_INVALID_REQUEST_ADDRESS;
		flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	if (requestparam == NULL)
	{
		errcode = E_TRACK_INVALID_REQUEST_PARAMETERS;
		flume_err_data = "emsg=Incoming parameter is invalid";
		writeLog(g_logid_local, LOGERROR, "Incoming parameter is invalid, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	} 

	//2. Load handle and check
	pctx = (MONITORCONTEXT *)gctx;
	if (pctx == NULL)
	{
		errcode = E_TRACK_INVALID_MONITORCONTEXT_HANDLE;
		flume_err_data = "emsg=Load handle failed";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Load handle failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	ctxinfo = &(pctx->redis_fds[index]);
	if (ctxinfo == NULL)
	{
		errcode = E_TRACK_INVALID_GETCONTEXTINFO_HANDLE;
		flume_err_data = "emsg=Load handle failed";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Load handle failed, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	// only for test
	cout << "1. and 2. ok" << endl;

	//3. parsed basic parameter and check
	mtype = getParamvalue(requestparam, "mtype");
	if (mtype == NULL)
	{
		errcode = E_TRACK_EMPTY_MTYPE;
		flume_err_data = "emsg=Parse parameter is empty";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	mtype_str = mtype;
	adx = getParamvalue(requestparam, "adx");
	if (adx == NULL)
	{
		errcode = E_TRACK_EMPTY_ADX;
		flume_err_data = "emsg=Parse parameter is empty";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	i_adx = atoi(adx);
	if (i_adx <= 0 || i_adx >= 255)
	{
		errcode = E_TRACK_UNDEFINE_ADX;
		flume_err_data = "emsg=Parse parameter is undefined";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	mapid = getParamvalue(requestparam, "mapid");
	if (mapid == NULL)
	{
		mapid = (char*)calloc(64, 1);
		strcpy(mapid, "06c5b927-f0cd-4a49-aec2-2089bae09701");
		errcode = E_TRACK_EMPTY_MAPID;
		flume_err_data = "emsg=Parse parameter is empty";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is empty, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		//goto exit;
	}
	mapid_str = mapid;
	if ((check_string_type(mapid, PXENE_MAPID_LEN, true, false, INT_RADIX_16)) == false)
	{
		errcode = E_TRACK_UNDEFINE_MAPID;
		flume_err_data = "emsg=Parse parameter is undefined";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
		goto exit;
	}
	nrtb = getParamvalue(requestparam, "nrtb");
	if (nrtb == NULL || nrtb[0] == 'n')
	{
		cout << "nrtb is 'n' or null, goto rtb." << endl;
		writeLog(g_logid_local, LOGINFO, "nrtb is 'n' or null, goto rtb");
		if ((errcode = handing_rtb_process(request, index, pctx, ctxinfo, datatransfer_f, datatransfer_k, requestaddr, requestparam, mtype_str, i_adx, mapid_str)) != E_SUCCESS)
		{
			writeLog(g_logid_local, LOGINFO, "Handing rtb process is failed...");
			
		}
		else
		{       
			writeLog(g_logid_local, LOGINFO, "Handing rtb process is succeed...");
		}
	}	
	else if (nrtb[0] == 'y')
	{
		cout << "nrtb is y, goto not rtb." << endl;
		writeLog(g_logid_local, LOGINFO, "nrtb is y, goto not rtb");
		if (handing_not_rtb_process() < 0)
		{
			writeLog(g_logid_local, LOGINFO, "Handing not rtb process is failed...");
		}
		else
		{
			writeLog(g_logid_local, LOGINFO, "Handing not  rtb process is succeed...");
		}
	}	
	else
	{
		errcode = E_TRACK_UNDEFINE_NRTB;
		flume_err_data = "emsg=Parse parameter is undefine";
		flume_base_data = "remoteaddr=" + string(requestaddr) + "|requestparam=" + string(requestparam);
		writeLog(g_logid_local, LOGERROR, "Parse parameter is undefined, err:[0x%x], in:[%s], on:[%d]", errcode, __FUNCTION__, __LINE__);
		if (send_flume_log(flume_base_data, flume_err_data, errcode, datatransfer_f) < 0)
		{
			writeLog(g_logid_local, LOGERROR, "Send flume log failed, err:[0x%x], in:[%s], on:[%d]", E_TRACK_FAILED_SEND_FLUME, __FUNCTION__, __LINE__);
		}
	}

exit:
	//6. free
	IFFREENULL(mtype);
	IFFREENULL(adx);
	IFFREENULL(mapid);
	IFFREENULL(nrtb);

	return errcode;
}
