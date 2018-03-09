#ifndef		ERRORCODE_H_
#define		ERRORCODE_H_

#define		E_SUCCESS								0x0     //成功

//公共错误
#define		E_UNKNOWN								0xE0000000  //[程序]由于程序逻辑较多，难免有遗漏之处，若此错误码出现过多，则需要改写程序
#define		E_BID_PROGRAM							0xE0000001  //[程序]竞价引擎程序问题，一般不会出现
#define		E_BAD_REQUEST							0xE0001000  //[流量]请求解析失败
#define		E_MALLOC								0xE0002000  //[程序]分配内存失败，一般不会出现
#define		E_SEMAPHORE								0xE0003000  //信号量操作失败
#define		E_CREATETHREAD							0xE0004000  //创建线程失败
#define		E_MUTEX_INIT							0xE0005000  //
#define		E_PROFILE_INIT							0xE0006000  //配置文件初始化失败
#define		E_PROFILE_INIT_MAKE						0xE0006010  //配置文件初始化失败MAKE
#define		E_PROFILE_INIT_ADVCAT					0xE0006020  //配置文件初始化失败ADVCAT
#define		E_PROFILE_INIT_APPCAT					0xE0006030  //配置文件初始化失败APPCAT
#define		E_PROFILE_INIT_ADVID					0xE0006040  //配置文件初始化失败ADVID
#define		E_PROFILE_INIT_CRE_ATTR					0xE0006060  //配置文件初始化失败CRE ATTR (创意属性)
#define		E_PROFILE_INIT_GEOIPB					0xE0006070  //配置文件初始化失败私有的区域编码
#define		E_PROFILE_INIT_TEMPLATE					0xE0006080  //配置文件初始化失败广告模板
#define		E_LOG_SERVER							0xE0007000
#define		E_LOG_SERVER_HDFS						0xE0007001  //流量服务器HDFS
#define		E_LOG_SERVER_BIDKAFKA					0xE0007002  //流量服务器kafka
#define		E_LOG_SERVER_DETAIL						0xE0007100  //流量详情log服务器
#define		E_LOG_SERVER_DETAILKAFKA				0xE0007101  //流量详情log服务器kafka

#define		E_INVALID_PARAM							0xE0010000//函数参数错误
#define		E_INVALID_PARAM_LOGID					0xE0011000//无效的日志文件id
#define		E_INVALID_PARAM_LOGID_LOC				0xE0011010//无效的local日志文件id
#define		E_INVALID_PARAM_LOGID_RESP				0xE0011020//无效的response日志文件id
#define		E_INVALID_PARAM_ADX						0xE0012000//无效的adx
#define		E_INVALID_PARAM_CTX						0xE0013000//无效的ctx指针
#define		E_INVALID_PARAM_REQUEST					0xE0014000//
#define		E_INVALID_PARAM_RESPONSE				0xE0015000//
#define		E_INVALID_PARAM_PADXTEMPLATE			0xE0016000//
#define		E_REQUEST_INVALID_BID_TYPE				0xE0017000//[流量] 无效的竞价类型（目前只有rtb和pmp两种）
#define		E_INVALID_PARAM_JSON					0xE0018000
#define		E_INVALID_CPU_COUNT						0xE0019000

#define		E_FILE									0xE0020000//文件操作失败
#define		E_INVALID_PROFILE						0xE0021000//配置文件读取错误
#define		E_INVALID_PROFILE_IPB					0xE0021010//配置文件读取错误
#define		E_INVALID_PROFILE_BIDIP					0xE0021020//配置文件读取错误 发送竞价日志IP
#define		E_INVALID_PROFILE_BIDPORT				0xE0021021//配置文件读取错误 发送竞价日志端口
#define		E_INVALID_PROFILE_BIDBROKER				0xE0021023//配置文件读取错误 发送竞价日志BROKER LIST(kafka方式)
#define		E_INVALID_PROFILE_BIDTOPIC				0xE0021024//配置文件读取错误 发送竞价日志TOPIC(kafka方式)
#define		E_INVALID_PROFILE_DETAILIP				0xE0021030//配置文件读取错误 发送竞价详情日志IP
#define		E_INVALID_PROFILE_DETAILPORT			0xE0021031//配置文件读取错误 发送竞价详情日志端口
#define		E_INVALID_PROFILE_DETAILBROKER			0xE0021032//配置文件读取错误 发送竞价详情日志BROKER LIST(kafka方式)
#define		E_INVALID_PROFILE_DETAILTOPIC			0xE0021033//配置文件读取错误 发送竞价详情日志TOPIC(kafka方式)
#define		E_FILENOTEXIST							0xE0022000//文件不存在
#define		E_FOLDEREXIST							0xE0023000//目录已存在
#define		E_FILE_IPB								0xE0024000//打开IPB文件失败

