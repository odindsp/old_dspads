#ifndef		ERRORCODE_H_
#define		ERRORCODE_H_

#define		E_SUCCESS								0x0     //�ɹ�

//��������
#define		E_UNKNOWN								0xE0000000  //[����]���ڳ����߼��϶࣬��������©֮�������˴�������ֹ��࣬����Ҫ��д����
#define		E_BID_PROGRAM							0xE0000001  //[����]��������������⣬һ�㲻�����
#define		E_BAD_REQUEST							0xE0001000  //[����]�������ʧ��
#define		E_MALLOC								0xE0002000  //[����]�����ڴ�ʧ�ܣ�һ�㲻�����
#define		E_SEMAPHORE								0xE0003000  //�ź�������ʧ��
#define		E_CREATETHREAD							0xE0004000  //�����߳�ʧ��
#define		E_MUTEX_INIT							0xE0005000  //
#define		E_PROFILE_INIT							0xE0006000  //�����ļ���ʼ��ʧ��
#define		E_PROFILE_INIT_MAKE						0xE0006010  //�����ļ���ʼ��ʧ��MAKE
#define		E_PROFILE_INIT_ADVCAT					0xE0006020  //�����ļ���ʼ��ʧ��ADVCAT
#define		E_PROFILE_INIT_APPCAT					0xE0006030  //�����ļ���ʼ��ʧ��APPCAT
#define		E_PROFILE_INIT_ADVID					0xE0006040  //�����ļ���ʼ��ʧ��ADVID
#define		E_PROFILE_INIT_CRE_ATTR					0xE0006060  //�����ļ���ʼ��ʧ��CRE ATTR (��������)
#define		E_PROFILE_INIT_GEOIPB					0xE0006070  //�����ļ���ʼ��ʧ��˽�е��������
#define		E_PROFILE_INIT_TEMPLATE					0xE0006080  //�����ļ���ʼ��ʧ�ܹ��ģ��
#define		E_LOG_SERVER							0xE0007000
#define		E_LOG_SERVER_HDFS						0xE0007001  //����������HDFS
#define		E_LOG_SERVER_BIDKAFKA					0xE0007002  //����������kafka
#define		E_LOG_SERVER_DETAIL						0xE0007100  //��������log������
#define		E_LOG_SERVER_DETAILKAFKA				0xE0007101  //��������log������kafka

#define		E_INVALID_PARAM							0xE0010000//������������
#define		E_INVALID_PARAM_LOGID					0xE0011000//��Ч����־�ļ�id
#define		E_INVALID_PARAM_LOGID_LOC				0xE0011010//��Ч��local��־�ļ�id
#define		E_INVALID_PARAM_LOGID_RESP				0xE0011020//��Ч��response��־�ļ�id
#define		E_INVALID_PARAM_ADX						0xE0012000//��Ч��adx
#define		E_INVALID_PARAM_CTX						0xE0013000//��Ч��ctxָ��
#define		E_INVALID_PARAM_REQUEST					0xE0014000//
#define		E_INVALID_PARAM_RESPONSE				0xE0015000//
#define		E_INVALID_PARAM_PADXTEMPLATE			0xE0016000//
#define		E_REQUEST_INVALID_BID_TYPE				0xE0017000//[����] ��Ч�ľ������ͣ�Ŀǰֻ��rtb��pmp���֣�
#define		E_INVALID_PARAM_JSON					0xE0018000
#define		E_INVALID_CPU_COUNT						0xE0019000

#define		E_FILE									0xE0020000//�ļ�����ʧ��
#define		E_INVALID_PROFILE						0xE0021000//�����ļ���ȡ����
#define		E_INVALID_PROFILE_IPB					0xE0021010//�����ļ���ȡ����
#define		E_INVALID_PROFILE_BIDIP					0xE0021020//�����ļ���ȡ���� ���;�����־IP
#define		E_INVALID_PROFILE_BIDPORT				0xE0021021//�����ļ���ȡ���� ���;�����־�˿�
#define		E_INVALID_PROFILE_BIDBROKER				0xE0021023//�����ļ���ȡ���� ���;�����־BROKER LIST(kafka��ʽ)
#define		E_INVALID_PROFILE_BIDTOPIC				0xE0021024//�����ļ���ȡ���� ���;�����־TOPIC(kafka��ʽ)
#define		E_INVALID_PROFILE_DETAILIP				0xE0021030//�����ļ���ȡ���� ���;���������־IP
#define		E_INVALID_PROFILE_DETAILPORT			0xE0021031//�����ļ���ȡ���� ���;���������־�˿�
#define		E_INVALID_PROFILE_DETAILBROKER			0xE0021032//�����ļ���ȡ���� ���;���������־BROKER LIST(kafka��ʽ)
#define		E_INVALID_PROFILE_DETAILTOPIC			0xE0021033//�����ļ���ȡ���� ���;���������־TOPIC(kafka��ʽ)
#define		E_FILENOTEXIST							0xE0022000//�ļ�������
#define		E_FOLDEREXIST							0xE0023000//Ŀ¼�Ѵ���
#define		E_FILE_IPB								0xE0024000//��IPB�ļ�ʧ��

