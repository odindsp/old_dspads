#ifndef TYPE_H_
#define TYPE_H_

#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include "json.h"
using namespace std;

#define GLOBAL_PATH			"/etc/dspads_odin/"
#define GLOBAL_CONF_FILE	"dspads.conf"
#define VERSION_BID			"2.262"

enum FULL_ERRCODE_LEVEL
{ 
	FERR_LEVEL_CREATIVE = 0, 
	FERR_LEVEL_GROUP, 
	FERR_LEVEL_TOP
};

#define DAYSECOND	86400
#define REQUESTIDLIFE		1800
#define	IMPCLKLIFE			3600
#define LIFEGROUP			3600

// 20170716 : add for new tracker
#define	REQUESTIDFLAGLIFE	3600
#define	REQUESTIDPRICELIFE	3600

#define  CHINAREGIONCODE	1156000000
#define  DEFAULTLENGTH		12//0.0000186km

#define  BID_RTB		0
#define  BID_PMP		1

#define MIN_RATIO           -0.5
#define MAX_RATIO           0.2
#define DISCOUNT            0.1
#define INIT_FINISH_TIME    -1
#define INIT_COMP_RATIO          255
#define INIT_RATIO          (INIT_COMP_RATIO + 1)
#define TIME_INTERVAL       30

#define RESTRICTION_PERIOD_15MIN 	0
#define RESTRICTION_PERIOD_30MIN 	1
#define RESTRICTION_PERIOD_HOUR 	2
#define RESTRICTION_PERIOD_DAY  	3

#define GROUP_FREQUENCY_INFINITY  	0
#define GROUP_FREQUENCY_IMP  		1
#define GROUP_FREQUENCY_CLICK 		2

#define USER_FREQUENCY_INFINITY  	0
#define USER_FREQUENCY_PROJECT  	1
#define USER_FREQUENCY_CREATIVE_1 	2
#define USER_FREQUENCY_CREATIVE_2 	3

#define PERHOUR		3600
#define PERDAY		86400

#define PERIOD_TIME(period) (((period) == RESTRICTION_PERIOD_HOUR) ? PERHOUR : PERDAY)

#define MAX_REQUEST_LENGTH	10000
#define MAX_LENGTH 4096
#define MID_LENGTH 512
#define MIN_LENGTH 64

#define DSP_FLAG_BID				0x00000001
#define DSP_FLAG_PRICE				0x00000002
#define DSP_FLAG_SHOW				0x00000004
#define DSP_FLAG_CLICK				0x00000008
#define	DSP_FLAG_PV					0x00000010

#define		PXENE_BID_LEN						60 //bid最大长度
#define 	PXENE_MAPID_LEN						36 //mapid最大长度
#define 	PXENE_ADV_LEN						40 //adv广告主
#define		PXENE_ADX_LEN						3
#define		PXENE_DEVICEID_LEN					40
#define		PXENE_APPSID_LEN					10
#define		PXENE_DEVICEIDTYPE_LEN				5
#define		PXENE_MTYPE_LEN						1
#define		PXENE_ACTION_LEN					24

#define TARGET_ALLOW_ALL							0x00000000

//Device Target Flag
#define TARGET_DEVICE_REGIONCODE_WHITELIST			0x00000001
#define TARGET_DEVICE_REGIONCODE_BLACKLIST			0x00000002
#define TARGET_DEVICE_REGIONCODE_MASK				0x00000003
#define TARGET_DEVICE_REGIONCODE_DISABLE_ALL		TARGET_DEVICE_REGIONCODE_MASK

#define TARGET_DEVICE_CONNECTIONTYPE_WHITELIST		0x00000004
#define TARGET_DEVICE_CONNECTIONTYPE_BLACKLIST		0x00000008
#define TARGET_DEVICE_CONNECTIONTYPE_MASK			0x0000000c
#define TARGET_DEVICE_CONNECTIONTYPE_DISABLE_ALL	TARGET_DEVICE_CONNECTIONTYPE_MASK

#define TARGET_DEVICE_OS_WHITELIST					0x00000010
#define TARGET_DEVICE_OS_BLACKLIST					0x00000020
#define TARGET_DEVICE_OS_MASK						0x00000030
#define TARGET_DEVICE_OS_DISABLE_ALL				TARGET_DEVICE_OS_MASK

