//3.0 support
#include <map>
#include <vector>
#include <string>
#include <errno.h>
#include <algorithm>
#include <set>
//#include <google/protobuf/util/json_util.h>
#include <google/protobuf/text_format.h>
#include "../../common/bid_filter.h"
#include "../../common/json_util.h"
#include "../../common/setlog.h"
#include "../../common/errorcode.h"
#include "../../common/google/decode.h"
#include "../../common/url_endecode.h"
#include "../../common/base64.h"
#include "../../common/bid_filter_dump.h"
#include "google_response.h"
using namespace std;

extern map<int, vector<uint32_t> > adv_cat_table_adxi;
extern map<uint32_t, vector<uint32_t> > adv_cat_table_adxo;
extern map<int, vector<uint32_t> > app_cat_table;
extern uint64_t g_logid_local, g_logid, g_logid_response;
extern map<string, uint16_t> dev_make_table;
extern map<uint8_t, uint8_t> creative_attr_in;
extern map<uint8_t, vector<uint8_t> > creative_attr_out;
extern map<int, int> geo_ipb;

extern uint64_t geodb;
extern MD5_CTX hash_ctx;
extern int *numcount;

#define  TRACKEVENT 5
string trackevent[5] = { "start", "firstQuartile", "midpoint", "thirdQuartile", "complete" };

/*typedef void (*CALLBACK_INSERT_PAIR)(
	IN void *data, //app cat转换表数据插入回调函数指针
	const string key_start, //key区间下限
	const string key_end, //key区间上限
	const string val //value
	);*/

void callback_insert_pair_int(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int, vector<uint32_t> > &table = *((map<int, vector<uint32_t> > *)data);

	uint32_t com_cat;
	sscanf(val.c_str(), "%x", &com_cat);//0x%x

	vector<uint32_t> &s_val = table[atoi(key_start.c_str())];

	s_val.push_back(com_cat);
}

void callback_insert_pair_attr_in(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint8_t, uint8_t> &table = *((map<uint8_t, uint8_t> *)data);

	uint32_t com_attr;
	sscanf(val.c_str(), "%d", &com_attr);
	cout << "key start :" << key_start << endl;

	cout << "val :" << val << endl;
	cout << "com_attr :" << com_attr << endl;

	table[atoi(key_start.c_str())] = com_attr;
}

void callback_insert_pair_geo_ipb(IN void *data, const string key_start, const string key_end, const string val)
{
	map<int, int> &table = *((map<int, int> *)data);

	int com_attr;
	sscanf(val.c_str(), "%d", &com_attr);
	cout << "key start :" << key_start << endl;

	cout << "val :" << val << endl;
	cout << "ipb :" << com_attr << endl;

	table[atoi(key_start.c_str())] = com_attr;
}

void callback_insert_pair_attr_out(IN void *data, const string key_start, const string key_end, const string val)
{
	map<uint8_t, vector<uint8_t> > &table = *((map<uint8_t, vector<uint8_t> > *)data);
	char *p = NULL;
	char *outer_ptr = NULL;
	cout << "key start :" << key_start << endl;
	cout << "val :" << val << endl;
	char value[1024];
	strncpy(value, val.c_str(), 1024);
	p = strtok_r(value, ",", &outer_ptr);
	while (p != NULL)
	{
		p = strtok_r(NULL, ",", &outer_ptr);
		if (p)
		{
			uint32_t com_attr;
			sscanf(p, "%d", &com_attr);

			vector<uint8_t> &s_val = table[atoi(key_start.c_str())];

			s_val.push_back(com_attr);
		}
	}
	return;
}