#define		E_REDIS									0xE0030000//Redis����/���ʴ���
#define		E_REDIS_CONNECT_INVALID					0xE0031000//redis����ʧЧ
#define		E_REDIS_MASTER_MAJOR					0xE0031100//�޷�����master_major
#define		E_REDIS_SLAVE_MAJOR						0xE0031200//�޷�����slave_major
#define		E_REDIS_MASTER_FILTER_ID				0xE0031300//�޷�����master_filter_id
#define		E_REDIS_SLAVE_GR						0xE0031400//�޷�����slave_ur
#define		E_REDIS_SLAVE_UR						0xE0031500//�޷�����slave_ur
#define		E_REDIS_MASTER_PUBSUB					0xE0031600//�޷����Ӷ��Ķ˿�
#define		E_REDIS_MASTER_FILTER_SMART				0xE0031700//�޷�����master_filter_smart
#define		E_REDIS_CONNECT_R_INVALID				0xE0031800//��redis����ʧЧ
#define		E_REDIS_CONNECT_W_INVALID				0xE0031900//дredis����ʧЧ
#define		E_REDIS_ADVCAT							0xE0031A00//�޷����������Ƽ����redis�˿�
#define		E_REDIS_SLAVE_PRICE						0xE0031B00//�޷�����slave_price // 20170708: add
#define		E_REDIS_COMMAND							0xE0032000//ִ��redis����ʧ��
#define		E_REDIS_KEY_NOT_EXIST					0xE0032100//����key������
#define		E_REDIS_NO_CONTENT						0xE0033000//��Ӧkey������
#define		E_REDIS_MASTER_WBLIST					0xE0034000//�޷�����master_wblist
#define		E_REDIS_SLAVE_WBLIST					0xE0035000//�޷�����slave_wblist

#define		E_CACHE											0xE0040000//[����]Redis Cache ���ݲ�ƥ�䣨adxģ�����ģ����û��������Ӧadx��

#define		E_FILTER										0xE0050000//[����]���������У�û���ʺ�Ͷ�ŵĹ��
#define		E_FILTER_HAVE_NO_GROUP							0xE0051000//û��Ͷ�ŵ���Ŀ
#define		E_FILTER_HAVE_NO_AVAILABLE_GROUP				0xE0051100//û����Ч����Ŀ����ĿƵ�ζ��ﵽ��
#define		E_FILTER_HAVE_NO_CREATIVE						0xE0051200//û���ҵ����ϵĴ���
#define		E_FILTER_BCAT_OR_BADV_GREXT						0xE0052000//�豸���������������Ŀ�����й���飬ȫ��bcat����badv����ʧ��
#define		E_FILTER_BCAT									0xE0052100//[����group] bcat����ʧ��
#define		E_FILTER_BADV									0xE0052200//[����group] badv����ʧ��
#define		E_FILTER_GROUP_EXT								0xE0052300//[����group] ext����ʧ�ܣ�һ���������advid�͹�����advid�Ƿ�ƥ��
#define		E_FILTER_GROUP_BID								0xE0052400//[����group] Ͷ��ģʽ��ƥ�䣨PMP RTB��
#define		E_FILTER_GROUP_SECURE							0xE0052410//[����group] ��ȫģʽ��ƥ��
#define		E_FILTER_GROUP_PMP_DEALID						0xE0052500//[����group] PMP��Ŀdealid������
#define		E_FILTER_GROUP_WBL								0xE0053000//�豸�ڰ�������Ŀʧ��
#define		E_FILTER_GROUP_NOT_WL							0xE0053010//[����group] �豸ID���ڰ�����
#define		E_FILTER_GROUP_IN_BL							0xE0053020//[����group] �豸ID�ں�����
#define		E_FILTER_GROUP_ALLBL							0xE0053100//[����group] �豸id�ڣ��ܵģ���������
#define		E_REQUEST_PROCESS_CB							0xE0054000//[����]������adx�ص�������һ���Ǽ����ҵ�λ
#define		E_FILTER_TARGET									0xE0056000//�����豸�ڰ�������ͨͶ��Ŀ������ʧ��
#define		E_FILTER_TARGET_APP_ID							0xE0056110//[����group] appid�������ʧ��
#define		E_FILTER_TARGET_APP_CAT							0xE0056120//[����group] app cat�������ʧ��
#define		E_FILTER_TARGET_DEVICE							0xE0056200//[����group] �豸��������Ϊȫ����֧��
#define		E_FILTER_TARGET_DEVICE_REGIONCODE				0xE0056210//[����group] ������ʧ��
#define		E_FILTER_TARGET_DEVICE_CONNECTIONTYPE			0xE0056220//[����group] �����������Ͷ���ʧ��
#define		E_FILTER_TARGET_DEVICE_OS						0xE0056230//[����group] ����ϵͳ����ʧ��
#define		E_FILTER_TARGET_DEVICE_CARRIER					0xE0056240//[����group] ��Ӫ�̶���ʧ��
#define		E_FILTER_TARGET_DEVICE_DEVICETYPE				0xE0056250//[����group] ������Ͷ���ʧ��
#define		E_FILTER_TARGET_DEVICE_MAKE						0xE0056260//[����group] �����̶���ʧ��
#define		E_REQUEST_TARGET_ADX							0xE0056300//[����] adxƽ̨����ʧ��
#define		E_FILTER_TARGET_SCENE							0xE0056400//[����group] ��������ʧ��
#define		E_FILTER_FRC_GROUP								0xE0056500//[����group] ƽ��Ͷ�Ŵﵽ����
#define		E_FILTER_FRC									0xE0057000//[����creative] �û�Ƶ�δﵽ����
#define		E_FILTER_PRICE_LOW								0xE0058001//[����creative] ����ѡ��۸ߴ��⣬�۵��߱�����
#define		E_FILTER_PRICE_WLIST							0xE0058100//[����creative] ��������Ŀ�´���۸񲻷���
#define		E_FILTER_PRICE_CALLBACK							0xE0058200//[����creative] �ص�������鴴��۸񲻷���
#define		E_FILTER_EXTS									0xE0059000//[����creative] �����ext�ֶβ����ϣ�ĿǰΪû��д��˹��Ĵ���id��
#define		E_FILTER_IMP_UNSUPPORTED						0xE005A000//[����creative] ������Ͳ�֧�֣�֧��banner,video,native��
#define		E_FILTER_IMP_BANNER_SIZE						0xE005B100//[����creative] banner�ĳߴ粻����
#define		E_FILTER_IMP_BANNER_BTYPE						0xE005B200//[����creative] banner��洴�����Ͳ�����
#define		E_FILTER_IMP_BANNER_BATTR						0xE005B300//[����creative] banner������Բ�����
#define		E_FILTER_IMP_BANNER_TYPE						0xE005B400//[����creative] �ô��ⲻ��banner���
#define		E_FILTER_IMP_VIDEO_FTYPE						0xE005C100//[����creative] ��Ƶ�ഴ���ļ����Ͳ��ʺ�
#define		E_FILTER_IMP_VIDEO_TYPE							0xE005C200//[����creative] �ô��ⲻ��video���
#define		E_FILTER_IMP_VIDEO_SIZE							0xE005C300//[����creative] video���ߴ粻����
#define		E_REQUEST_IMP_NATIVE_LAYOUT						0xE005D100//[����] ԭ����沼����ʽ������Ϣ��
#define		E_REQUEST_IMP_NATIVE_PLCMTCNT					0xE005D200//[����] ԭ���������С��1��
#define		E_FILTER_IMP_NATIVE_BCTYPE						0xE005D300//[����creative] ԭ����������Ͳ�����
#define		E_REQUEST_IMP_NATIVE_ASSETCNT					0xE005D400//[����] ԭ�����Ԫ���б�̫�٣�������title and image��
#define		E_FILTER_IMP_NATIVE_ASSET_TYPE					0xE005D500//[����creative] ���Ԫ�����Ͳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_TITLE_LENGTH			0xE005D610//[����creative] title�ĳ��Ȳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_TYPE			0xE005D710//[����creative] image���Ԫ�ص����Ͳ����ϣ�ֻ��icon���ߴ�ͼ��
#define		E_REQUEST_NATIVE_ASSET_IMAGE_SIZE				0xE005D720//[����] image���Ԫ�صĳߴ�����0ֵ
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_SIZE		0xE005D731//[����creative] icon�ĳߴ粻����
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_FTYPE		0xE005D732//[����creative] icon��ͼƬ���Ͳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_ICON_URL		0xE005D733//[����creative] icon���زĵ�ַΪ��
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_SIZE		0xE005D741//[����creative] ��ͼ�ĳߴ粻����
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_FTYPE		0xE005D742//[����creative] ��ͼ��ͼƬ���Ͳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_MAIN_URL		0xE005D743//[����creative] ��ͼ���زĵ�ַΪ��
#define		E_FILTER_IMP_NATIVE_ASSET_IMAGE_NUM				0xE005D744//[����creative] �����ͼ�������봴���д�ͼ������ƥ��
#define		E_REQUEST_IMP_NATIVE_ASSET_DATA_TYPE			0xE005D810//[����] ��Ϣ����dataԪ�ص����Ͳ����ϣ�������rating��ctatext��
#define		E_REQUEST_IMP_NATIVE_ASSET_DATA_LENGTH			0xE005D820//[����] ��Ϣ���е��������Ȳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_DESC_LENGTH		0xE005D831//[����creative] �����������ĳ��Ȳ�����
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_RATING_VALUE		0xE005D841//[����creative] rating��ֵ����5
#define		E_FILTER_IMP_NATIVE_ASSET_DATA_CTATEXT			0xE005D842//[����creative] ctatextΪ��
#define		E_FILTER_IMP_NATIVE_TYPE						0xE005D900//[����creative] ������Ͳ�����Ϣ��
#define		E_REQUEST_IMP_INSTL_SIZE_NOT_FIND				0xE005E000//[����] banner������ ��adx��û���ҵ�֧�ֵĸóߴ�
#define		E_FILTER_ADVCAT									0xE005E100//[����creative] �����ҵ�����Ȳ�����ƥ��
#define		E_FILTER_LAST_RANDOM							0xE005E101//[����creative] ���ļ������ʵĴ����ֻ���ȡһ����ʣ��Ĳ���ѡ��
#define		E_FILTER_DEEPLINK								0xE005E102//[����creative] deeplink