#define		E_REDIS									0xE0030000//Redis连接/访问错误
#define		E_REDIS_CONNECT_INVALID					0xE0031000//redis连接失效
#define		E_REDIS_MASTER_MAJOR					0xE0031100//无法连接master_major
#define		E_REDIS_SLAVE_MAJOR						0xE0031200//无法连接slave_major
#define		E_REDIS_MASTER_FILTER_ID				0xE0031300//无法连接master_filter_id
#define		E_REDIS_SLAVE_GR						0xE0031400//无法连接slave_ur
#define		E_REDIS_SLAVE_UR						0xE0031500//无法连接slave_ur
#define		E_REDIS_MASTER_PUBSUB					0xE0031600//无法连接订阅端口
#define		E_REDIS_MASTER_FILTER_SMART				0xE0031700//无法连接master_filter_smart
#define		E_REDIS_CONNECT_R_INVALID				0xE0031800//读redis连接失效
#define		E_REDIS_CONNECT_W_INVALID				0xE0031900//写redis连接失效
#define		E_REDIS_ADVCAT							0xE0031A00//无法连接智能推荐相关redis端口
#define		E_REDIS_SLAVE_PRICE						0xE0031B00//无法连接slave_price // 20170708: add
#define		E_REDIS_COMMAND							0xE0032000//执行redis命令失败
#define		E_REDIS_KEY_NOT_EXIST					0xE0032100//查找key不存在
#define		E_REDIS_NO_CONTENT						0xE0033000//对应key无内容
#define		E_REDIS_MASTER_WBLIST					0xE0034000//无法连接master_wblist
#define		E_REDIS_SLAVE_WBLIST					0xE0035000//无法连接slave_wblist

#define		E_CACHE											0xE0040000//[程序]Redis Cache 数据不匹配（adx模板错误，模板中没有新增响应adx）

