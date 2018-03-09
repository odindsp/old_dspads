#include "toutiao_adapter.h"
#include "toutiao_ssp_api.pb.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/url_endecode.h"
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

extern uint64_t g_logid_local,g_logid;
extern bool fullreqrecord;
// change toutiao request to common request
int parse_common_request(IN toutiao_ssp::api::BidRequest &bidrequest,OUT COM_REQUEST &commrequest)
{
    int err = E_SUCCESS;

    init_com_message_request(&commrequest);

    if(bidrequest.has_request_id())
    {
        commrequest.id = bidrequest.request_id();
    }
    else
    {
        err |= E_BAD_REQUEST;
        goto returnback;
    }

    for(uint32_t i = 0; i < bidrequest.adslots_size(); ++i)
    {
        COM_IMPRESSIONOBJECT impressionobject;
        init_com_impression_object(&impressionobject);

        toutiao_ssp::api::AdSlot adslot = bidrequest.adslots(i);
        if(adslot.has_id())
            impressionobject.id = adslot.id();
        else
            continue;
        impressionobject.type = IMP_BANNER;      // 现在赋值为1,banner

        if(adslot.has_bid_floor())
            impressionobject.bidfloor = (adslot.bid_floor() / 100.0);

        if(adslot.banner_size() > 0)
        {
            if(adslot.banner(0).has_width())
                impressionobject.banner.w = adslot.banner(0).width();
            else
                continue;
            if(adslot.banner(0).has_height())
                impressionobject.banner.h = adslot.banner(0).height();
            else
                continue;
            // 根据头条可接受的广告类型填充commonrequest里面拒绝的广告类型，需要根据文档进行类型转换
            // 需要改正
            bool bimagetype = true;  // 目前先处理图片广告，当可接受的广告没有图片时，插入拒绝的广告创意类型是图片
            for(uint32_t j = 0; j < adslot.ad_type_size(); ++j)
            {
                if(adslot.ad_type(j) == toutiao_ssp::api::DETAIL_BANNER)
                    bimagetype = false;
            }
            if (bimagetype)
                impressionobject.banner.btype.insert(ADTYPE_IMAGE);
        }
        else
        {
            continue;
        }
        commrequest.imp.push_back(impressionobject);
    }
    /*if(commrequest.imp.size() == 0)
       goto returnback;
     */

    // app
    if(bidrequest.has_app())
    {
        if(bidrequest.app().has_id())
            commrequest.app.id = bidrequest.app().id();
        if(bidrequest.app().has_name())
            commrequest.app.name = bidrequest.app().name();
        if(bidrequest.app().has_bundle())
            commrequest.app.bundle = bidrequest.app().bundle();
    }
    // device
    if(bidrequest.has_device())
    {
        if(bidrequest.device().has_ip())
            commrequest.device.ip = bidrequest.device().ip();
        if(bidrequest.device().has_ua())
            commrequest.device.ua = bidrequest.device().ua();
        if(bidrequest.device().has_make())
            commrequest.device.make = bidrequest.device().make();
        if(bidrequest.device().has_model())
            commrequest.device.model = bidrequest.device().model();
        if(bidrequest.device().has_osv())
            commrequest.device.osv = bidrequest.device().osv();
        if(bidrequest.device().has_geo())
        {
            if(bidrequest.device().geo().has_lat())
                commrequest.device.geo.lat = bidrequest.device().geo().lat();
            if(bidrequest.device().geo().has_lon())
                commrequest.device.geo.lon = bidrequest.device().geo().lon();
        }
        string os ;
        if(bidrequest.device().has_os())
            os = bidrequest.device().os();
        transform(os.begin(),os.end(),os.begin(),::tolower);     // 将os全都转换为小写

        commrequest.device.didtype = DID_UNKNOWN;
        commrequest.device.dpidtype = DPID_UNKNOWN;
        if(os.find("ios") != string::npos)   // ios
        {
            commrequest.device.os = OS_IOS;
            if(bidrequest.device().has_device_id())
            {
                commrequest.device.dpid = bidrequest.device().device_id();
                commrequest.device.dpidtype = DPID_IDFA;
            }
            else if(bidrequest.device().has_device_id_md5())   // 不存在明文时，判断是否存在MD5加密后的deviceid
            {
                commrequest.device.dpid = bidrequest.device().device_id_md5();
                commrequest.device.dpidtype = DPID_IDFA_MD5;
            }
        }
        else if(os.find("android") != string::npos)
        {
            commrequest.device.os = OS_ANDROID;
            if(bidrequest.device().has_device_id())
            {
                commrequest.device.did = bidrequest.device().device_id();
                commrequest.device.didtype = DID_IMEI;
            }
            else if(bidrequest.device().has_device_id_md5())
            {
                commrequest.device.did = bidrequest.device().device_id_md5();
                commrequest.device.didtype = DID_IMEI_MD5;
            }
        }
        else
        {
            commrequest.device.os = OS_UNKNOWN;
        }

        if(commrequest.device.didtype == DID_UNKNOWN && commrequest.device.dpidtype == DPID_UNKNOWN)
        {
            err |= E_DEVICEID_NOT_EXIST;
        }

        string make ;
        if(bidrequest.device().has_carrier())
            make = bidrequest.device().carrier();
        transform(make.begin(),make.end(),make.begin(),::tolower);
        if(make.find("mobile") != string::npos)
            commrequest.device.connectiontype = CARRIER_CHINAMOBILE;
        else if(make.find("unicom") != string::npos)
            commrequest.device.connectiontype = CARRIER_CHINAUNICOM;
        else if(make.find("telecom") != string::npos)
            commrequest.device.connectiontype = CARRIER_CHINATELECOM;
        else
            commrequest.device.connectiontype = CARRIER_UNKNOWN;
        if(bidrequest.device().has_device_type())
        {
            if(bidrequest.device().device_type() == toutiao_ssp::api::Device::PHONE)
                commrequest.device.devicetype = DEVICE_PHONE;
            else if(bidrequest.device().device_type() == toutiao_ssp::api::Device::TABLET)
                commrequest.device.devicetype = DEVICE_TABLET;
            else
                commrequest.device.devicetype = DEVICE_UNKNOWN;
        }

        if(bidrequest.device().has_connection_type())
        {
            if(bidrequest.device().connection_type() == toutiao_ssp::api::Device::Honeycomb)
                commrequest.device.connectiontype = CONNECTION_CELLULAR_3G;
            else if(bidrequest.device().connection_type() == toutiao_ssp::api::Device::WIFI)
                commrequest.device.connectiontype = CONNECTION_WIFI;
            else
                commrequest.device.connectiontype = CONNECTION_UNKNOWN;
        }
    }
    else
    {
        err |= E_DEVICE_NOT_EXIST;
        goto returnback;
    }

    if(commrequest.imp.size() == 0)
    {
        err |= E_IMP_SIZE_ZERO;
        goto returnback;
    }

    commrequest.cur = "CNY";

returnback:
    return err;
}