#define		E_ADXTEMPLATE_NOT_FIND_ADX						0xE0061100//[���ڴ���] ģ����û�и�adx
#define		E_ADXTEMPLATE_NOT_FIND_IURL						0xE0061200//[���ڴ���] ģ����û��iurl
#define		E_ADXTEMPLATE_NOT_FIND_CTURL					0xE0061300//[���ڴ���] ģ����û��cturl
#define		E_ADXTEMPLATE_NOT_FIND_AURL						0xE0061400//[���ڴ���] ģ����û��aurl
#define		E_ADXTEMPLATE_NOT_FIND_NURL						0xE0061500//[���ڴ���] ģ����û��nurl
#define		E_ADXTEMPLATE_NOT_FIND_ADM						0xE0061600//[���ڴ���] ģ����û��adm
#define		E_ADXTEMPLATE_RATIO_ZERO						0xE0062000//[���ڴ���] ģ���еļ۸������0

#define		E_REDIS_GROUP_FC_INVALID						0xE0064000//��ĿƵ����Ϣ�Ƿ�
#define		E_REDIS_INVALID_CREATIVE_INFO					0xE0065001//����۸�Ϊ0��ܾ�Ͷ�� 
#define		E_REDIS_NO_CREATIVE								0xE0065002//��Ŀû��Ͷ�ŵĴ���

#define		E_REQUEST										0xE0070000//
#define		E_REQUEST_HTTP_METHOD							0xE0070001//[����] HTTP�������ԣ��е���Ҫ��POST
#define		E_REQUEST_HTTP_CONTENT_TYPE						0xE0070002//[����] HTTP CONTENT_TYPE ��ƥ��
#define		E_REQUEST_HTTP_CONTENT_LEN						0xE0070003//[����] HTTP CONTENT_LEN Ϊ0
#define		E_REQUEST_HTTP_BODY								0xE0070004//[����] HTTP BODY ����
#define		E_REQUEST_NO_BID								0xE0070006//[����] û�ҵ�bid
#define		E_REQUEST_HTTP_REMOTE_ADDR						0xE0070007//[����] HTTP REMOTE_ADDRû�ҵ�
#define		E_REQUEST_HTTP_OPENRTB_VERSION					0xE0070008//[����] HTTP OPENRTB_VERSIONû�ҵ�
#define		E_REQUEST_ID									0xE0071000//[����] ����bid������
#define		E_REQUEST_IS_PING								0xE0071001//[����] ������PING�����Ե����Ǹ�����������
#define		E_REQUEST_AT									0xE0071002//[����] �������Ͳ�֧��
#define		E_REQUEST_PMP_DEALID							0xE0071003//[����] PMPû�м۸�
#define		E_REQUEST_DEVICE								0xE0072000//[����] �����е��豸��Ϣ������
#define		E_REQUEST_DEVICE_OS								0xE0072100//[����] ����os����ios��android
#define		E_REQUEST_DEVICE_ID								0xE0072200//[����] û���豸id
#define		E_REQUEST_DEVICE_ID_DID_UNKNOWN					0xE0072210//[����] did���Ͳ�����
#define		E_REQUEST_DEVICE_ID_DID_IMEI					0xE0072211//[����] imei���Ϸ�
#define		E_REQUEST_DEVICE_ID_DID_MAC						0xE0072212//[����] mac���Ϸ�
#define		E_REQUEST_DEVICE_ID_DID_SHA1					0xE0072213//[����] did��sha1ֵ���Ϸ�
#define		E_REQUEST_DEVICE_ID_DID_MD5						0xE0072214//[����] did��md5ֵ���Ϸ�
#define		E_REQUEST_DEVICE_ID_DID_OTHER					0xE0072215//[����] did��other���ͳ���Ϊ0
#define		E_REQUEST_DEVICE_ID_DPID_UNKNOWN				0xE0072220//[����] dpid���Ͳ�����
#define		E_REQUEST_DEVICE_ID_DPID_ANDROIDID				0xE0072221//[����] android���Ϸ�
#define		E_REQUEST_DEVICE_ID_DPID_IDFA					0xE0072222//[����] idfa���Ϸ�
#define		E_REQUEST_DEVICE_ID_DPID_SHA1					0xE0072223//[����] dpid��sha1���Ϸ�
#define		E_REQUEST_DEVICE_ID_DPID_MD5					0xE0072224//[����] dpid��md5���Ϸ�
#define		E_REQUEST_DEVICE_ID_DPID_OTHER					0xE0072225//[����] dpid��other���ͳ���Ϊ0
#define		E_REQUEST_DEVICE_IP								0xE0072300//[����] ip��ַ���ڹ���
#define		E_REQUEST_DEVICE_TYPE							0xE0072400//[����] �豸���Ͳ���֧�֣�Ŀǰֻ֧���ֻ���ƽ��͵��ӣ�����adxֻ֧��һ�֣�
#define		E_REQUEST_DEVICE_MAKE_NIL						0xE0072501//[����] �豸make�ֶ�Ϊ��
#define		E_REQUEST_DEVICE_MAKE_OS						0xE0072502//[����] �豸make��os��һ��
#define		E_REQUEST_IMP_SIZE_ZERO							0xE0073000//[����] ����Ĺ��λΪ0
#define		E_REQUEST_IMP_NO_ID								0xE0073001//[����] ����Ĺ��λûID
#define		E_REQUEST_IMP_TYPE								0xE0073002//[����] ����Ĺ��λ���Ͳ���֧��
#define		E_REQUEST_IMP_NATIVE_ASSET_TITLE_LENGTH			0xE0073003//[����] ����Ĺ��λ��native�������ⳤ�Ȳ��Ϸ�
#define		E_REQUEST_IMP_NATIVE_ASSET_IMAGE_TYPE			0xE0073004//[����] ����Ĺ��λ��native��ͼƬ���Ͳ��Ϸ�
#define		E_REQUEST_IMP_NATIVE_ASSET_TYPE					0xE0073005//[����] ����Ĺ��λ��native��ASSET���Ͳ�֧��
#define		E_REQUEST_BANNER								0xE0073010//[����] ����Ĺ��λbanner��ȡ����ʧ��
#define		E_REQUEST_VIDEO									0xE0073011//[����] ����Ĺ��λvideo��ȡ����ʧ��
#define		E_REQUEST_NATIVE								0xE0073012//[����] ����Ĺ��λnative��ȡ����ʧ��
#define		E_REQUEST_VIDEO_ADTYPE_INVALID					0xE0073013//[����] �������Ƶ������Ͳ�֧��
#define		E_REQUEST_PRICING_RULE							0xE0073100//[����] �����м۸��ȡʧ��
#define		E_REQUEST_APP									0xE0074000//[����] �����APP��Ϣ������

#define		E_PV											0xE0080000//����pv�Ĵ���
#define		E_PV_INVALID_TIME								0xE0080100
#define		E_PV_INVALID_ID									0xE0081000//pv��id��Ч(���ҳid)
#define		E_PV_INVALID_TY									0xE0082000//pv��ty��Ч(��������)
#define		E_PV_INVALID_BIDMID								0xE0083000//pv��mid/bid��Ч(����id/���id)
#define		E_PV_INVALID_SYSDS								0xE0084000//pv��sys/ds��Ч(����ϵͳ/�豸�ֱ���)
#define		E_PV_INVALID_PVUV								0xE0085000//pv��pv/uv��Ч(չ����/�û���)
#define		E_PV_INVALID_PU									0xE0086000//pv��pu��Ч(��������)
#define		E_PV_INVALID_HF									0xE0087000//����ʱ,pv��hf��Ч(������ַ)
#define		E_PV_INVALID_AT									0xE0088000//������������ʱ,pv��at��Ч(ƽ������ʱ��)
#define		E_PV_INVALID_CL									0xE0089000//����ʱ,pv��cl��Ч(�ܷ�ҳ��)


////////////////////////////////////////////////////////////////////////////////////////////

#define		E_TRACK									0xE0090000//׷��ϵͳ�Ĵ���
#define		E_TRACK_EMPTY							0xE0091000//�մ���
#define		E_TRACK_EMPTY_CPU_COUNT					0xE0091001//[flume]cpu_countΪ��
#define		E_TRACK_EMPTY_FLUME_IP					0xE0091002//[flume]flume_ipΪ��
#define		E_TRACK_EMPTY_FLUME_PORT				0xE0091003//[flume]flume_portΪ��
#define		E_TRACK_EMPTY_KAFKA_TOPIC				0xE0091004//[flume]kafka_topicΪ��
#define		E_TRACK_EMPTY_KAFKA_BROKER_LIST			0xE0091005//[flume]kafka_broker_listΪ��
#define		E_TRACK_EMPTY_MASTER_UR_IP				0xE0091006//[flume]master_ur_ipΪ��
#define		E_TRACK_EMPTY_MASTER_UR_PORT			0xE0091007//[flume]master_ur_portΪ��
#define		E_TRACK_EMPTY_SLAVE_UR_IP				0xE0091008//[flume]slave_ur_ipΪ��
#define		E_TRACK_EMPTY_SLAVE_UR_PORT				0xE0091009//[flume]slave_ur_portΪ��
#define		E_TRACK_EMPTY_MASTER_GROUP_IP			0xE0091010//[flume]master_group_ipΪ��
#define		E_TRACK_EMPTY_MASTER_GROUP_PORT			0xE0091011//[flume]master_group_portΪ��
#define		E_TRACK_EMPTY_SLAVE_GROUP_IP			0xE0091012//[flume]slave_group_ipΪ��
#define		E_TRACK_EMPTY_SLAVE_GROUP_PORT			0xE0091013//[flume]slave_group_portΪ��
#define		E_TRACK_EMPTY_MASTER_MAJOR_IP			0xE0091014//[flume]master_major_ipΪ��
#define		E_TRACK_EMPTY_MASTER_MAJOR_PORT			0xE0091015//[flume]master_major_portΪ��
#define		E_TRACK_EMPTY_SLAVE_MAJOR_IP			0xE0091016//[flume]slave_major_ipΪ��
#define		E_TRACK_EMPTY_SLAVE_MAJOR_PORT			0xE0091017//[flume]slave_major_portΪ��
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_IP		0xE0091018//[flume]master_filter_id_ipΪ��
#define		E_TRACK_EMPTY_MASTER_FILTER_ID_PORT		0xE0091019//[flume]master_filter_id_portΪ��
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_IP		0xE0091020//[flume]slave_filter_id_ipΪ��
#define		E_TRACK_EMPTY_SLAVE_FILTER_ID_PORT		0xE0091021//[flume]slave_filter_id_portΪ��
#define		E_TRACK_EMPTY_MTYPE						0xE0091022//[flume]mtype(���������)Ϊ��
#define		E_TRACK_EMPTY_ADX						0xE0091023//[flume]adx(���������id)Ϊ��
#define		E_TRACK_EMPTY_MAPID						0xE0091024//[flume]mapid(����id)Ϊ��
#define		E_TRACK_EMPTY_BID						0xE0091025//[flume]bid(����id)Ϊ��
#define		E_TRACK_EMPTY_IMPID						0xE0091026//[flume]impid(���չʾ����id)Ϊ��
#define		E_TRACK_EMPTY_GETCONTEXTINFO_HANDLE		0xE0091027//[flume]ȥ��redis���Ϊ��
#define		E_TRACK_EMPTY_PRICE						0xE0091028//[flume]Ӯ�ۻ�չ��֪ͨ�۸�Ϊ��
#define		E_TRACK_EMPTY_ADVERTISER				0xE0091029//[flume]advertiser(�����)Ϊ��
#define		E_TRACK_EMPTY_DEVICEID					0xE0091030//[flume]deviceid(�豸id)Ϊ��
#define		E_TRACK_EMPTY_DEVICEIDTYPE				0xE0091031//[flume]deviceidtype(�豸id����)Ϊ��
#define		E_TRACK_EMPTY_BID_PRICE					0xE0091032//[flume]����bid��Ӧ�ļ۸�Ϊ��
#define		E_TRACK_EMPTY_PRICE_CEILING				0xE0091033//[flume]�۸�����Ϊ��
#define		E_TRACK_EMPTY_REMOTEADDR				0xE0091034//[flume]�����ַΪ��
#define		E_TRACK_EMPTY_REQUESTPARAM				0xE0091035//[flume]�������Ϊ��

#define		E_TRACK_MALLOC							0xE0092000//����ռ����
#define		E_TRACK_MALLOC_PID						0xE0092001//[flume]Ϊpid����ռ����
#define		E_TRACK_MALLOC_DATATRANSFER_K			0xE0092002//[flume]Ϊdatatransfer_kafka����ռ����
#define		E_TRACK_MALLOC_DATATRANSFER_F			0xE0092003//[flume]Ϊdatatransfer_flume����ռ����
#define		E_TRACK_MALLOC_KAFKADATA				0xE0092004//[flume]Ϊkafkadata����ռ����

#define		E_TRACK_INVALID							0xE0093000//��Ч����
#define		E_TRACK_INVALID_FLUME_LOGID				0xE0093001//[flume]��Ч��flume��־���
#define		E_TRACK_INVALID_KAFKA_LOGID				0xE0093002//[flume]��Ч��kafka��־���
#define		E_TRACK_INVALID_LOCAL_LOGID				0xE0093003//[flume]��Ч��local��־���
#define		E_TRACK_INVALID_MUTEX_IDFLAG			0xE0093004//[flume]��Ч��ȥ����Ϣredis������
#define		E_TRACK_INVALID_MUTEX_USER				0xE0093005//[flume]��Ч���û���Ϣredis������
#define		E_TRACK_INVALID_MUTEX_GROUP				0xE0093006//[flume]��Ч������Ϣredis������
#define		E_TRACK_INVALID_MONITORCONTEXT_HANDLE	0xE0093007//[flume]��Ч�ļ��������
#define		E_TRACK_INVALID_DATATRANSFER_K			0xE0093008//[flume]��Ч��datatransfer_kafka
#define		E_TRACK_INVALID_DATATRANSFER_F			0xE0093009//[flume]��Ч��datatransfer_flume
#define		E_TRACK_INVALID_REQUEST_ADDRESS			0xE0093010//[flume]��Ч������ip
#define		E_TRACK_INVALID_REQUEST_PARAMETERS		0xE0093011//[flume]��Ч�����������
#define		E_TRACK_INVALID_GETCONTEXTINFO_HANDLE	0xE0093012//[flume]��Ч��ȥ��redis���
#define		E_TRACK_INVALID_PRICE_DECODE_HANDLE		0xE0093013//[flume]��Ч�ļ۸���ܾ��
#define		E_TRACK_INVALID_WINNOTICE				0xE0093014//[flume]��Ч��Ӯ��֪ͨ(bid_flag value Ϊ��)
#define		E_TRACK_INVALID_IMPNOTICE				0xE0093015//[kafka]��Ч��չ��֪ͨ(bid_flag value Ϊ��)
#define		E_TRACK_INVALID_CLKNOTICE				0xE0093016//[kafka]��Ч�ĵ��֪ͨ(bid_flag value Ϊ��)
	