#define TARGET_DEVICE_CARRIER_WHITELIST				0x00000040
#define TARGET_DEVICE_CARRIER_BLACKLIST				0x00000080
#define TARGET_DEVICE_CARRIER_MASK					0x000000c0
#define TARGET_DEVICE_CARRIER_DISABLE_ALL			TARGET_DEVICE_CARRIER_MASK

#define TARGET_DEVICE_DEVICETYPE_WHITELIST			0x00000100
#define TARGET_DEVICE_DEVICETYPE_BLACKLIST			0x00000200
#define TARGET_DEVICE_DEVICETYPE_MASK				0x00000300
#define TARGET_DEVICE_DEVICETYPE_DISABLE_ALL		TARGET_DEVICE_DEVICETYPE_MASK

#define TARGET_DEVICE_MAKE_WHITELIST				0x00000400
#define TARGET_DEVICE_MAKE_BLACKLIST				0x00000800
#define TARGET_DEVICE_MAKE_MASK						0x00000c00
#define TARGET_DEVICE_MAKE_DISABLE_ALL				TARGET_DEVICE_MAKE_MASK

#define TARGET_DEVICE_DISABLE_ALL					(TARGET_DEVICE_REGIONCODE_DISABLE_ALL | TARGET_DEVICE_CONNECTIONTYPE_DISABLE_ALL | TARGET_DEVICE_OS_DISABLE_ALL | TARGET_DEVICE_CARRIER_DISABLE_ALL | TARGET_DEVICE_DEVICETYPE_DISABLE_ALL | TARGET_DEVICE_MAKE_MASK)

//Application Target Flag
#define TARGET_APP_ID_WHITELIST						0x00000001
#define TARGET_APP_ID_BLACKLIST						0x00000002
#define TARGET_APP_ID_MASK							0x00000003
#define TARGET_APP_ID_WBLIST						TARGET_APP_ID_MASK

#define TARGET_APP_CAT_WHITELIST					0x00000004
#define TARGET_APP_CAT_BLACKLIST					0x00000008
#define TARGET_APP_CAT_MASK							0x0000000c
#define TARGET_APP_CAT_WBLIST						TARGET_APP_CAT_MASK

#define TARGET_APP_MASK								(TARGET_APP_ID_WBLIST | TARGET_APP_CAT_WBLIST)
#define TARGET_APP_ID_CAT_WBLIST                    0x00000003						

//Scene Target Flag
#define TARGET_SCENE_WHITELIST						0x00000001
#define TARGET_SCENE_BLACKLIST						0x00000002
#define TARGET_SCENE_MASK							0x00000003

#define TARGET_SCENE_DISABLE_ALL					TARGET_SCENE_MASK

#define IN
#define OUT
#define INOUT

//MAKE default is 255
#define APPLE_MAKE	1
#define DEFAULT_MAKE		255

//ADX
#define ADX_UNKNOWN	0
#define ADX_TANX	1
#define ADX_AMAX	2
#define ADX_INMOBI	3
#define ADX_IWIFI	4
#define ADX_LETV	5
#define ADX_TOUTIAO	6
#define ADX_ZPLAY	7
#define ADX_BAIDU	8
#define ADX_SOHU	9
#define ADX_ADWALKER 10
#define ADX_IQIYI	11
#define ADX_GOYOO	12
#define ADX_IFLYTEK 13
#define ADX_ADVIEW	14
#define ADX_GOOGLE	16
#define ADX_BAIDU_VIDEOS 17
#define ADX_GDT 21
#define ADX_ADLEFEE	22
#define ADX_ATHOME	23
#define ADX_MOMO	24
#define ADX_HOMETV  25
#define ADX_PXENE 17
#define ADX_MAX     255

//IMPEXT.instl
#define INSTL_NO		0
#define INSTL_INSERT	1
#define INSTL_FULL		2

//IMPEXT.splash
#define SPLASH_NO	0
#define SPLASH_YES	1

//Imptype
#define IMP_UNKNOWN	0
#define IMP_BANNER	1
#define IMP_VIDEO	2
#define IMP_NATIVE	3

enum AssetImageType {
	ASSET_IMAGETYPE_UNKNOWN = 0, 
	ASSET_IMAGETYPE_ICON, 
	ASSET_IMAGETYPE_LOGO,
	ASSET_IMAGETYPE_MAIN
};