#define		E_FILTER										0xE0050000//[流量]竞价引擎中，没有适合投放的广告
#define		E_FILTER_HAVE_NO_GROUP							0xE0051000//没有投放的项目
#define		E_FILTER_HAVE_NO_AVAILABLE_GROUP				0xE0051100//没有有效的项目（项目频次都达到）
#define		E_FILTER_HAVE_NO_CREATIVE						0xE0051200//没有找到符合的创意
#define		E_FILTER_BCAT_OR_BADV_GREXT						0xE0052000//设备白名单或黑名单项目的所有广告组，全因bcat或者badv过滤失败
#define		E_FILTER_BCAT									0xE0052100//[单个group] bcat过滤失败
#define		E_FILTER_BADV									0xE0052200//[单个group] badv过滤失败
#define		E_FILTER_GROUP_EXT								0xE0052300//[单个group] ext过滤失败，一般检查请求的advid和广告组的advid是否匹配
#define		E_FILTER_GROUP_BID								0xE0052400//[单个group] 投放模式不匹配（PMP RTB）
#define		E_FILTER_GROUP_SECURE							0xE0052410//[单个group] 安全模式不匹配
#define		E_FILTER_GROUP_PMP_DEALID						0xE0052500//[单个group] PMP项目dealid不符合
#define		E_FILTER_GROUP_WBL								0xE0053000//设备黑白名单项目失败
#define		E_FILTER_GROUP_NOT_WL							0xE0053010//[单个group] 设备ID不在白名单
#define		E_FILTER_GROUP_IN_BL							0xE0053020//[单个group] 设备ID在黑名单
#define		E_FILTER_GROUP_ALLBL							0xE0053100//[单个group] 设备id在（总的）黑名单中
#define		E_REQUEST_PROCESS_CB							0xE0054000//[流量]，各个adx回调函数，一般是检查货币单位
#define		E_FILTER_TARGET									0xE0056000//整个设备黑白名单或通投项目，定向失败
#define		E_FILTER_TARGET_APP_ID							0xE0056110//[单个group] appid定向过滤失败
#define		E_FILTER_TARGET_APP_CAT							0xE0056120//[单个group] app cat定向过滤失败
#define		E_FILTER_TARGET_DEVICE							0xE0056200//[单个group] 设备定向设置为全部不支持
#define		E_FILTER_TARGET_DEVICE_REGIONCODE				0xE0056210//[单个group] 地域定向失败
#define		E_FILTER_TARGET_DEVICE_CONNECTIONTYPE			0xE0056220//[单个group] 网络连接类型定向失败
#define		E_FILTER_TARGET_DEVICE_OS						0xE0056230//[单个group] 操作系统定向失败
#define		E_FILTER_TARGET_DEVICE_CARRIER					0xE0056240//[单个group] 运营商定向失败
#define		E_FILTER_TARGET_DEVICE_DEVICETYPE				0xE0056250//[单个group] 设别类型定向失败
#define		E_FILTER_TARGET_DEVICE_MAKE						0xE0056260//[单个group] 制造商定向失败
#define		E_REQUEST_TARGET_ADX							0xE0056300//[流量] adx平台定向失败
#define		E_FILTER_TARGET_SCENE							0xE0056400//[单个group] 场景定向失败
#define		E_FILTER_FRC_GROUP								0xE0056500//[单个group] 平滑投放达到上限
#define		E_FILTER_FRC									0xE0057000//[单个creative] 用户频次达到上限
#define		E_FILTER_PRICE_LOW								0xE0058001//[单个creative] 优先选择价高创意，价低者被忽略
#define		E_FILTER_PRICE_WLIST							0xE0058100//[单个creative] 白名单项目下创意价格不符合
#define		E_FILTER_PRICE_CALLBACK							0xE0058200//[单个creative] 回调函数检查创意价格不符合
#define		E_FILTER_EXTS									0xE0059000//[单个creative] 创意的ext字段不符合（目前为没有写审核过的创意id）
#define		E_FILTER_IMP_UNSUPPORTED						0xE005A000//[单个creative] 广告类型不支持（支持banner,video,native）
#define		E_FILTER_IMP_BANNER_SIZE						0xE005B100//[单个creative] banner的尺寸不符合
#define		E_FILTER_IMP_BANNER_BTYPE						0xE005B200//[单个creative] banner广告创意类型不符合
#define		E_FILTER_IMP_BANNER_BATTR						0xE005B300//[单个creative] banner广告属性不符合
#define		E_FILTER_IMP_BANNER_TYPE						0xE005B400//[单个creative] 该创意不是banner广告
#define		E_FILTER_IMP_VIDEO_FTYPE						0xE005C100//[单个creative] 视频类创意文件类型不适合
#define		E_FILTER_IMP_VIDEO_TYPE							0xE005C200//[单个creative] 该创意不是video广告
#define		E_FILTER_IMP_VIDEO_SIZE							0xE005C300//[单个creative] video广告尺寸不符合
#define		E_REQUEST_IMP_NATIVE_LAYOUT						0xE005D100//[流量] 原生广告布局样式不是信息流
#define		E_REQUEST_IMP_NATIVE_PLCMTCNT					0xE005D200//[流量] 原生广告数量小于1个
#define		E_FILTER_IMP_NATIVE_BCTYPE						0xE005D300//[单个creative] 原生广告点击类型不符合
#define		E_REQUEST_IMP_NATIVE_ASSETCNT					0xE005D400//[流量] 原生广告元素列表太少（至少是title and image）
#define		E_FILTER_IMP_NATIVE_ASSET_TYPE					0xE005D500//[单个creative] 广告元素类型不符合
#define		E_FILTER_IMP_NATIVE_ASSET_TITLE_LENGTH			0xE005D610//[单个creative] title的长度不符合
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_TYPE			0xE005D710//[单个creative] image广告元素的类型不符合（只是icon或者大图）
#define		E_REQUEST_NATIVE_ASSET_IMAGE_SIZE				0xE005D720//[流量] image广告元素的尺寸宽高有0值
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_SIZE		0xE005D731//[单个creative] icon的尺寸不符合
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_FTYPE		0xE005D732//[单个creative] icon的图片类型不符合
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_URL		0xE005D733//[单个creative] icon的素材地址为空
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_SIZE		0xE005D741//[单个creative] 大图的尺寸不符合
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_FTYPE		0xE005D742//[单个creative] 大图的图片类型不符合
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_URL		0xE005D743//[单个creative] 大图的素材地址为空
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_NUM				0xE005D744//[单个creative] 请求大图的数量与创意中大图数量不匹配
#define		E_REQUEST_IMP_NATIVE_ASSET_DATA_TYPE			0xE005D810//[流量] 信息流中data元素的类型不符合（描述，rating，ctatext）
#define		E_REQUEST_IMP_NATIVE_ASSET_DATA_LENGTH			0xE005D820//[流量] 信息流中的描述长度不符合
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_DESC_LENGTH		0xE005D831//[单个creative] 创意下描述的长度不符合
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_RATING_VALUE		0xE005D841//[单个creative] rating的值大于5
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_CTATEXT			0xE005D842//[单个creative] ctatext为空
#define		E_FILTER_IMP_NATIVE_TYPE						0xE005D900//[单个creative] 广告类型不是信息流
#define		E_REQUEST_IMP_INSTL_SIZE_NOT_FIND				0xE005E000//[流量] banner型请求， 在adx下没有找到支持的该尺寸
#define		E_FILTER_ADVCAT									0xE005E100//[单个creative] 广告行业关联度不是最匹配
#define		E_FILTER_LAST_RANDOM							0xE005E101//[单个creative] 最后的几个合适的创意里，只随机取一个，剩余的不被选中
#define		E_FILTER_DEEPLINK								0xE005E102//[单个creative] deeplink