// convert google request to common request
int parse_common_request(IN BidRequest &bidrequest, OUT COM_REQUEST &crequest, OUT uint64_t &billing_id)
{
	int errorcode = E_SUCCESS;
	string userid;
	int geo_criteria_id = 0;
	string postal_code, postal_code_prefix;
	int user_data_treatment_count = 0;
	int gender = 0;
	COM_IMPRESSIONOBJECT cimpobj;

	init_com_message_request(&crequest);
	init_com_impression_object(&cimpobj);
	if (bidrequest.has_id())
	{
		char *id_encode = base64_encode(bidrequest.id().c_str(), bidrequest.id().size());
		if (id_encode != NULL)
		{
			crequest.id = id_encode;
			free(id_encode);
		}
		else
		{
			errorcode = E_REQUEST_ID;
			goto badrequest;
		}
	}
	else
	{
		errorcode = E_REQUEST_NO_BID;
		goto badrequest;
	}
	// The first 3 bytes of the IP address in network byte order for IPv4, or the
	// first 6 bytes for IPv6. Note that the number and position of the bytes
	// included from IPv6 addresses may change later.
	//# 4 bytes in IPv4, but last byte is truncated giving an overall length of 3
	//# bytes.
	if (bidrequest.has_ip())
	{
		string ip = bidrequest.ip();
		//string ip2 = ip.substr(0, 3);
		//		crequest.device.ip = bidrequest.ip();
		string ip_conv;
		if (ip.size() == 3)
		{
			for (int i = 0; i < 3; ++i)
			{
				ip_conv += intToString((unsigned char)ip[i]) + ".";
			}
			ip_conv += "0";
		}
		//cout <<"ip_conv :" <<ip_conv <<";"<<endl;	
		/*string
		char *id_encode = base64_encode(bidrequest.ip().c_str(), bidrequest.ip().size());
		cout <<"id :" << crequest.id <<",ip_encode :" <<id_encode <<endl;
		free(id_encode);*/
		//crequest.device.ip = "36.110.73.210";
		crequest.device.ip = ip_conv;
		crequest.device.location = getRegionCode(geodb, crequest.device.ip);
	}

	for (uint32_t i = 0; i < bidrequest.user_data_treatment_size(); ++i)
	{
		uint64_t id = bidrequest.user_data_treatment(i);
		++user_data_treatment_count;
	}

	if (user_data_treatment_count > 0)
	{
		userid = bidrequest.constrained_usage_google_user_id();
	}
	else
	{
		userid = bidrequest.google_user_id();
	}
	crequest.user.id = userid;
	if (bidrequest.has_user_agent())
	{
		crequest.device.ua = bidrequest.user_agent();
	}

	// Geo information. These use a subset of the codes used in the AdWords API.
	// See the geo-table.csv table in the technical documentation for a list of
	// ids. The geo_criteria_id field replaces the deprecated country, region,
	// city, and metro fields.
	if (bidrequest.has_geo_criteria_id())
	{
		geo_criteria_id = bidrequest.geo_criteria_id();
		map<int, int>::iterator geo_ipb_it;

		geo_ipb_it = geo_ipb.find(geo_criteria_id);
		if (geo_ipb_it != geo_ipb.end())
		{
			int ipb = geo_ipb_it->second;
			if (ipb != crequest.device.location)
			{
				errorcode = E_REQUEST_DEVICE_IP;
				writeLog(g_logid_local, LOGDEBUG, "id:%s,ip:%s,geo_criteria_id %d,geo_to_ipb %d,device_location %d", crequest.id.c_str(),
					crequest.device.ip.c_str(), geo_criteria_id, ipb, crequest.device.location);
				goto release;
			}
		}
		else
		{
			errorcode = E_REQUEST_DEVICE_IP;
			writeLog(g_logid_local, LOGDEBUG, "id:%s,ip:%s,geo_criteria_id %d,device_location %d", crequest.id.c_str(),
				crequest.device.ip.c_str(), geo_criteria_id, crequest.device.location);
			goto release;
		}
		//writeLog(g_logid_local, LOGINFO, "id:%s,ip:%s,geo_criteria_id %d", crequest.id.c_str(), crequest.device.ip.c_str(), geo_criteria_id);
	}

	if (bidrequest.has_encrypted_hyperlocal_set())
	{
		//const BidRequest_HyperlocalSet&  hyperlocalset = bidrequest.hyperlocal_set();
		string cleartext;
		BidRequest bid_req;
		const string encrypted_hyperlocal_set = bidrequest.encrypted_hyperlocal_set();
		writeLog(g_logid_local, LOGDEBUG, "id : %s and encrypted_hyperlocal_set :%s", crequest.id.c_str(), encrypted_hyperlocal_set.c_str());
		int status = DecodeByteArray(encrypted_hyperlocal_set, cleartext);
		if (status == E_SUCCESS)
		{
			BidRequest_HyperlocalSet hyperlocalsetinner;
			bool parsesuccess = hyperlocalsetinner.ParsePartialFromString(cleartext);
			if (parsesuccess)
			{
				writeLog(g_logid_local, LOGDEBUG, "%s parse hyperlocalsetinner success", crequest.id.c_str());
				if (hyperlocalsetinner.has_center_point())
				{
					writeLog(g_logid_local, LOGDEBUG, "%s has_center_point", crequest.id.c_str());
					const BidRequest_Hyperlocal_Point &centet_point = hyperlocalsetinner.center_point();
					crequest.device.geo.lat = centet_point.latitude();
					crequest.device.geo.lon = centet_point.longitude();
				}
				else if (hyperlocalsetinner.hyperlocal().size() > 0)
				{
					writeLog(g_logid_local, LOGDEBUG, "%s has_hyperlocal", crequest.id.c_str());
					printf("hyperlocal.size >0\n");
					const BidRequest_Hyperlocal& hyperlocal = hyperlocalsetinner.hyperlocal(0);
					// The mobile device can be at any point inside the geofence polygon defined
					// by a list of corners. Currently, the polygon is always a parallelogram
					// with 4 corners.
					if (hyperlocal.corners_size() > 0)
					{
						const BidRequest_Hyperlocal_Point& point = hyperlocal.corners(0);
						printf("lat :%lf and lon :%lf\n", point.latitude(), point.longitude());
						crequest.device.geo.lat = point.latitude();
						crequest.device.geo.lon = point.longitude();
					}
				}
				else
				{
					writeLog(g_logid_local, LOGDEBUG, "%s parse hyperlocalsetinner unknown", crequest.id.c_str());
				}
			}
			else
			{
				writeLog(g_logid_local, LOGDEBUG, "%s parse hyperlocalsetinner failed", crequest.id.c_str());
			}
		}
	}

	/*if(bidrequest.has_hyperlocal_set())
	{
	const BidRequest_HyperlocalSet&  hyperlocalset =bidrequest.hyperlocal_set();
	// This field currently contains at most one hyperlocal polygon.
	if(hyperlocalset.hyperlocal_size() >0)
	{
	const BidRequest_Hyperlocal& hyperlocal = hyperlocalset.hyperlocal(0);
	// The mobile device can be at any point inside the geofence polygon defined
	// by a list of corners. Currently, the polygon is always a parallelogram
	// with 4 corners.
	if(hyperlocal.corners_size() >0)
	{
	const BidRequest_Hyperlocal_Point& point = hyperlocal.corners(0);
	crequest.device.geo.lat = point.latitude();
	crequest.device.geo.lon = point.longitude();
	}
	}
	}*/

	if (bidrequest.has_user_demographic())
	{
		const BidRequest_UserDemographic &userdemographic = bidrequest.user_demographic();
		int age;
		int gender = GENDER_UNKNOWN;
		if (userdemographic.has_gender())
		{
			if (userdemographic.gender() == BidRequest_UserDemographic_Gender_MALE)
				gender = GENDER_MALE;
			else if (userdemographic.gender() == BidRequest_UserDemographic_Gender_FEMALE)
				gender = GENDER_FEMALE;
		}
		crequest.user.gender = gender;
		if (userdemographic.age_high())
		{

		}
		else if (userdemographic.age_low())
		{

		}
	}



	postal_code = bidrequest.postal_code();
	postal_code_prefix = bidrequest.postal_code_prefix();

	// google now support one adzone

	if (bidrequest.has_video())
	{
		cimpobj.type = IMP_VIDEO;
		cimpobj.requestidlife = 3600;
		const BidRequest_Video& video = bidrequest.video();
		if (video.is_clickable())
		{
			writeLog(g_logid_local, LOGINFO, "bid:%s is_clickable", crequest.id.c_str());
		}

		if (video.has_max_ad_duration())
		{
			// The maximum duration in milliseconds of the ad that you should return.
			// If this is not set or has value <= 0, any duration is allowed.
			int max_ad_duration = video.max_ad_duration();
			if (max_ad_duration > 0)
			{
				cimpobj.video.maxduration = max_ad_duration;
			}
		}

		if (video.has_min_ad_duration())
		{
			int min_ad_duration = video.min_ad_duration();
			if (min_ad_duration > 0)
			{
				cimpobj.video.minduration = min_ad_duration;
			}
		}
		// The maximum number of ads in an Adx video pod. A non-zero value indicates
		// that the current ad slot is a video pod that can show multiple video
		// ads. Actual number of video ads shown can be less than or equal to this
		// value but cannot exceed it.
		/*if(video.max_ads_in_pod() > 0)
		{

		}*/

		if (video.has_video_ad_skippable())
		{
			int video_ad_skippable = video.video_ad_skippable();
			if (video_ad_skippable == BidRequest_Video_SkippableBidRequestType_ALLOW_SKIPPABLE)
			{
				writeLog(g_logid_local, LOGINFO, "bid:%s video_ad_skippable:ALLOW_SKIPPABLE", crequest.id.c_str());
			}
			else if (video_ad_skippable == BidRequest_Video_SkippableBidRequestType_REQUIRE_SKIPPABLE)
			{
				writeLog(g_logid_local, LOGINFO, "bid:%s video_ad_skippable:REQUIRE_SKIPPABLE", crequest.id.c_str());
			}
			else if (video_ad_skippable == BidRequest_Video_SkippableBidRequestType_BLOCK_SKIPPABLE)
			{
				writeLog(g_logid_local, LOGINFO, "bid:%s video_ad_skippable:BLOCK_SKIPPABLE", crequest.id.c_str());
			}
		}

		if (video.has_skippable_max_ad_duration())
		{
			// The maximum duration in milliseconds for the ad you should return, if
			// this ad is skippable (this generally differs from the maximum duration
			// allowed for non-skippable ads). If this is not set or has value <= 0, any
			// duration is allowed.
			int skippable_max_ad_duration = video.skippable_max_ad_duration();
			if (skippable_max_ad_duration > 0)
			{

			}
		}

		for (int i = 0; i < video.allowed_video_formats_size(); ++i)
		{//The values for this repeated field come from the enumeration, VideoFormat:VIDEO_FLASH = 0;VIDEO_HTML5 = 1;
			int video_type = video.allowed_video_formats(i);
			switch (i)
			{
			case 0:// Flash video files are accepted (FLV).
			{
				video_type = VIDEOFORMAT_X_FLV;
				break;
			}
			case 1:
			{
				video_type = VIDEOFORMAT_MP4;
				break;
			}
			case 2:// Valid VAST ads with at least one media file hosted
				// on youtube.com.
			{
				video_type = VIDEO_YT_HOSTED;
				break;
			}
			case 3:// Flash VPAID (SWF).
			{
				video_type = VIDEOFORMAT_APP_X_SHOCKWAVE_FLASH;
				break;
			}
			case 4:// JavaScript VPAID.
			{
				video_type = VIDEO_VPAID_JS;
				break;
			}
			}
			cimpobj.video.mimes.insert(video_type);
		}

		for (int i = 0; i < video.companion_slot_size(); ++i)
		{
			const BidRequest_Video_CompanionSlot&  video_companionslot = video.companion_slot(i);
			// These fields represent the available heights and widths in this slot.
			// There will always be the same number heights and widths fields.
			for (int j = 0; j < video_companionslot.width_size(); j++)
			{

			}
			// These are the formats of the creatives allowed in this companion ad slot
			for (int j = 0; j < video_companionslot.creative_format_size(); j++)
			{

			}
		}
		// Attributes of the video that the user is viewing, not the video ad.
		// These fields are based on the availability of the video metadata from the
		// video publisher and may not always be populated.
		//
		if (video.has_content_attributes())
		{

		}
	}
	for (uint32_t i = 0; i < bidrequest.adslot_size(); ++i)
	{
		cimpobj.bidfloorcur = "CNY";
		const BidRequest_AdSlot &adslot = bidrequest.adslot(i);
		int32_t id = adslot.id();
		char tempid[1024] = "";
		sprintf(tempid, "%d", id);
		cimpobj.id = tempid;
		set<int> allowed_vendor_type;
		//需转换
		for (int i = 0; i < adslot.targetable_channel_size(); ++i)
		{

		}
		//需转换battr
		for (int j = 0; j < adslot.excluded_attribute_size(); ++j)
		{
			/*If 48 is present in the excluded_attribute field, SSL-compliant ads are required.
			  If 48 is not present in the excluded_attribute field, both SSL-compliant and non-SSL-compliant ads can be returned.*/
			int attr = adslot.excluded_attribute(j);
			if (attr == 48)
			{
				crequest.is_secure = 1;
				//errorcode = E_INVALID_PARAM;
			}
			cimpobj.banner.battr.insert(creative_attr_in[attr]);

		}
		//需转换
		for (int j = 0; j < adslot.allowed_vendor_type_size(); ++j)
		{
			allowed_vendor_type.insert(adslot.allowed_vendor_type(j));
		}
		// The disallowed sensitive ad categories. See the
		// ad-sensitive-categories.txt file in the technical documentation for a
		// list of ids. You should enforce these exclusions if you have the ability
		// to classify ads into the listed categories. This field does not apply to
		// deals with block overrides (see
		// https://support.google.com/adxbuyer/answer/6114194).
		//需转换bcat
		for (int j = 0; j < adslot.excluded_sensitive_category_size(); ++j)
		{
			int cat = adslot.excluded_sensitive_category(j);
			cimpobj.ext.sensitiveCat.insert(cat);
		}

		// The allowed restricted ad categories for private and open auctions. See
		// the ad-restricted-categories.txt file in the technical documentation for
		// a list of ids. These only apply for private and open auction bids. See
		// the allowed_restricted_category_for_deals field for preferred deals or
		// programmatic guarantees. If you bid with an ad in a restricted category,
		// you MUST ALWAYS declare the category in the bid response regardless of
		// the values in this field.
		//需转换
		for (int i = 0; i < adslot.allowed_restricted_category_size(); ++i)
		{

		}
		// The allowed restricted ad categories for preferred deals or programmatic
		// guarantees. See the ad-restricted-categories.txt file in the technical
		// documentation for a list of ids. These only apply for preferred deals or
		// programmatic guarantees. See the allowed_restricted_category field for
		// private and open auctions. In some cases, restricted categories are only
		// allowed on preferred deals or programmatic guarantees, so this field
		// lists all categories in allowed_restricted_category, and additionally,
		// restricted categories that are only allowed for preferred deals or
		// programmatic guarantees. If you bid with an ad in a restricted category,
		// you MUST ALWAYS declare the category in the bid response regardless of
		// the values in this field.
		//需转换
		for (int i = 0; i < adslot.allowed_restricted_category_for_deals_size(); ++i)
		{

		}
		// The disallowed ad product categories. See the ad-product-categories.txt
		// file in the technical documentation for a list of ids. You should enforce
		// these exclusions if you have the ability to classify ads into the listed
		// categories. This field does not apply to deals with block overrides (see
		// https://support.google.com/adxbuyer/answer/6114194).
		for (uint32_t i = 0; i < adslot.excluded_product_category_size(); ++i)
		{
			int cat = adslot.excluded_product_category(i);
			cimpobj.ext.productCat.insert(cat);
			/*vector<uint32_t> &adcat = adv_cat_table_adxi[cat];
			for(uint32_t j = 0; j < adcat.size(); ++j)
			crequest.bcat.insert(adcat[j]);*/
		}
		// Information about the pre-targeting configs that matched.
		for (int i = 0; i < adslot.matching_ad_data_size(); ++i)
		{
			// The billing ids corresponding to the pretargeting configs that matched.
			vector<int64_t>billing_vid;
			const BidRequest_AdSlot_MatchingAdData &matching_ad_data = adslot.matching_ad_data(i);
			for (int j = 0; j < matching_ad_data.billing_id_size(); ++j)
			{
				billing_vid.push_back(matching_ad_data.billing_id(j));
			}
			//billing_id = billing_vid[get_random_index(billing_vid.size())];
			if (billing_vid.size() > 0)
			{
				billing_id = billing_vid[i];
			}

			if (matching_ad_data.has_minimum_cpm_micros())
			{
				cimpobj.bidfloor = matching_ad_data.minimum_cpm_micros() / 1000000.0;
			}

			for (int j = 0; j < matching_ad_data.pricing_rule_size(); ++j)
			{
				const BidRequest_AdSlot_MatchingAdData::BuyerPricingRule &buyerpricingrule = matching_ad_data.pricing_rule(j);
				// Only one of the blocked and minimum_cpm_micros can be set in a rule.
				// If set to true, indicates that the specified advertisers/agencies are
				// blocked from bidding.
				//It is important to read the BuyerPricingRule.blocked flag.
				//If it is set to true in the first pricing rule matching the bid, the bid will get filtered regardless of the bid amount. 
				//Filtering will be for either ADX_AD_FILTERED_BY_RULE_EXCLUSION or BELOWCPM.


				bool blocked = false;
				if (buyerpricingrule.has_blocked())
				{
					if ((blocked = buyerpricingrule.blocked()))
					{
						if (j == 0)
						{
							errorcode = E_REQUEST_PRICING_RULE;
						}
						else
						{

						}
					}
				}

				if (buyerpricingrule.has_minimum_cpm_micros())
				{
					cimpobj.bidfloor = buyerpricingrule.minimum_cpm_micros() / 1000000.0;
				}
				//badv需转换
				if (blocked)
				{
					for (int k = 0; k < buyerpricingrule.included_advertisers_size(); ++k)
					{
						int64_t advertiser = buyerpricingrule.included_advertisers(k);
					}
					for (int k = 0; k < buyerpricingrule.included_agencies_size(); ++k)
					{
						int64_t agency = buyerpricingrule.included_agencies(k);
					}

				}

				//badv
				for (int k = 0; k < buyerpricingrule.excluded_advertisers_size(); ++k)
				{
					int64_t advertiser = buyerpricingrule.excluded_advertisers(k);
				}

				for (int k = 0; k < buyerpricingrule.excluded_agencies_size(); ++k)
				{
					int64_t agency = buyerpricingrule.excluded_agencies(k);
				}

				for (int k = 0; k < matching_ad_data.direct_deal_size(); ++k)
				{
					const BidRequest_AdSlot_MatchingAdData::DirectDeal &directdeal = matching_ad_data.direct_deal(k);
					int64_t direc_deal_id = directdeal.direct_deal_id();
					// You must bid at least fixed_cpm_micros (in micros of your account
					// currency) in order to paticipate in the deal. If you win, you will be
					// charged fixed_cpm_micros. This does not apply when
					// deal_type=PRIVATE_AUCTION. For private auctions, you must bid at
					// least fixed_cpm_micros and you pay the second price. Bidding higher
					// CPM than fixed_cpm_micros will increase your chance to win when
					// deal_type=PRIVATE_AUCTION, however it will not increase your chance
					// to win in other types of deals.
					if (directdeal.has_deal_type())
					{
						int  deal_type = directdeal.deal_type();
						if (deal_type == BidRequest_AdSlot_MatchingAdData_DirectDeal_DealType_PREFERRED_DEAL)
						{
							crequest.at = BID_PMP;
							if (directdeal.has_fixed_cpm_micros())
							{
								int64_t fixed_cpm_micros = directdeal.fixed_cpm_micros();
							}
						}
						else if (deal_type == BidRequest_AdSlot_MatchingAdData_DirectDeal_DealType_PRIVATE_AUCTION)
						{
							crequest.at = BID_PMP;
						}
					}
					if (directdeal.has_publisher_blocks_overridden())
					{
						// Whether the publisher has exempted this deal from configured blocks.
						// This setting does not override AdX policies or Ad Review Center
						// decisions.
						if (directdeal.publisher_blocks_overridden())
						{

						}
					}
				}

			}
		}

		for (int i = 0; i < adslot.publisher_settings_list_id_size(); ++i)
		{

		}

		// Visibility information for the slot.
		{
			//optional int32 viewability = 21 [default = -1];成功展现率
			// optional float click_through_rate = 25 [default = -1.0];点击率
		}

		if (adslot.native_ad_template_size() > 0)
		{
			cimpobj.type = IMP_NATIVE;
			cimpobj.requestidlife = 3600;
			int id = 0;
			int i = 0;
			cimpobj.native.layout = NATIVE_LAYOUTTYPE_NEWSFEED;
			//for(int i = 0 ;i < adslot.native_ad_template_size(); ++i)
			{
				const BidRequest_AdSlot_NativeAdTemplate& nativeadtemplate = adslot.native_ad_template(i);
				if (nativeadtemplate.has_required_fields())
				{
					uint64_t required_fields = nativeadtemplate.required_fields();
					writeLog(g_logid_local, LOGDEBUG, "id:%s, type:0x%x", crequest.id.c_str(), required_fields);
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_HEADLINE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_TITLE;
						comassetobj.title.len = nativeadtemplate.headline_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_BODY)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_DESC;
						comassetobj.data.len = nativeadtemplate.body_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_CALL_TO_ACTION)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_CTATEXT;
						comassetobj.data.len = nativeadtemplate.call_to_action_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_ADVERTISER)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_SPONSORED;
						comassetobj.data.len = nativeadtemplate.advertiser_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_IMAGE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_MAIN;
						comassetobj.img.w = nativeadtemplate.image_width();
						comassetobj.img.h = nativeadtemplate.image_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_LOGO)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_LOGO;
						comassetobj.img.w = nativeadtemplate.logo_width();
						comassetobj.img.h = nativeadtemplate.logo_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_APP_ICON)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_IMAGE;
						comassetobj.img.type = ASSET_IMAGETYPE_ICON;
						comassetobj.img.w = nativeadtemplate.app_icon_width();
						comassetobj.img.h = nativeadtemplate.app_icon_height();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_STAR_RATING)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_RATING;
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_PRICE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_PRICE;
						comassetobj.data.len = nativeadtemplate.price_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
					if (required_fields & BidRequest_AdSlot_NativeAdTemplate_Fields_STORE)
					{
						COM_ASSET_REQUEST_OBJECT comassetobj;
						init_com_asset_request_object(comassetobj);
						comassetobj.required = 1;
						comassetobj.id = ++id;
						comassetobj.type = NATIVE_ASSETTYPE_DATA;
						comassetobj.data.type = ASSET_DATATYPE_DOWNLOADS;
						comassetobj.data.len = nativeadtemplate.store_max_safe_length();
						cimpobj.native.assets.push_back(comassetobj);
					}
				}
			}
		}
		else if (cimpobj.type == IMP_VIDEO)
		{
			cimpobj.video.is_limit_size = false;
		}
		else
		{
			cimpobj.type = IMP_BANNER;
			cimpobj.requestidlife = 1800;
			if (adslot.width_size() > 0)
			{
				cimpobj.banner.w = adslot.width(0);
				cimpobj.banner.h = adslot.height(0);
			}
		}
		crequest.imp.push_back(cimpobj);
		break;
	}

	if (bidrequest.has_device())  // app bundle
	{
		const BidRequest_Device &device = bidrequest.device();
		string platform;

		if (device.device_type() == BidRequest_Device::HIGHEND_PHONE)
			crequest.device.devicetype = DEVICE_PHONE;
		else if (device.device_type() == BidRequest_Device::TABLET)
			crequest.device.devicetype = DEVICE_TABLET;
		else if (device.device_type() == BidRequest_Device::CONNECTED_TV)
			crequest.device.devicetype = DEVICE_TV;

		platform = device.platform();

		if (platform == "android")
		{
			crequest.device.os = OS_ANDROID;
		}
		else if (platform == "iphone" || platform == "ipad")
		{
			crequest.device.os = OS_IOS;
		}
		else
		{
			writeLog(g_logid_local, LOGDEBUG, "id :%s and platform :%s", crequest.id.c_str(), platform.c_str());
		}

		string s_make = device.brand();
		if (s_make != "")
		{
			//writeLog(g_logid_local, LOGDEBUG, "id %s, make find %s", crequest.id.c_str(), s_make.c_str());
			map<string, uint16_t>::iterator it;

			crequest.device.makestr = s_make;
			for (it = dev_make_table.begin(); it != dev_make_table.end(); ++it)
			{
				if (s_make.find(it->first) != string::npos)
				{
					crequest.device.make = it->second;
					break;
				}
			}
		}
		crequest.device.model = device.model();

		if (device.has_os_version())
		{
			char buf[64];
			sprintf(buf, "%d.%d.%d", device.os_version().major(), device.os_version().minor(), device.os_version().micro());
			crequest.device.osv = buf;
		}

		int32_t carrier = device.carrier_id();
		if (carrier == 70120)
			crequest.device.carrier = CARRIER_CHINAMOBILE;
		else if (carrier == 70123)
			crequest.device.carrier = CARRIER_CHINAUNICOM;
		else if (carrier == 70121)
			crequest.device.carrier = CARRIER_CHINATELECOM;
		else
			writeLog(g_logid_local, LOGDEBUG, "id :%s and device carrier :%d", crequest.id.c_str(), carrier);

		if (crequest.device.os == OS_IOS)
		{
			if (device.has_hardware_version())
			{
				// Apple iOS device model, e.g., "iphone 5s", "iphone 6+", "ipad 4".
			}
		}
	}

	if (bidrequest.has_mobile())
	{
		const BidRequest_Mobile& mobile = bidrequest.mobile();
		if (mobile.is_app())//不是媒体请是否要拒绝？？
		{

		}
		string appid;
		string dpid, endpid;
		int status = 0;

		//For Android devices, this is the fully
		// qualified package name. Examples: 343200656, com.rovio.angrybirds
		crequest.app.id = mobile.app_id();
		// If true, then this is a mobile full screen ad request.
		if (mobile.is_interstitial_request())
		{
			if (crequest.imp.size() > 0)
				crequest.imp[0].ext.instl = INSTL_FULL;
		}

		if (mobile.is_app())
		{
			//app.cat待转换
			for (int i = 0; i < mobile.app_category_ids_size(); i++)
			{
				int cat = mobile.app_category_ids(i);
				vector<uint32_t> &v_cat = app_cat_table[cat];
				for (uint32_t i = 0; i < v_cat.size(); ++i)
					crequest.app.cat.insert(v_cat[i]);
			}
		}

		if (mobile.has_encrypted_advertising_id())
		{
			//待解密
			endpid = mobile.encrypted_advertising_id();
			//writeLog(g_logid_local, LOGDEBUG, "id :%s and endpid :%s\n", crequest.id.c_str(), endpid.c_str());
			status = DecodeByteArray(endpid, dpid);
			if (status != E_SUCCESS)
			{
				writeLog(g_logid_local, LOGDEBUG, "parse advertising_id failed\n");
			}
			if (crequest.device.os == OS_ANDROID)
			{
				//cout <<"id :"<<crequest.id <<",len "<<dpid.size() <<",android :" <<dpid <<","<<endl;
				if (dpid.size() == 16)
				{
					char android[64] = "";
					sprintf(android,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					//cout << "bid :" <<crequest.id <<" ,android :" <<android <<endl;
					writeLog(g_logid_local, LOGDEBUG, "id :%s, android :%s, and ip:%s", 
						crequest.id.c_str(), android, crequest.device.ip.c_str());
					crequest.device.dpids.insert(make_pair(DPID_ANDROIDID_MD5, android));
				}
			}
			else if (crequest.device.os == OS_IOS)
			{
				if (dpid.size() == 16)
				{
					char IDFA[64] = "";

					sprintf(IDFA,
						"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					//cout << "bid :" <<crequest.id <<" ,IDFA :" <<IDFA <<endl;
					crequest.device.dpids.insert(make_pair(DPID_IDFA, string(IDFA)));
				}
			}
		}
		else if (mobile.has_encrypted_hashed_idfa())
		{
			//待解密
			endpid = mobile.encrypted_hashed_idfa();
			status = DecodeByteArray(endpid, dpid);
			if (status != E_SUCCESS)
			{
				writeLog(g_logid_local, LOGDEBUG, "parse encrypted_hashed_idfa failed\n");
				if (dpid.size() == 16)
				{
					char IDFA[64] = "";

					sprintf(IDFA,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						(unsigned char)dpid[0], (unsigned char)dpid[1], (unsigned char)dpid[2], (unsigned char)dpid[3],
						(unsigned char)dpid[4], (unsigned char)dpid[5], (unsigned char)dpid[6], (unsigned char)dpid[7],
						(unsigned char)dpid[8], (unsigned char)dpid[9], (unsigned char)dpid[10], (unsigned char)dpid[11],
						(unsigned char)dpid[12], (unsigned char)dpid[13], (unsigned char)dpid[14], (unsigned char)dpid[15]
						);
					cout << "bid :" << crequest.id << " ,IDFA_sha1 :" << IDFA << endl;
					crequest.device.dpids.insert(make_pair(DPID_IDFA_MD5, string(IDFA)));
				}
			}
		}

		// Only set if the BidRequest contains one or more user_data_treatment
		// value. If constrained_usage_encrypted_advertising_id or
		// constrained_usage_encrypted_hashed_idfa is set, then the corresponding
		// non-constrained field is not set. You must be whitelisted for all
		// user_data_treatments in this request in order to receive these fields.
		if (mobile.has_constrained_usage_encrypted_advertising_id())
		{
			endpid = mobile.constrained_usage_encrypted_advertising_id();
		}
		// App names for Android apps are from the Google Play store.
		// App names for iOS apps are provided by AppAnnie.
		crequest.app.name = mobile.app_name();
	}

	//crequest.device.devicetype = DEVICE_PHONE;
	//crequest.device.dids.insert(make_pair(DID_MAC_SHA1, "78ffc0b4fee4ed2d95eb53bbdccae14ff30aae09"));
	//crequest.device.os = OS_ANDROID;
	if (0 == crequest.imp.size())  // 没有符合的曝光对象
	{
		errorcode = E_REQUEST_IMP_SIZE_ZERO;
		writeLog(g_logid_local, LOGERROR, "bid=%s imp size is zero!", bidrequest.id().c_str());
		goto release;
	}

	for (int i = 0; i < bidrequest.detected_content_label_size(); ++i)
	{//需要转码
		int cat = bidrequest.detected_content_label(i);
		if (crequest.imp[0].type == IMP_BANNER)
		{
			vector<uint32_t> &v_cat = adv_cat_table_adxi[cat];
		}
	}

	if (crequest.device.location == 0 || crequest.device.location > 1156999999 || crequest.device.location < CHINAREGIONCODE)
	{
		cflog(g_logid_local, LOGDEBUG, "%s,getRegionCode ip:%s location:%d failed!", 
			crequest.id.c_str(), crequest.device.ip.c_str(), crequest.device.location);
		errorcode = E_REQUEST_DEVICE_IP;
	}

	// Feedback on bids submitted in previous responses. This is only set if
	// real-time feedback is enabled for your bidder. Please contact your account
	// manager if you wish to enable real-time feedback.
	//
	for (int i = 0; i < bidrequest.bid_response_feedback_size(); ++i)
	{
		const BidRequest_BidResponseFeedback& bidrespfeedback = bidrequest.bid_response_feedback(i);
		string req_id = bidrespfeedback.request_id();
		int status = bidrespfeedback.creative_status_code();
		int price = bidrespfeedback.cpm_micros();
		char *id_encode = base64_encode(req_id.c_str(), req_id.size());
		if (id_encode != NULL)
		{
			writeLog(g_logid_local, LOGDEBUG, "id :%s and status :%d and price %d", req_id.c_str(), status, price);
			free(id_encode);
		}
	}
	MY_DEBUG
	release :
	crequest.cur.insert(string("CNY"));
badrequest:
	return errorcode;
}

// set response ad category
/*void set_response_category_new(IN set<uint32_t> &cat,OUT BidResponse_Ad *bidresponseads)
{
if (NULL == bidresponseads)
return;

set<uint32_t>::iterator pcat = cat.begin();
bidresponseads->set_category(9901);
return;
}
*/
static bool filter_bid_price(IN const COM_REQUEST &crequest, IN const double &ratio, IN const double &price, IN const int &advcat)
{
	if ((price - crequest.imp[0].bidfloor - 0.01) >= 0.000001)
		return true;

	return false;
}
static bool filter_bid_ext(IN const COM_REQUEST &crequest, IN const CREATIVE_EXT &ext)
{
	bool filter = true;
	//  json_t *root = NULL;
	if (ext.id == "")
	{
		filter = false;
		goto exit;
	}
	/*if(ext.ext != "")
	{
	string cre_ext = ext.ext;
	json_parse_document(&root, cre_ext.c_str());
	if(root != NULL)
	{
	json_t *label;
	set<int> advid;
	set<int> procat;
	set<int> restrictcat;
	set<int> sendtivecat;

	if(JSON_FIND_LABEL_AND_CHECK(root, label, "advid", JSON_ARRAY))
	jsonGetIntSet(label, advid);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "procat", JSON_ARRAY))
	jsonGetIntSet(label, procat);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "restrictcat", JSON_ARRAY))
	jsonGetIntSet(label, restrictcat);
	if(JSON_FIND_LABEL_AND_CHECK(root, label, "sendtivecat", JSON_ARRAY))
	jsonGetIntSet(label, sendtivecat);
	}
	}*/
exit:
	/*if(root != NULL)
		json_free_value(&root);*/
	return filter;
}
void https_replace(string &a) 
{
    string temp = a;
    size_t s = 0;
    while(1)
    {   
        size_t pos = temp.find("http", s); 
        if(pos == string::npos)
            break;
        if(temp[pos+4] == 's')
        {   
            s += 5;
        }else{
            temp.replace(pos, 4, "https");
        }   
    }   
    a = temp;
}
void parse_common_response(IN COM_REQUEST &crequest, IN COM_RESPONSE &cresponse, IN ADXTEMPLATE &adxtemp, 
	IN uint64_t billing_id, OUT BidResponse &bidresponse)
{
/////////////////////////
    string str1 = "http";
    string str2 = "https";
	//replaceMacro(adxtemp.iurl[0], str1.c_str(), str2.c_str());
	//replaceMacro(adxtemp.cturl[0], str1.c_str(), str2.c_str());
	//replaceMacro(adxtemp.aurl, str1.c_str(), str2.c_str());
    replace_https_url(adxtemp, 1);
////////////////////////
	string trace_url_par = string("&") + get_trace_url_par(crequest, cresponse);
	// replace result
	int adcount = 0;
	for (uint32_t i = 0; i < cresponse.seatbid.size(); ++i)
	{
		COM_SEATBIDOBJECT &seatbidobject = cresponse.seatbid[i];
		for (uint32_t j = 0; j < seatbidobject.bid.size(); ++j)
		{
			COM_BIDOBJECT &bidobj = seatbidobject.bid[j];
			//replace_https_url(adxtemp,crequest.is_secure);

			BidResponse_Ad *bidresponseads = bidresponse.add_ad();
			if (NULL == bidresponseads)
				continue;
			//string extdata = "mapid=" + bidobj.mapid + "&deviceid="+crequest.device.deviceid + "&deviceidtype=" + intToString(crequest.device.deviceidtype);
			CREATIVE_EXT &ext = bidobj.creative_ext;
			if (ext.id.size() > 0)//创意审核返回的id
			{
				bidresponseads->set_buyer_creative_id(ext.id);
			}
			if (ext.ext != "")
			{
				string cre_ext;
				cre_ext = ext.ext;
				json_t *root = NULL;
				json_parse_document(&root, cre_ext.c_str());

				if (root != NULL)
				{
					json_t *label;

					if (JSON_FIND_LABEL_AND_CHECK(root, label, "advid", JSON_ARRAY))
					{

					}
					if (JSON_FIND_LABEL_AND_CHECK(root, label, "procat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->add_category(atoi(temp->text));
							temp = temp->next;
						}
					}
					if (JSON_FIND_LABEL_AND_CHECK(root, label, "restrictcat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->add_restricted_category(atoi(temp->text));
							temp = temp->next;
						}
					}

					if (JSON_FIND_LABEL_AND_CHECK(root, label, "sendtivecat", JSON_ARRAY))
					{
						json_t *temp = label->child->child;
						while (temp != NULL)
						{
							if (temp->type == JSON_NUMBER)
								bidresponseads->category(atoi(temp->text));
							temp = temp->next;
						}
					}
				}
				if (root != NULL)
					json_free_value(&root);
			}
			string curl = bidobj.curl;
            // replace http -- > https 
	        https_replace(curl);

			int type = bidobj.type;
			switch (type)
			{
			case ADTYPE_IMAGE:
			{
				if (adxtemp.adms.size() > 0)
				{
					string adm = "";
					//根据os和type查找对应的adm
					for (int count = 0; count < adxtemp.adms.size(); ++count)
					{
						if (adxtemp.adms[count].os == crequest.device.os && adxtemp.adms[count].type == bidobj.type)
						{
							adm = adxtemp.adms[count].adm;
							break;
						}
					}

					if (adm.size() > 0)
					{
                        // replace http -- > https 
	                    //replaceMacro(bidobj.sourceurl, str1.c_str(), str2.c_str());
						replaceMacro(adm, "#SOURCEURL#", bidobj.sourceurl.c_str());
						int lenout;
						char *enurl = urlencode(curl.c_str(), curl.size(), &lenout);
						replaceMacro(adm, "#CURL#", enurl);
						if (enurl != NULL)
							free(enurl);

						if (adxtemp.cturl.size() > 0)
						{
							string cturl = adxtemp.cturl[0] + trace_url_par;
							replaceMacro(adm, "#CTURL#", cturl.c_str());
						}
						/*if (adxtemp.iurl.size() > 0)
						{
						string iurl = adxtemp.iurl[0];
						replaceMacro(adm, "#IURL#", iurl.c_str());
						}*/

						//replaceMacro(adm, "#MONITORCODE#", bidobj.monitorcode.c_str());

						string cmonitor;
						if (bidobj.cmonitorurl.size() > 0)
						{

							cmonitor = bidobj.cmonitorurl[0];
                            // replace http -- > https 
	                        //replaceMacro(cmonitor, str1.c_str(), str2.c_str());
						}
						replaceMacro(adm, "#CMONITORURL#", cmonitor.c_str());
                        // replace http -- > https 
	                    https_replace(adm);
						bidresponseads->set_html_snippet(adm);
					}
				}
				break;
			}
			case ADTYPE_VIDEO:
			{
				// The URL to fetch a video ad. The URL should return an XML response that
				// conforms to the VAST 2.0 standard. Please use
				// BidResponse.Ad.AdSlot.billing_id to indicate which billing id to
				// attribute this ad to. Only one of the following should be set:
				// html_snippet, video_url. Only set this field if the BidRequest is for an
				// in-video ad (BidRequest.video is present).
				if (adxtemp.adms.size() > 0)
				{
					string adm = "";
					//根据os和type查找对应的adm
					for (int count = 0; count < adxtemp.adms.size(); ++count)
					{
						if (adxtemp.adms[count].os == crequest.device.os && adxtemp.adms[count].type == bidobj.type)
						{
							adm = adxtemp.adms[count].adm;
							break;
						}
					}

					if (adm.size() > 0)
					{
						string duration = "";
						string aurl;

						if (bidobj.creative_ext.ext.size() > 0)
						{
							string ext = bidobj.creative_ext.ext;
							json_t *tmproot = NULL;
							json_t *label;
							json_parse_document(&tmproot, ext.c_str());
							if (tmproot != NULL)
							{
								if ((label = json_find_first_label(tmproot, "duration")) != NULL)
									duration = label->child->text;
								json_free_value(&tmproot);
							}
						}
						replaceMacro(adm, "#DURATION#", duration.c_str());
                        // replace http -- > https 
	                    //replaceMacro(bidobj.sourceurl, str1.c_str(), str2.c_str());
						replaceMacro(adm, "#SOURCEURL#", bidobj.sourceurl.c_str());

						if (adxtemp.aurl.size() > 0)
						{
							for (int k = 0; k < TRACKEVENT; k++)
							{
								aurl += "<Tracking event=\"" + trackevent[k] + "\"><![CDATA[" + 
									adxtemp.aurl + trace_url_par + "&event=" + trackevent[k] + "]]></Tracking>";
							}
						}
						replaceMacro(adm, "#AURL#", aurl.c_str());

						if (adxtemp.iurl.size() > 0)
						{
							string iurl = adxtemp.iurl[0] + trace_url_par;
							replaceMacro(adm, "#IURL#", iurl.c_str());
						}
						string imonitorurl;
						if (bidobj.imonitorurl.size() > 0)
						{
							for (int k = 0; k < bidobj.imonitorurl.size(); ++k)
							{
								imonitorurl += "<Impression><![CDATA[" + bidobj.imonitorurl[k] + "]]></Impression>";
                                // replace http -- > https 
	                            //replaceMacro(imonitorurl, str1.c_str(), str2.c_str());
							}
						}
						replaceMacro(adm, "#IMONITORURL#", imonitorurl.c_str());

						replaceMacro(adm, "#CURL#", curl.c_str());

						if (adxtemp.cturl.size() > 0)
						{
							string cturl = adxtemp.cturl[0] + trace_url_par;
							replaceMacro(adm, "#CTURL#", cturl.c_str());
						}
						string cmonitorurl;
						for (int k = 0; k < bidobj.cmonitorurl.size(); k++)
						{
                            // replace http -- > https 
	                        //replaceMacro(bidobj.cmonitorurl[k], str1.c_str(), str2.c_str());
							cmonitorurl += "<ClickTracking><![CDATA[" + bidobj.cmonitorurl[k] + "]]></ClickTracking>";
						}

						replaceMacro(adm, "#CMONITORURL#", cmonitorurl.c_str());

						string attr = "type='video/MP4' width='" + intToString(bidobj.w) + "' height='" + intToString(bidobj.h) + "'";
						replaceMacro(adm, "#ATTR#", attr.c_str());
						replace_url(crequest.id, bidobj.mapid, crequest.device.deviceid, crequest.device.deviceidtype, adm);
						replaceMacro(adm, "'", "\"");
                        // replace http -- > https 
	                    https_replace(adm);
						bidresponseads->set_video_url(adm);
					}
				}
				break;
			}
			case ADTYPE_FEEDS:
			{
				BidResponse_Ad_NativeAd* native_ad = bidresponseads->mutable_native_ad();
				for (int j = 0; j < bidobj.native.assets.size(); ++j)
				{
					COM_ASSET_RESPONSE_OBJECT &comasset = bidobj.native.assets[j];
					if (comasset.required)
					{
						switch (comasset.type)
						{
						case NATIVE_ASSETTYPE_TITLE:
						{
							native_ad->set_headline(comasset.title.text);
							break;
						}
						case NATIVE_ASSETTYPE_IMAGE:
						{
							BidResponse_Ad_NativeAd_Image* image = NULL;
							switch (comasset.img.type)
							{
							case ASSET_IMAGETYPE_MAIN:
							{
								image = native_ad->mutable_app_icon();
								break;
							}
							case ASSET_IMAGETYPE_ICON:
							{
								image = native_ad->mutable_image();
								break;
							}
							case ASSET_IMAGETYPE_LOGO:
							{
								image = native_ad->mutable_logo();
								break;
							}
							}
							if (image != NULL)
							{
                                // replace http -- > https 
	                            https_replace(comasset.img.url);
								image->set_url(comasset.img.url);
								image->set_width(comasset.img.w);
								image->set_height(comasset.img.h);
							}

							break;
						}
						case NATIVE_ASSETTYPE_DATA:
						{
							switch (comasset.data.type)
							{
							case ASSET_DATATYPE_SPONSORED:
							{
								break;
							}
							case ASSET_DATATYPE_DESC:
							{
								native_ad->set_body(comasset.data.value);
								break;
							}
							case ASSET_DATATYPE_RATING:
							{
								native_ad->set_star_rating(atoi(comasset.data.value.c_str()));
								break;
							}
							case ASSET_DATATYPE_PRICE:
							{
								break;
							}
							case ASSET_DATATYPE_DOWNLOADS:
							{
								break;
							}
							case ASSET_DATATYPE_CTATEXT:
							{
								native_ad->set_call_to_action(comasset.data.value);
								break;
							}
							}
							break;
						}
						}
					}
				}

				// The URL to use for click tracking. The SDK pings click tracking url on
				// a background thread. When resolving the url, HTTP 30x redirects are
				// followed. The SDK ignores the contents of the response.
				//optional string click_tracking_url = 11;
				string nativecurl = adxtemp.cturl[0] + trace_url_par;
				/*int lenout;
				char *enurl = urlencode(curl.c_str(), curl.size(),  &lenout);
				nativecurl += "&curl=" + string(enurl, lenout);
				free(enurl);*/
				//replace the MACRO in cturl
				native_ad->set_click_tracking_url(nativecurl);
				// Link to ad preferences page. This is only supported for native ads.
				// If present, a standard AdChoices icon is added to the native creative and
				// linked to this URL.
				//bidresponseads->set_ad_choices_destination_url(curl);

				break;
			}
			}

			// The set of destination URLs for the snippet. This includes
			// the URLs that the user will go to if they click on the displayed
			// ad, and any URLs that are visible in the rendered ad. Do not
			// include intermediate calls to the adserver that are unrelated to the
			// final landing page. This data is used for post-filtering of
			// publisher-blocked URLs among other things. A BidResponse that returns a
			// snippet or video ad but declares no click_through_url will be discarded.
			// For native ads, only the first value is used as the click URL, though
			// all values are subject to categorization and review.  Only set this
			// field if html_snippet or video_url or native_ad are set. For native ads,
			// the first value is used to direct the user to the landing page.
			//repeated string click_through_url = 4;

			if (crequest.imp[0].type == IMP_NATIVE)
			{
				bidresponseads->add_click_through_url(curl);
			}
			if (bidobj.landingurl != "")
			{
				bidresponseads->add_click_through_url(bidobj.landingurl);
			}


			// All vendor types for the ads that may be shown from this snippet. You
			// should only declare vendor ids listed in the vendors.txt file in the
			// technical documentation. We will check to ensure that the vendors you
			// declare are in the allowed_vendor_type list sent in the BidRequest for
			// AdX publishers, or in gdn-vendors.txt for GDN publishers.
			// repeated int32 vendor_type = 5;

			//attr需转换
			for (int j = 0; i < bidobj.attr.size(); ++j)
			{

			}
			//cat需转换
			for (int j = 0; j < bidobj.cat.size(); ++j)
			{

			}
			// All restricted categories for the ads that may be shown from this
			// snippet. See ad-restricted-categories.txt in the technical documentation
			// for a list of ids. We will check to ensure these categories were listed
			// in the allowed_restricted_category list in the BidRequest. If you are
			// bidding with ads in restricted categories you MUST ALWAYS declare them
			// here.

			if (bidobj.adomain != "")
			{
				bidresponseads->add_advertiser_name(bidobj.adomain);
			}
			bidresponseads->set_width(bidobj.w);
			bidresponseads->set_height(bidobj.h);
			// The Agency associated with this ad. See agencies.txt file in the
			// technical documentation for a list of ids. If this ad has no associated
			// agency then the value NONE (agency_id: 1) should be used rather than
			// leaving this field unset.
			bidresponseads->set_agency_id(1);
			BidResponse_Ad_AdSlot *adslot = bidresponseads->add_adslot();
			if (adslot != NULL)
			{
				adslot->set_id(atoi(bidobj.impid.c_str()));
				adslot->set_max_cpm_micros(bidobj.price * 1000000);
				//待二次设置
				// Billing id to attribute this impression to. The value must be in the
				// set of billing ids for this slot that were sent in the
				// BidRequest.AdSlot.matching_ad_data.billing_id. This must always be set
				// if the BidRequest has more than one
				// BidRequest.AdSlot.matching_ad_data.billing_id.
				//optional int64 billing_id = 4;
				if (billing_id != 0)
					adslot->set_billing_id(billing_id);
				// The deal id that you want this bid to participate in. Leave unset
				// or set it to "1" if a deal is available but you want to
				// ignore the deal and participate in the open auction.
				adslot->set_deal_id(1);
			}
			// The URLs to call when the impression is rendered.  This is supported for
			// all inventory types and all formats except for VAST video.
			//repeated string impression_tracking_url = 19;
			if (crequest.imp[0].type != IMP_VIDEO)
			{
				if (adxtemp.iurl.size() > 0)
				{
					string iurl = adxtemp.iurl[0] + trace_url_par;
					//replaceMacro(iurl, "%%EXT_DATA%%", extdata.c_str());
					bidresponseads->add_impression_tracking_url(iurl);
				}
				for (int i = 0; i < bidobj.imonitorurl.size(); i++)
				{
					string iurl = bidobj.imonitorurl[i];

                    // replace http -- > https 
	                https_replace(iurl);
					bidresponseads->add_impression_tracking_url(iurl);
				}
			}
		}
	}

	return;
}

int getBidResponse(IN uint64_t ctx, IN uint8_t index, IN RECVDATA *recvdata, OUT string &data, OUT FLOW_BID_INFO &flow_bid)
{
	char *senddata = NULL;
	uint32_t sendlength = 0;
	BidRequest bidrequest;
	bool parsesuccess = false;
	int err = E_SUCCESS;

	FULL_ERRCODE ferrcode = { 0 };
	COM_REQUEST crequest;
	COM_RESPONSE cresponse;
	BidResponse bidresponse;
	uint8_t viewtype = 0;
	ADXTEMPLATE adxtemplate;
	struct timespec ts1, ts2;
	long lasttime = 0;
	string str_ferrcode = "";
	uint64_t billing_id = 0;
	getTime(&ts1);
	string original_req;
	string responseret;
    bool is_https = false;

	if (recvdata == NULL || recvdata->data == NULL || recvdata->length == 0)
	{
		err = E_BID_PROGRAM;
		writeLog(g_logid_local, LOGERROR, "recv data is error!");
		goto release;
	}

	parsesuccess = bidrequest.ParseFromArray(recvdata->data, recvdata->length);
	if (!parsesuccess)
	{
		err = E_BAD_REQUEST;
		writeLog(g_logid_local, LOGERROR, "Parse data from array failed!");
		va_cout("parse data from array failed!");
		goto release;
	}

	err = parse_common_request(bidrequest, crequest, billing_id);
	if (err != E_SUCCESS)
	{
		writeLog(g_logid_local, LOGERROR, "id = %s the request is not suit for bidding!", crequest.id.c_str());
		va_cout("the request is not suit for bidding!");
		goto returnresponse;
	}
    ////////////////////
    if(crequest.is_secure > 0)
    {
        is_https = true; 
        crequest.is_secure = 0; 
    }
    ////////////////////
	flow_bid.set_req(crequest);
	ferrcode = bid_filter_response(ctx, index, crequest, NULL, NULL, filter_bid_price, filter_bid_ext, &cresponse, &adxtemplate);
	str_ferrcode = bid_detail_record(600, 10000, ferrcode);
	err = ferrcode.errcode;
	flow_bid.set_bid_flow(ferrcode);
	g_ser_log.send_bid_log(0, crequest, cresponse, err);

	if (err == E_SUCCESS)
	{
		parse_common_response(crequest, cresponse, adxtemplate, billing_id, bidresponse);
		va_cout("parse common response to google response success!");
	}

returnresponse:
	//If you do not wish to bid on an impression, you can set the processing_time_ms field alone, and leave all other fields empty.
	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	bidresponse.set_processing_time_ms(lasttime);
	sendlength = bidresponse.ByteSize();
	senddata = (char *)calloc(1, sizeof(char) * (sendlength + 1));
	if (senddata == NULL)
	{
		va_cout("allocate memory failure!");
		writeLog(g_logid_local, LOGERROR, "allocate memory failure!");
		goto release;
	}
	bidresponse.SerializeToArray(senddata, sendlength);

	data = "Content-Type: application/octet-stream\r\nContent-Length: " + 
		intToString(sendlength) + "\r\n\r\n" + string(senddata, sendlength);

	if (err == E_SUCCESS)
	{
		//    writeLog(g_logid_local, LOGDEBUG,"response= " + string(senddata,sendlength));
		writeLog(g_logid_response, LOGINFO, "%s,%s,%s,%d,%lf", 
			cresponse.id.c_str(), cresponse.seatbid[0].bid[0].mapid.c_str(), 
			crequest.device.deviceid.c_str(), crequest.device.deviceidtype, 
			cresponse.seatbid[0].bid[0].price);
		numcount[index]++;
		writeLog(g_logid, LOGINFO, string(recvdata->data, recvdata->length));
		if (numcount[index] % 1000 == 0)
		{
			google::protobuf::TextFormat::PrintToString(bidresponse, &responseret);
			writeLog(g_logid_local, LOGDEBUG, "response id:" + crequest.id + ", " + responseret);
			numcount[index] = 0;
		}
	}
	bidresponse.clear_ad();

	getTime(&ts2);
	lasttime = (ts2.tv_sec - ts1.tv_sec) * 1000000 + (ts2.tv_nsec - ts1.tv_nsec) / 1000;
	writeLog(g_logid_local, LOGDEBUG, "bidver:%s,%s,spent time:%ld,err:0x%x,desc:%s", 
		VERSION_BID, crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());
	va_cout("%s,spent time:%ld,err:0x%x,desc:%s", crequest.id.c_str(), lasttime, err, str_ferrcode.c_str());

release:
	if (senddata != NULL)
	{
		free(senddata);
		senddata = NULL;
	}

	MY_DEBUG;
	flow_bid.set_err(err);
	return err;
}