enum AssetDataType {
	ASSET_DATATYPE_UNKNOWN = 0, 
	ASSET_DATATYPE_SPONSORED, 
	ASSET_DATATYPE_DESC, 
	ASSET_DATATYPE_RATING,
	ASSET_DATATYPE_LIKES,
	ASSET_DATATYPE_DOWNLOADS,
	ASSET_DATATYPE_PRICE,
	ASSET_DATATYPE_SALEPRICE,
	ASSET_DATATYPE_PHONE,
	ASSET_DATATYPE_ADDRESS,
	ASSET_DATATYPE_DESC2,
	ASSET_DATATYPE_DISPLAYURL,
	ASSET_DATATYPE_CTATEXT
};

enum NativeAssetType {
	NATIVE_ASSETTYPE_UNKNOWN = 0, 
	NATIVE_ASSETTYPE_TITLE, 
	NATIVE_ASSETTYPE_IMAGE,
	NATIVE_ASSETTYPE_DATA 
};

enum NativeLayoutType {
	NATIVE_LAYOUTTYPE_UNKNOWN = 0, 
	NATIVE_LAYOUTTYPE_CONTENTWALL, 
	NATIVE_LAYOUTTYPE_APPWALL,
	NATIVE_LAYOUTTYPE_NEWSFEED, 
	NATIVE_LAYOUTTYPE_CHATLIST,
	NATIVE_LAYOUTTYPE_CAROUSEL,
	NATIVE_LAYOUTTYPE_CONTENTSTREAM, 
	NATIVE_LAYOUTTYPE_GRIDADJOININGCONTENT,
	NATIVE_LAYOUTTYPE_OPENSCREEN = 245
};

//didtype
#define DID_UNKNOWN			0x00
#define DID_IMEI			0x10
#define DID_IMEI_SHA1		0x11
#define DID_IMEI_MD5		0x12
#define DID_MAC				0x20
#define DID_MAC_SHA1		0x21
#define DID_MAC_MD5			0x22
#define DID_OTHER			0xFF

//dpidtype
#define DPID_UNKNOWN		0x00
#define DPID_ANDROIDID		0x60
#define DPID_ANDROIDID_SHA1	0x61
#define DPID_ANDROIDID_MD5	0x62
#define DPID_IDFA			0x70
#define DPID_IDFA_SHA1		0x71
#define DPID_IDFA_MD5		0x72
#define DPID_OTHER			0xFF

//did及dpid长度
#define MAX_LEN_REQUESTID	60
#define LEN_IMEI_SHORT		14
#define LEN_IMEI_LONG		15
#define LEN_MAC_SHORT		12	//60d819cddd40
#define LEN_MAC_LONG		17	//60:d8:19:cd:dd:40 or 60-d8-19-cd-dd-40
#define LEN_ANDROIDID		16
#define LEN_IDFA			36
#define LEN_SHA1			40
#define LEN_MD5				32

//Carrier 运营商

//#define CARRIER_UNKNOWN			0
//#define CARRIER_CHINAMOBILE		1
//#define CARRIER_CHINAUNICOM		2
//#define CARRIER_CHINATELECOM	3
enum CarrierType{
	CARRIER_UNKNOWN	= 0,
	CARRIER_CHINAMOBILE,
	CARRIER_CHINAUNICOM,
	CARRIER_CHINATELECOM
};

//Connectiontype 连接类型
//#define CONNECTION_UNKNOWN			0
//#define CONNECTION_WIFI				1
//#define CONNECTION_CELLULAR_UNKNOWN	2
//#define CONNECTION_CELLULAR_2G		3
//#define CONNECTION_CELLULAR_3G		4
//#define CONNECTION_CELLULAR_4G		5
//#define CONNECTION_CELLULAR_5G		6
enum ConnectType {
	CONNECTION_UNKNOWN = 0,
	CONNECTION_WIFI,
	CONNECTION_CELLULAR_UNKNOWN,
	CONNECTION_CELLULAR_2G,
	CONNECTION_CELLULAR_3G,
	CONNECTION_CELLULAR_4G,
	CONNECTION_CELLULAR_5G
};

//Devicetype 设备类型
#define DEVICE_UNKNOWN	0
#define DEVICE_PHONE	1
#define DEVICE_TABLET	2
#define DEVICE_TV		3