#define		E_ADXTEMPLATE_NOT_FIND_ADX						0xE0061100//[后期处理] 模板中没有该adx
#define		E_ADXTEMPLATE_NOT_FIND_IURL						0xE0061200//[后期处理] 模板中没有iurl
#define		E_ADXTEMPLATE_NOT_FIND_CTURL					0xE0061300//[后期处理] 模板中没有cturl
#define		E_ADXTEMPLATE_NOT_FIND_AURL						0xE0061400//[后期处理] 模板中没有aurl
#define		E_ADXTEMPLATE_NOT_FIND_NURL						0xE0061500//[后期处理] 模板中没有nurl
#define		E_ADXTEMPLATE_NOT_FIND_ADM						0xE0061600//[后期处理] 模板中没有adm
#define		E_ADXTEMPLATE_RATIO_ZERO						0xE0062000//[后期处理] 模板中的价格比例是0

#define		E_REDIS_GROUP_FC_INVALID						0xE0064000//项目频次信息非法
#define		E_REDIS_INVALID_CREATIVE_INFO					0xE0065001//创意价格为0或拒绝投放 
#define		E_REDIS_NO_CREATIVE								0xE0065002//项目没有投放的创意

#define		E_REQUEST										0xE0070000//
#define		E_REQUEST_HTTP_METHOD							0xE0070001//[流量] HTTP方法不对，有的需要是POST
#define		E_REQUEST_HTTP_CONTENT_TYPE						0xE0070002//[流量] HTTP CONTENT_TYPE 不匹配
#define		E_REQUEST_HTTP_CONTENT_LEN						0xE0070003//[流量] HTTP CONTENT_LEN 为0
#define		E_REQUEST_HTTP_BODY								0xE0070004//[流量] HTTP BODY 出错
#define		E_REQUEST_NO_BID								0xE0070006//[流量] 没找到bid
#define		E_REQUEST_HTTP_REMOTE_ADDR						0xE0070007//[流量] HTTP REMOTE_ADDR没找到
#define		E_REQUEST_HTTP_OPENRTB_VERSION					0xE0070008//[流量] HTTP OPENRTB_VERSION没找到
#define		E_REQUEST_ID									0xE0071000//[流量] 请求bid有问题
#define		E_REQUEST_IS_PING								0xE0071001//[流量] 请求是PING，可以当作是个测试类请求
#define		E_REQUEST_AT									0xE0071002//[流量] 流量类型不支持
#define		E_REQUEST_PMP_DEALID							0xE0071003//[流量] PMP没有价格
#define		E_REQUEST_DEVICE								0xE0072000//[流量] 请求中的设备信息有问题
#define		E_REQUEST_DEVICE_OS								0xE0072100//[流量] 请求os不是ios和android
#define		E_REQUEST_DEVICE_ID								0xE0072200//[流量] 没有设备id
#define		E_REQUEST_DEVICE_ID_DID_UNKNOWN					0xE0072210//[流量] did类型不符合
#define		E_REQUEST_DEVICE_ID_DID_IMEI					0xE0072211//[流量] imei不合法
#define		E_REQUEST_DEVICE_ID_DID_MAC						0xE0072212//[流量] mac不合法
#define		E_REQUEST_DEVICE_ID_DID_SHA1					0xE0072213//[流量] did的sha1值不合法
#define		E_REQUEST_DEVICE_ID_DID_MD5						0xE0072214//[流量] did的md5值不合法
#define		E_REQUEST_DEVICE_ID_DID_OTHER					0xE0072215//[流量] did的other类型长度为0
#define		E_REQUEST_DEVICE_ID_DPID_UNKNOWN				0xE0072220//[流量] dpid类型不符合
#define		E_REQUEST_DEVICE_ID_DPID_ANDROIDID				0xE0072221//[流量] android不合法
#define		E_REQUEST_DEVICE_ID_DPID_IDFA					0xE0072222//[流量] idfa不合法
#define		E_REQUEST_DEVICE_ID_DPID_SHA1					0xE0072223//[流量] dpid的sha1不合法
#define		E_REQUEST_DEVICE_ID_DPID_MD5					0xE0072224//[流量] dpid的md5不合法
#define		E_REQUEST_DEVICE_ID_DPID_OTHER					0xE0072225//[流量] dpid的other类型长度为0
#define		E_REQUEST_DEVICE_IP								0xE0072300//[流量] ip地址不在国内
#define		E_REQUEST_DEVICE_TYPE							0xE0072400//[流量] 设备类型不被支持（目前只支持手机，平板和电视，个别adx只支持一种）
#define		E_REQUEST_DEVICE_MAKE_NIL						0xE0072501//[流量] 设备make字段为空
#define		E_REQUEST_DEVICE_MAKE_OS						0xE0072502//[流量] 设备make与os不一致
#define		E_REQUEST_IMP_SIZE_ZERO							0xE0073000//[流量] 请求的广告位为0
#define		E_REQUEST_IMP_NO_ID								0xE0073001//[流量] 请求的广告位没ID
#define		E_REQUEST_IMP_TYPE								0xE0073002//[流量] 请求的广告位类型不被支持
#define		E_REQUEST_IMP_NATIVE_ASSET_TITLE_LENGTH			0xE0073003//[流量] 请求的广告位是native，但标题长度不合法
#define		E_REQUEST_IMP_NATIVE_ASSET_IMAGE_TYPE			0xE0073004//[流量] 请求的广告位是native，图片类型不合法
#define		E_REQUEST_IMP_NATIVE_ASSET_TYPE					0xE0073005//[流量] 请求的广告位是native，ASSET类型不支持
#define		E_REQUEST_BANNER								0xE0073010//[流量] 请求的广告位banner获取数据失败
#define		E_REQUEST_VIDEO									0xE0073011//[流量] 请求的广告位video获取数据失败
#define		E_REQUEST_NATIVE								0xE0073012//[流量] 请求的广告位native获取数据失败
#define		E_REQUEST_VIDEO_ADTYPE_INVALID					0xE0073013//[流量] 请求的视频广告类型不支持
#define		E_REQUEST_PRICING_RULE							0xE0073100//[流量] 请求中价格获取失败
#define		E_REQUEST_APP									0xE0074000//[流量] 请求的APP信息有问题