#define		E_TRACK_UNDEFINE						0xE0094000//δ�������
#define		E_TRACK_UNDEFINE_MAPID					0xE0094001//[flume]δ�����mapid(��ʽ����)
#define		E_TRACK_UNDEFINE_NRTB					0xE0094002//[flume]δ�����nrtb(ֵδ����)
#define		E_TRACK_UNDEFINE_AT						0xE0094003//[flume]δ�����at(ֵδ����)
#define		E_TRACK_UNDEFINE_MTYPE					0xE0094004//[flume]δ�����mtype(ֵδ����)
#define		E_TRACK_UNDEFINE_ADX					0xE0094005//[flume]δ�����adx(ֵδ����)
#define		E_TRACK_UNDEFINE_ADVERTISER				0xE0094006//[flume]δ�����advertiser(ֵδ����)
#define		E_TRACK_UNDEFINE_APPSID					0xE0094007//[flume]δ�����appsid(ֵδ����)
#define		E_TRACK_UNDEFINE_DEVICEID				0xE0094008//[flume]δ�����deviceid(���ȳ���40)
#define		E_TRACK_UNDEFINE_DEVICEIDTYPE			0xE0094009//[flume]δ�����deviceidtype(���ȳ���5)
#define		E_TRACK_UNDEFINE_PRICE_CEILING			0xE0094010//[flume]δ�����Ӯ�ۼ۸�����(ֵ>50Ԫ)

#define		E_TRACK_REPEATED						0xE0095000//�ظ�����
#define		E_TRACK_REPEATED_WINNOTICE				0xE0095001//[kafka]�ظ���Ӯ��֪ͨ
#define		E_TRACK_REPEATED_IMPNOTICE				0xE0095002//[kafka]�ظ���չ��֪ͨ
#define		E_TRACK_REPEATED_CLKNOTICE				0xE0095003//[kafka]�ظ��ĵ��֪ͨ

#define		E_TRACK_FAILED							0xE0096000//ʧ�ܴ���
#define		E_TRACK_FAILED_DECODE_PRICE				0xE0096001//[flume]�����۸�ʧ��
#define		E_TRACK_FAILED_NORMALIZATION_PRICE		0xE0096002//[flume]��һ���۸�ʧ��
#define		E_TRACK_FAILED_FCGX_ACCEPT_R			0xE0096003//[flume]fast-cgi��ܽ�������ʧ��
#define		E_TRACK_FAILED_SEND_FLUME				0xE0096004//[flume]����flumeʧ��
#define		E_TRACK_FAILED_SEND_KAFKA				0xE0096005//[flume]����kafkaʧ��
#define		E_TRACK_FAILED_FREQUENCYCAPPING			0xE0096006//[flume]չ�ֻ�����frequency����ʧ��
#define		E_TRACK_FAILED_SEMAPHORE_WAIT			0xE0096007//[flume]semaphore_waitʧ��
#define		E_TRACK_FAILED_SEMAPHORE_RELEASE		0xE0096008//[flume]semaphore_releaseʧ��
#define		E_TRACK_FAILED_PARSE_DEVICEINFO			0xE0096009//[flume]����deviceid��deviceidtypeʧ�� 

#define		E_TRACK_OTHER							0xE0097000//��������
#define		E_TRACK_OTHER_APP_MONITOR				0xE0097001//[flume]app������ؼ��
#define		E_TRACK_OTHER_UPPER_LIMIT_ALERT			0xE0097002//[flume]second price �������޾���

/////////////////////////////////////////////////////////////////////////////////////


#define		E_DL_GET_FUNCTION_FAIL					0xE0110000//���㲿�ֵĴ���
#define		E_DL_OPEN_FAIL							0xE0111000//
#define		E_REPEATED_REQUEST						0xE0112000//�ظ�����
#define		E_DL_DECODE_FAILED						0xE0113000//�۸����ʧ��

#endif