//Ctype 点击类型
#define CTYPE_UNKNOWN									0
#define CTYPE_OPEN_WEBPAGE								1
#define CTYPE_DOWNLOAD_APP								2
#define CTYPE_DOWNLOAD_APP_FROM_APPSTORE				3
#define CTYPE_SENDSHORTMSG								4
#define CTYPE_CALL										5
#define CTYPE_OPEN_WEBPAGE_WITH_WEBVIEW					6
#define CTYPE_OPEN_DETAIL_PAGE							7
#define CTYPE_DOWNLOAD_APP_INTERNALLY_AND_ACTIVE_URL	8

//Type 广告素材类型
#define ADTYPE_UNKNOWN			0
#define ADTYPE_TEXT_LINK		1
#define ADTYPE_IMAGE			2
#define ADTYPE_GIF_IMAGE		3
#define ADTYPE_HTML5			4
#define ADTYPE_MRAID_V2			5
#define ADTYPE_VIDEO			6
#define ADTYPE_FLASH			7
#define ADTYPE_DOWNLOAD_APP		8
#define ADTYPE_FEEDS			9

//Ftype 广告创意文件类型
#define ADFTYPE_UNKNOWN			0x0
#define ADFTYPE_IMAGE_PNG		0x11
#define ADFTYPE_IMAGE_JPG		0x12
#define ADFTYPE_IMAGE_GIF		0x13
#define ADFTYPE_VIDEO_FLV		0x21
#define ADFTYPE_VIDEO_MP4		0x22

//Attr 广告属性
#define ATTR_UNKNOWN							0
#define ATTR_AUDIO_AUTO							1
#define ATTR_AUDIO_USERINIT						2
#define ATTR_EXPANDABLE_AUTO					3
#define ATTR_EXPANDABLE_USERINIT_CLICK			4
#define ATTR_EXPANDABLE_USERINIT_ROLLOVER		5
#define ATTR_INBANNER_VIDEO_AUTO				6
#define ATTR_INBANNER_VIDEO_USERINIT			7
#define ATTR_POP								8
#define ATTR_PROVOCATIVE_OR_SUGGESTIVE_IMAGERY	9
#define ATTR_SHAKY_FLASHING_FLICKERING_EXTREME_ANIMATION_SMILEYS 10
#define ATTR_SURVEYS							11
#define ATTR_TEXT_ONLY							12
#define ATTR_USER_INTERACTIVE					13
#define ATTR_WINDOWSDIALOG_OR_ALERTSTYLE		14
#define ATTR_HAS_AUDIO_WITCH_BUTTON				15
#define ATTR_CAN_BE_SKIPPED						16

//OS 类型
//#define OS_UNKNOWN	0
//#define OS_IOS		1
//#define OS_ANDROID	2
//#define OS_WINDOWS	3
enum OsType {
	OS_UNKNOWN = 0,
	OS_IOS,
	OS_ANDROID,
	OS_WINDOWS
};

//Video 广告位类型
#define DISPLAY_UNKNOWN			0
#define DISPLAY_PAGE			1
#define DISPLAY_DRAW_CURTAIN	2
#define DISPLAY_FRONT_PASTER	3
#define DISPLAY_MIDDLE_PASTER	4
#define DISPLAY_BACK_PASTER		5
#define DISPLAY_PAUSE			6
#define DISPLAY_SUPERNATANT		7
#define DISPLAY_CORNER			8

//Video 格式类型
#define VIDEOFORMAT_UNKNOWN					0	
#define VIDEOFORMAT_X_FLV					ADFTYPE_VIDEO_FLV
#define VIDEOFORMAT_MP4						ADFTYPE_VIDEO_MP4
#define VIDEOFORMAT_X_MS_WMV				0x23
#define VIDEOFORMAT_APP_X_SHOCKWAVE_FLASH	0x24
#define VIDEO_VPAID_JS						0x25
#define VIDEO_YT_HOSTED						0x26

// adpos 广告位类型
#define ADPOS_UNKNOWN           0
#define ADPOS_FIRST				1
#define ADPOS_SECOND			2
#define ADPOS_OTHER				3
#define ADPOS_HEADER			4
#define ADPOS_FOOTER			5
#define ADPOS_SIDEBAR			6
#define ADPOS_FULL				7