#define		E_PV											0xE0080000//关于pv的错误
#define		E_PV_INVALID_TIME								0xE0080100
#define		E_PV_INVALID_ID									0xE0081000//pv的id无效(落地页id)
#define		E_PV_INVALID_TY									0xE0082000//pv的ty无效(请求类型)
#define		E_PV_INVALID_BIDMID								0xE0083000//pv的mid/bid无效(创意id/广告id)
#define		E_PV_INVALID_SYSDS								0xE0084000//pv的sys/ds无效(操作系统/设备分辨率)
#define		E_PV_INVALID_PVUV								0xE0085000//pv的pv/uv无效(展现数/用户数)
#define		E_PV_INVALID_PU									0xE0086000//pv的pu无效(上游连接)
#define		E_PV_INVALID_HF									0xE0087000//二跳时,pv的hf无效(二跳地址)
#define		E_PV_INVALID_AT									0xE0088000//二跳或者跳出时,pv的at无效(平均访问时长)
#define		E_PV_INVALID_CL									0xE0089000//访问时,pv的cl无效(受访页面)


////////////////////////////////////////////////////////////////////////////////////////////

#define		E_TRACK									0xE0090000//追踪系统的错误
#define		E_TRACK_EMPTY							0xE0091000//空错误
#define		E_TRACK_EMPTY_CPU_COUNT					0xE0091001//[flume]cpu_count为空
#define		E_TRACK_EMPTY_FLUME_IP					0xE0091002//[flume]flume_ip为空
#define		E_TRACK_EMPTY_FLUME_PORT				0xE0091003//[flume]flume_port为空
#define		E_TRACK_EMPTY_KAFKA_TOPIC				0xE0091004//[flume]kafka_topic为空
#define		E_TRACK_EMPTY_KAFKA_BROKER_LIST			0xE0091005//[flume]kafka_broker_list为空
#define		E_TRACK_EMPTY_MASTER_UR_IP				0xE0091006//[flume]master_ur_ip为空
#define		E_TRACK_EMPTY_MASTER_UR_PORT			0xE0091007//[flume]master_ur_port为空
#define		E_TRACK_EMPTY_SLAVE_UR_IP				0xE0091008//[flume]slave_ur_ip为空
#define		E_TRACK_EMPTY_SLAVE_UR_PORT				0xE0091009//[flume]slave_ur_port为空
#define		E_TRACK_EMPTY_MASTER_GROUP_IP			0xE0091010//[flume]master_group_ip为空
#define		E_TRACK_EMPTY_MASTER_GROUP_PORT			0xE0091011//[flume]master_group_port为空
#define		E_TRACK_EMPTY_SLAVE_GROUP_IP			0xE0091012//[flume]slave_group_ip为空
#define		E_TRACK_EMPTY_SLAVE_GROUP_PORT			0xE0091013//[flume]slave_group_port为空
#define		E_TRACK_EMPTY_MASTER_MAJOR_IP			0xE0091014//[flume]master_major_ip为空
#define		E_TRACK_EMPTY_MASTER_MAJOR_PORT			0xE0091015//[flume]master_major_port为空
#define		E_TRACK_EMPTY_SLAVE_MAJOR_IP			0xE0091016//[flume]slave_major_ip为空
#define		E_TRACK_EMPTY_SLAVE_MAJOR_PORT			0xE0091017//[flume]slave_major_port为空
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_IP		0xE0091018//[flume]master_filter_id_ip为空
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_PORT		0xE0091019//[flume]master_filter_id_port为空
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_IP		0xE0091020//[flume]slave_filter_id_ip为空
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_PORT		0xE0091021//[flume]slave_filter_id_port为空
#define		E_TRACK_EMPTY_MTYPE						0xE0091022//[flume]mtype(请求的类型)为空
#define		E_TRACK_EMPTY_ADX						0xE0091023//[flume]adx(请求的渠道id)为空
#define		E_TRACK_EMPTY_MAPID						0xE0091024//[flume]mapid(创意id)为空
#define		E_TRACK_EMPTY_BID						0xE0091025//[flume]bid(请求id)为空
#define		E_TRACK_EMPTY_IMPID						0xE0091026//[flume]impid(广告展示机会id)为空
#define		E_TRACK_EMPTY_GETCONTEXTINFO_HANDLE		0xE0091027//[flume]去重redis句柄为空
#define		E_TRACK_EMPTY_PRICE						0xE0091028//[flume]赢价或展现通知价格为空
#define		E_TRACK_EMPTY_ADVERTISER				0xE0091029//[flume]advertiser(广告主)为空
#define		E_TRACK_EMPTY_DEVICEID					0xE0091030//[flume]deviceid(设备id)为空
#define		E_TRACK_EMPTY_DEVICEIDTYPE				0xE0091031//[flume]deviceidtype(设备id类型)为空
#define		E_TRACK_EMPTY_BID_PRICE					0xE0091032//[flume]缓存bid对应的价格为空
#define		E_TRACK_EMPTY_PRICE_CEILING				0xE0091033//[flume]价格上限为空
#define		E_TRACK_EMPTY_REMOTEADDR				0xE0091034//[flume]请求地址为空
#define		E_TRACK_EMPTY_REQUESTPARAM				0xE0091035//[flume]请求参数为空