void replace_macro(OUT string &str,IN string bid,IN string dpid,IN string mapid,IN int deviceidtype)
{
    replaceMacro(str, "#BID#", bid.c_str());
    replaceMacro(str, "#DEVICEID#", dpid.c_str());
    replaceMacro(str,"#MAPID#",mapid.c_str());
    char dtype[100];
    sprintf(dtype,"%d",deviceidtype);
    replaceMacro(str,"#DEVICEIDTYPE#",dtype);
}

void print_bid_object(COM_BIDOBJECT &bidobject)
{
    cout<<"impid = "<<bidobject.impid<<endl;
    cout<<"price = "<<bidobject.price<<endl;
    cout<<"adid = "<<bidobject.adid<<endl;
    set<uint16_t>::iterator p = bidobject.cat.begin();
    for(p; p != bidobject.cat.end(); ++p)
    {
        cout<<"cat = "<<(int)*p<<endl;
    }
    cout<<"type = "<<bidobject.type<<endl;
    cout<<"ftype = "<<bidobject.ftype<<endl;
    cout<<"bundle = "<<bidobject.bundle<<endl;
    cout<<"adomain = "<<bidobject.adomain<<endl;
    cout<<"w = "<<bidobject.w<<endl;
    cout<<"h = "<<bidobject.h<<endl;
    cout<<"curl = "<<bidobject.curl<<endl;
    cout<<"monitorcode = "<<bidobject.monitorcode<<endl;
    for(uint32_t i = 0; i<bidobject.cmonitorurl.size(); ++i)
        cout<<"cmonitorurl = "<<bidobject.cmonitorurl[i]<<endl;
    for(uint32_t i =0; i<bidobject.imonitorurl.size(); ++i)
        cout<<"imonitoturl = "<<bidobject.imonitorurl[i]<<endl;
    cout<<"cid = "<<bidobject.cid<<endl;
    cout<<"crid = "<<bidobject.crid<<endl;
}
// change common response to toutiao response
void parse_common_response(IN COM_REQUEST &commrequest,IN COM_RESPONSE &commresponse,IN ADXTEMPLATE &adxtemplate,OUT toutiao_ssp::api::BidResponse &bidresponse)
{
#ifdef DEBUG
    cout<<"-------commonresponse----------"<<endl;
    cout<<"id = "<<commresponse.id<<endl;
    cout<<"bidid = "<<commresponse.bidid<<endl;
#endif

    for(uint32_t i = 0; i < commresponse.seatbid.size(); ++i)
    {
        toutiao_ssp::api::SeatBid *seatbid = bidresponse.add_seatbids();
        COM_SEATBIDOBJECT seatbidobject = commresponse.seatbid[i];
        for(uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
        {
            COM_BIDOBJECT bidobject = seatbidobject.bid[j];
#ifdef DEBUG
            print_bid_object(bidobject);
#endif
            toutiao_ssp::api::Bid *bid = seatbid->add_ads();
            bid->set_id(commresponse.bidid);
            bid->set_adslot_id(bidobject.impid);

            double price = bidobject.price * 100;
            bid->set_price((uint32_t)(price));

            bid->set_adid(0);                   // 现在都设置为0

            toutiao_ssp::api::MaterialMeta *creative = bid->mutable_creative();
            uint8_t type = bidobject.type;   // 需要转换
            //	creative->set_ad_type(type);   //待定

            string mapid ;      // 待定,需要填充
            replace_macro(adxtemplate.nurl,commresponse.id,commrequest.device.deviceid,mapid,commrequest.device.deviceidtype);
            (creative)->set_nurl(adxtemplate.nurl);

            //	if(type != DETAIL_BANNER)  // 设定title，待定
            //		bid->creative().set_title();

            (creative)->set_source("powerxene");

            toutiao_ssp::api::MaterialMeta_ImageMeta *image = (creative)->mutable_image_banner();
            // 设置图片的宽度和高度
            /*	for(uint32_t k =0;k < commrequest.imp.size();++k)
            {
                     if(commrequest.imp[i].id == bidobject.impid)
                     {
                image.set_width(commrequest.imp[i].banner.w);
                image.set_height(commrequest.imp[i].banner.h);
                break;
              }
            }
            */
            image->set_width(bidobject.w);
            image->set_height(bidobject.h);
            image->set_url(bidobject.sourceurl);

            toutiao_ssp::api::MaterialMeta_ExternalMeta  *external =  (creative)->mutable_external();

            // 需要宏替换
            replaceMacro(bidobject.curl,"#BID#",commrequest.id.c_str());
            external->set_url(bidobject.curl);

            if(type == toutiao_ssp::api::ANDROID_APP)
            {
                toutiao_ssp::api::MaterialMeta_AndroidApp *app = (creative)->mutable_android_app();
                app->set_app_name(bidobject.bundle);
                app->set_download_url(bidobject.curl);
            }
            else if(type == toutiao_ssp::api::IOS_APP)
            {
                toutiao_ssp::api::MaterialMeta_IosApp *app = creative->mutable_ios_app();
                app->set_app_name(bidobject.bundle);
                app->set_download_url(bidobject.curl);
            }

            for(uint32_t k = 0; k < adxtemplate.iurl.size(); ++k)
            {
                replace_macro(adxtemplate.iurl[k],commresponse.id,commrequest.device.deviceid,mapid,commrequest.device.deviceidtype);
                (creative)->add_show_url(adxtemplate.iurl[k]);
            }
            for(uint32_t k = 0; k < adxtemplate.cturl.size(); ++k)
            {
                string cturl = adxtemplate.cturl[k];
                if (bidobject.redirect)   // 重定向替换
                {
                    int len = 0;
                    string strcurl = bidobject.curl;
                    // replaceMacro(strcurl,"#BID#",commrequest.id.c_str());
                    char *curl = urlencode(strcurl.c_str(),strcurl.size(),&len);
                    cturl = cturl + "&curl=" + string(curl,len);
                    free(curl);
                }

                replace_macro(cturl,commresponse.id,commrequest.device.deviceid,mapid,commrequest.device.deviceidtype);
                (creative)->add_click_url(cturl);
            }
        }
        //   seatbid->set_seat(seatbidobject.seat);       // 新结构中将这个字段给去掉
    }
    return;
}

void print_comm_request(IN COM_REQUEST &commrequest)
{
    cout<<"id = "<<commrequest.id<<endl;
    cout<<"impression:"<<endl;
    for(uint32_t i = 0; i < commrequest.imp.size(); ++i)
    {
        if(commrequest.imp[i].type == IMP_BANNER) // bannner
        {
            cout<<"w = "<<commrequest.imp[i].banner.w<<endl;
            cout<<"h = "<<commrequest.imp[i].banner.h<<endl;
            set<uint8_t>::iterator p = commrequest.imp[i].banner.btype.begin();
            while(p != commrequest.imp[i].banner.btype.end())
            {
                cout<<"btype = "<<(int)*p<<endl;
                ++p;
            }

        }
        cout<<"bidfloor = "<<commrequest.imp[i].bidfloor<<endl;
    }
    cout<<"app:"<<endl;
    cout<<"id = "<<commrequest.app.id<<endl;
    cout<<"name = "<<commrequest.app.name<<endl;
    cout<<"device:"<<endl;
    cout<<"did = " <<commrequest.device.did<<endl;
    cout<<"didtype = "<<(int)commrequest.device.didtype<<endl;
    cout<<"dpid = " <<commrequest.device.dpid<<endl;
    cout<<"dpidtype = "<<(int)commrequest.device.dpidtype<<endl;
    cout<<"ua = "<<commrequest.device.ua<<endl;
    cout<<"ip = "<<commrequest.device.ip<<endl;
    cout<<"lat = "<<commrequest.device.geo.lat<<endl;
    cout<<"lon = "<<commrequest.device.geo.lon<<endl;
    cout<<"carrier = "<<(int)commrequest.device.carrier<<endl;
    cout<<"make = "<<commrequest.device.make<<endl;
    cout<<"model = "<<commrequest.device.model<<endl;
    cout<<"os = "<<(int)commrequest.device.os<<endl;
    cout<<"osv = "<<commrequest.device.osv<<endl;
    cout<<"connectiontype = "<<(int)commrequest.device.connectiontype<<endl;
    cout<<"devicetype = "<<(int)commrequest.device.devicetype<<endl;
    cout<<"cur = "<<commrequest.cur<<endl;
    set<uint16_t>::iterator p = commrequest.bcat.begin();
    while(p != commrequest.bcat.end())
    {
        cout<<"bcat = "<<(int)*p<<endl;
        ++p;
    }
}

static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price)
{
    if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
        return true;

    return false;
}