// gender 用户性别
//#define GENDER_UNKNOWN			0
//#define GENDER_FEMALE			1
//#define GENDER_MALE				2
enum GenderType {
	GENDER_UNKNOWN = 0,
	GENDER_FEMALE,
	GENDER_MALE
};

#define INT_IGNORE				0
#define INT_RADIX_2				1
#define INT_RADIX_8				2
#define INT_RADIX_10			3
#define INT_RADIX_16			4
#define INT_RADIX_16_LOWER		5
#define INT_RADIX_16_UPPER		6

typedef int                 int32;
typedef long long           int64;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef unsigned char       uchar;

typedef struct _errcode_group{
	int errcode;
	map<string, int> m_err_creative;//mapid : creative's errcode
}ERRCODE_GROUP;

typedef struct _errcode_group_family{
	int errcode;
	map<string, ERRCODE_GROUP> m_err_group;//groupid : ERRCODE_GROUP
}ERRCODE_GROUP_FAMILY;

typedef struct _full_errcode{
	int errcode;
	int level;
	ERRCODE_GROUP_FAMILY err_group_family[2];//0: wlist, 1:ordinary
}FULL_ERRCODE;

//频次上限
struct _frequency_restriction_group
{
	uint8_t type;                                           //0:NO CONTROL, 1:IMP, 2:CLICK 
	uint8_t period;											//0:15MIN, 1:30MIN, 2:HOUR, 3:DAY
	vector<int> frequency;//size must be: 24
};
typedef struct _frequency_restriction_group FREQUENCY_RESTRICTION_GROUP;

struct _frequency_restriction_user
{
	uint8_t type;                                        
	uint8_t period;                                         //2:HOUR, 3:DAY
	map<string, uint32_t> capping;
};
typedef struct _frequency_restriction_user FREQUENCY_RESTRICTION_USER;

struct _frequencycapping_restriction
{
	string groupid;
	map<uint8_t, FREQUENCY_RESTRICTION_GROUP> frg;
	FREQUENCY_RESTRICTION_USER fru;
};
typedef struct _frequencycapping_restriction FREQUENCYCAPPINGRESTRICTION;
//频次上限

struct _frequencycapping_restriction_impclk
{
	string groupid;
	uint32_t lastrefreshtime;
	map<uint8_t, FREQUENCY_RESTRICTION_GROUP> frg;
	FREQUENCY_RESTRICTION_USER fru;
};
typedef struct _frequencycapping_restriction_impclk FREQUENCYCAPPINGRESTRICTIONIMPCLK;

//user频次计数
struct _user_creative_frequency_count
{
//	string mapid;
	uint32_t starttime;
	uint32_t imp;
};
typedef struct _user_creative_frequency_count USER_CREATIVE_FREQUENCY_COUNT;

//struct _user_group_frequency_count
//{
//	string groupid;
//	vector<USER_CREATIVE_FREQUENCY_COUNT> user_creative_frequency_count;
//};
//typedef struct _user_group_frequency_count USER_GROUP_FREQUENCY_COUNT;

struct _user_frequency_count
{
	string userid;
//	vector<USER_GROUP_FREQUENCY_COUNT> user_group_frequency_count;
	map<string, map<string, USER_CREATIVE_FREQUENCY_COUNT> > user_group_frequency_count;
};
typedef struct _user_frequency_count USER_FREQUENCY_COUNT;
//user频次计数

//group频次计数
struct _group_interval_frequency_count
{
	uint32_t imp;
	uint32_t click;
};
typedef struct _group_interval_frequency_count GROUP_INTERVAL_FREQUENCY_COUNT;

struct _group_frequency_count
{
	string groupid;
	map<string, GROUP_INTERVAL_FREQUENCY_COUNT> group_interval_frequency_count;
};
typedef struct _group_frequency_count GROUP_FREQUENCY_COUNT;
//group频次计数

//定向控制

//Device
struct _device_target
{
	uint32_t flag;
	set<uint32_t> regioncode;
	set<uint8_t> connectiontype;
	set<uint8_t> os;
	set<uint8_t> carrier;
	set<uint8_t> devicetype;
	set<uint16_t> make;
};
typedef struct _device_target DEVICE_TARGET;
//Device

//Application 
struct _app_id_wblist
{
	uint32_t flag;//forbid: 0011 or 1100
	set<string> wlist;
	set<string> blist;
};
typedef struct _app_id_wblist APP_ID_WBLIST;