#define		E_TRACK_MALLOC							0xE0092000//分配空间错误
#define		E_TRACK_MALLOC_PID						0xE0092001//[flume]为pid分配空间错误
#define		E_TRACK_MALLOC_DATATRANSFER_K			0xE0092002//[flume]为datatransfer_kafka分配空间错误
#define		E_TRACK_MALLOC_DATATRANSFER_F			0xE0092003//[flume]为datatransfer_flume分配空间错误
#define		E_TRACK_MALLOC_KAFKADATA				0xE0092004//[flume]为kafkadata分配空间错误

#define		E_TRACK_INVALID							0xE0093000//无效错误
#define		E_TRACK_INVALID_FLUME_LOGID				0xE0093001//[flume]无效的flume日志句柄
#define		E_TRACK_INVALID_KAFKA_LOGID				0xE0093002//[flume]无效的kafka日志句柄
#define		E_TRACK_INVALID_LOCAL_LOGID				0xE0093003//[flume]无效的local日志句柄
#define		E_TRACK_INVALID_MUTEX_IDFLAG			0xE0093004//[flume]无效的去重信息redis互斥锁
#define		E_TRACK_INVALID_MUTEX_USER				0xE0093005//[flume]无效的用户信息redis互斥锁
#define		E_TRACK_INVALID_MUTEX_GROUP				0xE0093006//[flume]无效的组信息redis互斥锁
#define		E_TRACK_INVALID_MONITORCONTEXT_HANDLE	0xE0093007//[flume]无效的监视器句柄
#define		E_TRACK_INVALID_DATATRANSFER_K			0xE0093008//[flume]无效的datatransfer_kafka
#define		E_TRACK_INVALID_DATATRANSFER_F			0xE0093009//[flume]无效的datatransfer_flume
#define		E_TRACK_INVALID_REQUEST_ADDRESS			0xE0093010//[flume]无效的请求ip
#define		E_TRACK_INVALID_REQUEST_PARAMETERS		0xE0093011//[flume]无效的请求参数集
#define		E_TRACK_INVALID_GETCONTEXTINFO_HANDLE	0xE0093012//[flume]无效的去重redis句柄
#define		E_TRACK_INVALID_PRICE_DECODE_HANDLE		0xE0093013//[flume]无效的价格解密句柄
#define		E_TRACK_INVALID_WINNOTICE				0xE0093014//[flume]无效的赢价通知(bid_flag value 为空)
#define		E_TRACK_INVALID_IMPNOTICE				0xE0093015//[kafka]无效的展现通知(bid_flag value 为空)
#define		E_TRACK_INVALID_CLKNOTICE				0xE0093016//[kafka]无效的点击通知(bid_flag value 为空)
	