void toutiao_adapter(IN uint64_t ctx,IN uint8_t index,IN DATATRANSFER *datatransfer,IN RECVDATA *recvdata,OUT string &data)
{
    char *senddata = NULL;
    uint32_t sendlength = 0;
    toutiao_ssp::api::BidRequest bidrequest;
    COM_REQUEST commrequest;
    COM_RESPONSE commresponse;
    toutiao_ssp::api::BidResponse bidresponse;
    ADXTEMPLATE adxtemplate;
    bool logsent = true;
    int err = -1;
    bool parsesuccess = false;
    struct timespec ts1, ts2,ts3,ts4;
    long lasttime = 0,lasttime2 = 0;

    getTime(&ts3);
    if(recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
    {
        writeLog(g_logid_local,LOGERROR, "recv data is error!");
        goto release;
    }

    parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
    if (!parsesuccess)
    {
        writeLog(g_logid_local,LOGERROR, "Parse data from array failed!");
        va_cout("parse data from array failed!");
        goto release;
    }


    err = parse_common_request(bidrequest,commrequest);
    if(err == E_BAD_REQUEST)
    {
        va_cout("request format is error");
        writeLog(g_logid_local,LOGERROR, "request format is error");
        goto release;
    }
    else if(err != E_SUCCESS)
    {
        bidresponse.set_request_id(commrequest.id);
        va_cout("request is not suit for bid!");
        writeLog(g_logid_local,LOGERROR, "request is not suit for bid!");
        goto returnresponse;
    }

    writeLog(g_logid_local,LOGINFO,"toutiao request parse to common request success!");
    bidresponse.set_request_id(commrequest.id);

#ifdef DEBUG
    print_comm_request(commrequest);
#endif

    err = bid_filter_response(ctx, index, commrequest, NULL, filter_bid_price,NULL, &commresponse,&adxtemplate);
    if(err != E_SUCCESS)
        goto returnresponse;

    parse_common_response(commrequest,commresponse,adxtemplate,bidresponse);

    // writeLog(g_logid_local,LOGINFO,"parse common response to toutiao response success!");
    va_cout("parse common response to toutiao response success!");

returnresponse:
    sendlength = bidresponse.ByteSize();
    senddata = (char *)calloc(1,sizeof(char) * (sendlength + 1));
    if(senddata == NULL)
    {
        va_cout("allocate memory failure!");
        writeLog(g_logid_local,LOGERROR, "allocate memory failure!");
        goto release;
    }
    bidresponse.SerializeToArray(senddata,sendlength);
    // clear
    bidresponse.clear_seatbids();
    data = "Content-Length: " + intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);
#ifdef DEBUG
    cout<<"senddata="<<data<<endl;
#endif
    if (err == E_SUCCESS)
        writeLog(g_logid_local, LOGDEBUG,"response = "+string(senddata,sendlength));

    if(logsent)
    {
        sendLog(datatransfer,commrequest,ADX_TOUTIAO,err);
    }

    if (fullreqrecord)
        writeLog(g_logid,LOGINFO,string(recvdata->data,recvdata->length));
    else if (err == E_SUCCESS)
        writeLog(g_logid,LOGINFO,string(recvdata->data,recvdata->length));

    getTime(&ts4);
    lasttime2 = (ts4.tv_sec - ts3.tv_sec ) * 1000000 + (ts4.tv_nsec - ts3.tv_nsec) / 1000;
    writeLog(g_logid_local, LOGDEBUG, "%s, spent time:%d, err:0x%x", commrequest.id.c_str(), lasttime2, err);

    va_cout("%s, spent time:%d, err:0x%x", commrequest.id.c_str(), lasttime2, err);

release:
    if (recvdata->data != NULL)
    {
        free(recvdata->data);
        recvdata->data = NULL;
    }
    if (recvdata != NULL)
    {
        free(recvdata);
        recvdata = NULL;
    }
    if (senddata != NULL)
    {
        free(senddata);
        senddata = NULL;
    }
    return;
}