struct _app_cat_wblist
{
	uint32_t flag;
	set<uint32_t> wlist;
	set<uint32_t> blist;
};
typedef struct _app_cat_wblist APP_CAT_WBLIST;

struct _app_target
{
//	uint32_t flag;//forbid: 0011 or 1100
	APP_ID_WBLIST id;//removed key: adx
	APP_CAT_WBLIST cat;
};
typedef struct _app_target APP_TARGET;
//Application 

struct _loc_object
{
	double lat;
	double lon;
};
typedef struct _loc_object LOC_LIST;

struct _scene_target
{
	uint32_t flag;//forbid: 11
	int length;
	set<string> loc_set;
	//map<string, LOC_LIST> loc_map;//removed key: adx
};
typedef struct _scene_target SCENE_TARGET;
//Scenario 

struct _group_target
{
	string groupid;
	DEVICE_TARGET device_target;
	APP_TARGET app_target;
	SCENE_TARGET scene_target;
};
typedef struct _group_target GROUP_TARGET;
//定向控制

struct _adx_target
{
	uint8_t adx;
	DEVICE_TARGET device_target;
	APP_TARGET app_target;
};
typedef struct _adx_target ADX_TARGET;
//定向控制

struct wb_list
{
	string groupid;
	string relationid;
	double ratio;
	double mprice;
	set<uint8_t> whitelist;
	set<uint8_t> blacklist;
};
typedef struct wb_list WBLIST;

struct creative_object_ext
{
	string id;
	string ext;
};
typedef creative_object_ext CREATIVE_EXT;

struct icon_object
{
	uint8_t ftype;
	uint16_t w;
	uint16_t h;
	string sourceurl;
};
typedef icon_object ICON_OBJECT;

struct imgs_object
{
	uint8_t ftype;
	uint16_t w;
	uint16_t h;
	string sourceurl;
};
typedef imgs_object IMAG_OBJECT;

struct creative_object
{
	string mapid;
	string groupid;
	string groupid_type;
	double price;
	string adid;
	uint8_t type;
	uint16_t ftype;
	uint8_t ctype;
	string bundle;
	string apkname;
	uint16_t w;
	uint16_t h;
	string curl;
	string landingurl;
	string securecurl;
	string monitorcode;
	vector<string> cmonitorurl;
	vector<string> imonitorurl;
	string sourceurl;
	string deeplink;
	string cid;
	string crid;
	set<uint8_t> attr;
	set<uint8_t> badx;
	ICON_OBJECT icon;
	vector<IMAG_OBJECT> imgs;
	string title;
	string description;
	uint8_t rating;
	string ctatext;
	int advcat;
	string dealid;
	CREATIVE_EXT ext;
};
typedef struct creative_object CREATIVEOBJECT;

struct impclk_creative_object
{
	string groupid;
	uint32_t lastrefreshtime;
};
typedef impclk_creative_object IMPCLKCREATIVEOBJECT;

typedef struct adms
{
	uint8_t os;
	uint8_t type;
	string adm;
}ADMS;

typedef struct adx_template
{
	uint8_t adx;
	double ratio;
	vector<string> iurl;
	vector<string> cturl;
	string aurl;
	string nurl;
	vector<ADMS> adms;
}ADXTEMPLATE;

struct group_info_ext
{
	string advid;
};
typedef group_info_ext GROUPINFO_EXT;

typedef struct groupid_info
{
	set<uint32_t> cat;
	int advcat;
	int is_secure;
	string adomain;
	set<uint8_t> adx;
	uint8_t at;
	string dealid;
	uint8_t redirect;
	uint8_t effectmonitor;
	GROUPINFO_EXT ext;
}GROUPINFO;

struct _group_smart_bid_info
{
	uint8_t flag;
	double minratio;
	double maxratio;
	int interval;
};

typedef _group_smart_bid_info GROUP_SMART_BID_INFO;

struct _log_conrol_info
{
	int ferr_level;
	int is_print_time;
};
typedef _log_conrol_info LOG_CONTROL_INFO;

struct _user_clkadvcat_info
{
	string userid;
	set<int> clkadvcat;
};
typedef struct _user_clkadvcat_info USER_CLKADVCAT_INFO;

#endif