#define		E_TRACK_UNDEFINE						0xE0094000//未定义错误
#define		E_TRACK_UNDEFINE_MAPID					0xE0094001//[flume]未定义的mapid(格式错误)
#define		E_TRACK_UNDEFINE_NRTB					0xE0094002//[flume]未定义的nrtb(值未定义)
#define		E_TRACK_UNDEFINE_AT						0xE0094003//[flume]未定义的at(值未定义)
#define		E_TRACK_UNDEFINE_MTYPE					0xE0094004//[flume]未定义的mtype(值未定义)
#define		E_TRACK_UNDEFINE_ADX					0xE0094005//[flume]未定义的adx(值未定义)
#define		E_TRACK_UNDEFINE_ADVERTISER				0xE0094006//[flume]未定义的advertiser(值未定义)
#define		E_TRACK_UNDEFINE_APPSID					0xE0094007//[flume]未定义的appsid(值未定义)
#define		E_TRACK_UNDEFINE_DEVICEID				0xE0094008//[flume]未定义的deviceid(长度超过40)
#define		E_TRACK_UNDEFINE_DEVICEIDTYPE			0xE0094009//[flume]未定义的deviceidtype(长度超过5)
#define		E_TRACK_UNDEFINE_PRICE_CEILING			0xE0094010//[flume]未定义的赢价价格上限(值>50元)

#define		E_TRACK_REPEATED						0xE0095000//重复错误
#define		E_TRACK_REPEATED_WINNOTICE				0xE0095001//[kafka]重复的赢价通知
#define		E_TRACK_REPEATED_IMPNOTICE				0xE0095002//[kafka]重复的展现通知
#define		E_TRACK_REPEATED_CLKNOTICE				0xE0095003//[kafka]重复的点击通知

#define		E_TRACK_FAILED							0xE0096000//失败错误
#define		E_TRACK_FAILED_DECODE_PRICE				0xE0096001//[flume]解析价格失败
#define		E_TRACK_FAILED_NORMALIZATION_PRICE		0xE0096002//[flume]归一化价格失败
#define		E_TRACK_FAILED_FCGX_ACCEPT_R			0xE0096003//[flume]fast-cgi框架接受连接失败
#define		E_TRACK_FAILED_SEND_FLUME				0xE0096004//[flume]发送flume失败
#define		E_TRACK_FAILED_SEND_KAFKA				0xE0096005//[flume]发送kafka失败
#define		E_TRACK_FAILED_FREQUENCYCAPPING			0xE0096006//[flume]展现或点击的frequency计数失败
#define		E_TRACK_FAILED_SEMAPHORE_WAIT			0xE0096007//[flume]semaphore_wait失败
#define		E_TRACK_FAILED_SEMAPHORE_RELEASE		0xE0096008//[flume]semaphore_release失败
#define		E_TRACK_FAILED_PARSE_DEVICEINFO			0xE0096009//[flume]解析deviceid和deviceidtype失败 

#define		E_TRACK_OTHER							0xE0097000//其他错误
#define		E_TRACK_OTHER_APP_MONITOR				0xE0097001//[flume]app下载相关监控
#define		E_TRACK_OTHER_UPPER_LIMIT_ALERT			0xE0097002//[flume]second price 超出上限警报

/////////////////////////////////////////////////////////////////////////////////////


#define		E_DL_GET_FUNCTION_FAIL					0xE0110000//结算部分的错误
#define		E_DL_OPEN_FAIL							0xE0111000//
#define		E_REPEATED_REQUEST						0xE0112000//重复请求
#define		E_DL_DECODE_FAILED						0xE0113000//价格解密失败

#endif
