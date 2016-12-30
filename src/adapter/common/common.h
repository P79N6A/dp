#ifndef COMMON_DEFINE_H
#define COMMON_DEFINE_H



#include <string>
#include <vector>

#include "poseidon_rtb.pb.h"


namespace poseidon
{

namespace adapter
{




const char kCtlReturn = '\n';
const char kFieldSep = '`';
const char kZero = '\0';
const char kCtrlA = '\001';
const char kCtrlB = '\002';
const char kCtrlC = '\003';
const char kDot = '.';
const std::string kSCtrlA = "\001";
const std::string kSCtrlB = "\002";
const std::string kSCtrlC = "\003";
const std::string kSCtrlD = "\004";
const std::string kSCtrlE = "\005";
const char kComma = ',';
const char kCross = 'x';
const char kUnderline = '_';
const std::string kSUnderline = "_";


enum MidHandlerIndex
{
    CREATIVE_TEMPLATE = 0,
    CONTENT_CATE_MAP ,
    CREATIVE_CATE_MAP ,
    CREATIVE_INVERT_CATE_MAP ,
    NATIVE_TEMPLATE_ID_MAP,
    NATIVE_TEMPLATE_ID_INVERT_MAP,
    NATIVE_META_INFO,
    ADZONE_INFO,
    DSP_CLICK_THROUGH,
    EF_PRE_URL,
    COPPER_KEY,
    COPPER_KEY_VER,
    NEED_AD_NUM,
    TRACK_CONF,
    AID_CONTROL,
    ADVERTISER_CONF,
    CIPHER_SECRET_KEY,
    SYSTEM_DEFAULT_ADS_INFO
};



enum AdzoneInfoContentIndex
{
    PUBLISHER_ID = 0,
    ADV_TYPE ,
    DISPLAY_TYPE ,
    PLATFORM_TYPE ,
    DEVICE_TYPE ,
    ADZ_LOCATION ,
    PUB_CATEGORY ,
    PAGE_CATEGORY ,
    HEIGHT_SIZE ,
    WIDTH_SIZE ,
    API_FRAMEWORK
};

enum AdvertisementType
{
    AT_TEXT = 1,
    AT_IMAGE ,
    AT_FLASH ,
    AT_VIDEO
};

enum DeviceType
{
    DT_PC = 1,
    DT_PHONE ,
    DT_TABLET,
    DT_TV ,
    DT_CONNECTED_DEVICE ,
    DT_SET_TOP_BOX
};

enum PlatformType
{
    PT_HTML = 1,
    PT_H5_WEB ,
    PT_H5_APP ,
    PT_NATIVE
};

enum ActionType
{
    ACT_DISPALY = 1,
    ACT_CLICK ,
    ACT_DOWNLOAD
};

const std::string PtArgsList[] = {"imp", "vt", "cb", "rsid", "sz", "num", "uid", "u", "r", "rp", "is_app",
                        "pvid", "br", "os", "osv", "nt", "lon",
                        "lat", "sr", "imei", "idfa", "mod",
                        "mac", "ip", "pkn", "op", "na", "lang",
                        "tz", "cd"};
enum PtArgsListIndex
{
    IMPRESSION_ID = 0,
    VIEW_TYPE ,
    JS_CALLBACK ,
    REF_SESSION_ID ,
    ADZ_SIZE ,
    AD_NUM ,
    USER_ID ,
    PAGE_URL ,
    REFER_URL ,
    REF_PID ,
    IS_APP ,
    PAGE_PVID ,
    BRAND ,
    OPERATING_SYSTEM ,
    OPERATING_SYSTEM_VER ,
    NETWORK_TYPE ,
    LONGITUDE ,
    LATITUDE ,
    SCREEN_RESOLUTION ,
    IMEI ,
    IDFA ,
    MODEL ,
    MAC ,
    IP_ADDR ,
    PACKAGE_NAME ,
    OPERATOR ,
    NATION ,
    LANGUAGE ,
    TIME_ZONE ,
    CUSTOM_DATA
};

const std::string MobileOSList[] = {"unknow", "ios", "android",
                                    "wp", "symbian", "blackberry"};
enum MobileOSListIndex
{
    OS_UNKNOW = 0,
    OS_IOS ,
    OS_ANDROID,
    OS_WP,
    OS_SYMBIAN,
    OS_BLACKBERRY
};

enum NetSecure
{
    NS_HTTP = 0,
    NS_HTTPS
};

enum NetworkType
{
    NT_UNKNOWN = 0,
    NT_ETHERNET ,
    NT_WIFI ,
    NT_UG ,
    NT_2G ,
    NT_3G ,
    NT_4G
};

enum PrivateAuction
{
    PA_UNLIMITED = 0,
    PA_SPECIFIC_DEALS
};

const std::string CurrencyCode[] = {"CNY", "USD", "EUR",
                                    "GBP", "HKD", "TWD", "JPY"};

enum CurrencyCodeIndex
{
    CHINA = 0,
    US ,
    EURO ,
    BRITAIN ,
    HONGKONG ,
    TAIWAN ,
    JAPAN
};

const std::string TanxPlatformList[] = {"iphone", "android", "ipad", "tablet"};
enum TanxPlatformListIndex
{
    TANX_PF_IPHONE = 0,
    TANX_PF_ANDROID ,
    TANX_PF_IPAD ,
    TANX_PF_TABLET
};

const std::string TanxOSList[] = {"ios", "android"};
enum TanxOSListIndex
{
    TANX_OS_IOS = 0,
    TANX_OS_ANDROID
};

enum CookieType
{
    CT_COOKIE = 1, // private own cookie
    CT_ACOOKIE     // ali-cookie
};


const std::string VideoMacros[] =
{
    "%%EF_URL%%",
    "%%RES_URL%%",
    "%%CLICK_THROUGH_URL%%",
    "%%CLICK_TRACK_URL%%",
    "%%HEIGHT%%",
    "%%WIDTH%%",
    "%%DURATION%%"
};

enum VideoMacrosIndex
{
    VM_EF = 0,
    VM_RES,
    VM_CLICK_THR,
    VM_CLICK_TRA,
    VM_HEIGHT,
    VM_WIDTH,
    VM_DURATION
};

const std::string AdmMacros[] =
{
    "%%ADM_FEEDBACK%%",
    "%%ADM_DOWNLOADED%%"
};

enum AdmMacrosIndex
{
    ADM_EF = 0,
    ADM_DL
};

enum AdvertiserConfigIndex
{
    ADVC_PILING = 0,
    ADVC_SOME
};

struct CmsContent
{
    std::string cookie; // raw cookie from adx or private site
    std::string acookie; // ali cookie
};

struct AidContent
{
    std::string imei; // imei number of android device
    std::string idfa; // idfa number of ios device
    std::string mac; // mac addr of electroic device
    std::string acookie; // ali cookie
    std::string aid; // ali-id
};

#define EF_MACRO "%%EF_URL%%"
#define TD_MACRO "%%TD_URL%%"
#define SRC_MACRO "%%CREATIVE_DATA%%"
//xxxx add:
#define EXPOSE_MACRO "%%EXPOSE_URL%%"

#define BID_REASON_SUCCESS 0
#define RTB_VERSION "1.0"

#define XOR_KEY 163
#define XOR_KEY_VER 1
#define DL_URL_PRE "http://ef.opendsp.tanx.com/downloaded?"
#define FB_URL_PRE "http://ef.opendsp.tanx.com/detail?"
#define TRD_URL_PRE "http://ef.opendsp.tanx.com/trd?"
#define HA_CONTROLER_NAME "controler"
#define HA_CONTROLER_TEST_NAME "controler_test"

//视频暂停、无线Native必须在投放前，通过接口上传审核。PCbanner、
//无线banner通常不需要预先上传审核。
/* //xxxx: 此viewtype移至rtb.proto 2016/08/12
enum TanxViewType
{
//两个维度：1是phone或pc  2是video(VIDEO_ADS)，Banner,...
    VT_FIXED         = 1, //固定
    VT_DOU_COUPLET   = 2, //双边对联
    VT_POP_WINDOW    = 5, //弹窗
    VT_POP_UNDER     = 7, //背投
    VT_VIDEO_ROLL    = 11, //视频贴片   //VIDEO_ADS | LINEAR_VIDEO_AD
    VT_VIDEO_PAUSE   = 12, //视频暂停   //VIDEO_ADS | NON_LINEAR_VIDEO_AD
    VT_HOVER         = 13, //悬停
    VT_SIN_COUPLET   = 14, // 单边对联
    VT_WL_FULL       = 101, //无线开屏 //xxxx add >= 101就属于MOBILE_DEVICE | OUTER_FEEDBACK
    VT_WL_POP_WINDOW = 102, //无线弹窗  //H5_TRAFFIC      |    OUTER_FEEDBACK | H5_TS xxxx add:插屏广告102
    VT_WL_BANNER     = 103, //无线横幅  //H5_TRAFFIC      |   OUTER_FEEDBACK | H5_TS
    VT_WL_WALL       = 104, //无线墙   //xxxx:NATIVE_TRAFFIC |  OUTER_FEEDBACK
    VT_WL_TEXT       = 105, //无线文字链 //NATIVE_TRAFFIC   |  OUTER_FEEDBACK
    VT_WL_VIDEO_ROLL = 106, //无线视频前贴片  //VIDEO_ADS | LINEAR_VIDEO_AD
    VT_WL_VIDEO_MIDLINEAR = 170,//xxxx add:无线视频中贴
    VT_WL_VIDEO_POSTLINEAR = 171,//xxxx add:无线视频后贴
    VT_WL_VIDEO_FULLSCN = 172,//xxxx add:无线视频全屏 //VIDEO_ADS | NON_LINEAR_VIDEO_AD
    VT_WL_VIDEO_PAUSE= 107, //无线视频暂停  //VIDEO_ADS  | NON_LINEAR_VIDEO_AD
    VT_WL_FEEDS      = 108, //无线Feeds流 //NATIVE_TRAFFIC | OUTER_FEEDBACK
    VT_WL_FOCUS      = 109, //无线焦点图  //NATIVE_TRAFFIC | OUTER_FEEDBACK
    VT_WL_EMBED_WALL = 110, //无线内嵌墙  //NATIVE_TRAFFIC | OUTER_FEEDBACK
    VT_WL_NATIVE     = 111, //无线Native  //NATIVE_TRAFFIC | OUTER_FEEDBACK
    VT_WL_FIXED_WEB  = 201, //固定(移动网页) //WAP_TS
    VT_WL_WEB_OVERLAY= 202, //浮窗(移动网页)  //
    VT_WL_ADM        = 501, // 无线H5自有流量直投
    VT_WL_MIX_APP    = 301,  //阿里游戏自有混合视频图片APP SDK
    //以下为优土专有
    VT_YT_MOBILE_APP_PAUSE = 401,//InAPP视频暂停
    VT_YT_MOBILE_APP_FULL = 402, //InAPP全屏
    VT_YT_MOBILE_APP_LINEAR_0 = 403,//InAPP前贴片
    VT_YT_MOBILE_APP_LINEAR_1 = 404,//InAPP中贴片
    VT_YT_MOBILE_APP_LINEAR_2 = 405,//InAPP后贴片
    VT_YT_MOBILE_APP_PIC = 406,//InAPP优酷移动播放页 图片广告
    VT_YT_MOBILE_SITE_PIC = 407,//移动WEB(M-W)优播页 图片广告
    VT_YT_PC_SITE_YOUKU = 408,//优酷PC页面播放页  图片广告
    VT_YT_PC_SITE_TUDOU = 409,//土豆PC页面播放页  图片广告
    VT_YT_PC_SITE_PAUSE = 410,//PC网页视频播放暂停广告
    VT_YT_PC_SITE_JIAOBIAO= 411,//PC网页视频角标
    VT_YT_PC_SITE_LINEAR_0 = 412,//PC网页视频前帖
    VT_YT_PC_SITE_LINEAR_1 = 413,//PC网页视频中帖
    VT_YT_PC_SITE_LINEAR_2 = 414, //PC网页视频后帖
    VT_YT_MOBILE_APP_NATIVE_100 = 415, //信息流100
    //以下为爱奇异专有
    VT_IQY_GEN_LINEAR_0 = 450, //前帖 不分PC/移动端
    VT_IQY_GEN_LINEAR_1 = 451, //中贴 不分PC/移动端
    VT_IQY_GEN_LINEAR_2 = 452, //后贴 不分PC/移动端
    VT_IQY_PC_PAUSE = 453,//暂停PC
    VT_IQY_MOBILE_PAUSE = 454,//暂停移动端/TV
    VT_IQY_MOBILE_JIAOBIAO = 455, //移动端角标
    VT_IQY_PC_JIAOBIAO = 456, //PC角标
    VT_IQY_PC_OVERLAY = 457//PC端通用Overlay
    //今日头条
    banner.pos指定了信息流，详情页，开屏类型。如下view_type包含在这三大类中
    VT_TT_FEED_LP_LARGE = 470, //头条信息流大图落地页，来源(限定10个字符),标题(25),落地页链接
    VT_TT_FEED_LP_SMALL = 471, //头条信息流小图落地页，来源(10),标题(25),落地页链接
    VT_TT_FEED_LP_GROUP = 472, //头条信息流组图落地页，来源(10),标题(25),落地页链接
    VT_TT_FEED_DL_LARGE = 473, //头条信息流大图应用下载，应用名(6), 描述(25),下载链接
    VT_TT_FEED_DL_SMALL = 474, //头条信息流小图应用下载，应用名(6), 描述(25),下载链接
    VT_TT_FEED_DL_TUANZI = 478, //段子（信息流）应用下载，应用名(6),标题(30),下载链接
    VT_TT_DETAIL_DL_BANNER = 475, //详情页 banner 应用下载，应用名(6),下载链接
    VT_TT_DETAIL_LP_BANNER = 476, //详情页 banner 落地页，来源(10),素材,落地页链接
    VT_TT_DETAIL_LP_TEXT_IMAGE = 477, //详情页大图文落地页，来源(10),标题(20),落地页链接
    VT_TT_VIDEO_PATCH = 479, //头条视频前贴【此 type 的广告已停止投放】来源（10），标题（25），视频地址，落地页链接
    VT_TT_SPLASH_LP = 480, //开屏广告静态图 标题（6-25），落地页链接
    VT_TT_SPLASH_NETWORK_LP = 481 //开屏联播【投放到外部合作媒体】标题（25），落地页链接
    //微博
    VT_WAX_BANNER = 505, //BANNER
    VT_WAX_FEED_NOR = 506,//普通feed
    VT_WAX_FEED_ACTIVITY = 507,//品牌大card
    VT_WAX_FEED_VIDEO = 508,//品速视频
    VT_WAX_FEED_GRID = 509, //9宫格
    //搜狐
    VT_SOHU_VIDEO_LINEAR_0=550,    //sohu视频前贴
    VT_SOHU_VIDEO_LINEAR_1=551,    //sohu视频中插
    VT_SOHU_VIDEO_LINEAR_2=552,    //sohu视频后贴
    VT_SOHU_VIDEO_PAUSE=553,       //sohu视频暂停
    VT_SOHU_BANNER_OPEN_PIC=554,   //sohu视频开机图
    VT_SOHU_BANNER_MIX=555,        //sohu图文混排
    VT_SOHU_BANNER_HP=556,         //sohu 首页通栏
    VT_SOHU_BANNER_HP_BRAND=557,   //sohu 首页品牌
    VT_SOHU_BANNER_SELF=558,       //sohu 我的频道页&搜索页
};
*/

#define IS_NATIVE_TRAFFIC(vt) (vt == rtb::VT_WL_WALL || vt == rtb::VT_WL_TEXT \
                               || vt == rtb::VT_WL_FEEDS || vt == rtb::VT_WL_FOCUS ||\
                                vt == rtb::VT_WL_EMBED_WALL || vt == rtb::VT_WL_NATIVE)
#define IS_H5_TRAFFIC(vt) (vt == rtb::VT_WL_BANNER || vt == rtb::VT_WL_POP_WINDOW)
#define IS_GOOGLE_TRAFF(ts) (ts == 3)
#define IS_VIDEO_ADS(tp) (tp == rtb::VT_VIDEO_ROLL ||tp == rtb::VT_VIDEO_PAUSE ||\
                          tp == rtb::VT_WL_VIDEO_ROLL || tp == rtb::VT_WL_VIDEO_PAUSE || \
                          tp == rtb::VT_WL_VIDEO_FULLSCN || tp == rtb::VT_WL_VIDEO_MIDLINEAR ||\
                          tp == rtb::VT_WL_VIDEO_POSTLINEAR)
#define IS_NON_LINEAR_VIDEO_AD(vt) (vt == rtb::VT_VIDEO_PAUSE || vt == rtb::VT_WL_VIDEO_PAUSE ||\
                                    vt == rtb::VT_WL_VIDEO_FULLSCN)
#define IS_LINEAR_VIDEO_AD(vt) (vt == rtb::VT_VIDEO_ROLL || vt == rtb::VT_WL_VIDEO_ROLL || \
                                vt == rtb::VT_WL_VIDEO_MIDLINEAR || vt == rtb::VT_WL_VIDEO_POSTLINEAR)
#define IS_OUTER_FEEDBACK(vt) ((vt >= rtb::VT_WL_FULL  && vt <= rtb::VT_WL_TEXT) ||\
                              (vt >= rtb::VT_WL_FEEDS && vt  <= rtb::VT_WL_NATIVE))
// google traffic, adx_type = 1
#define IS_GOOGLE_TS(ty) (ty == 1)
#define IS_H5_TS(ty) (ty == rtb::VT_WL_POP_WINDOW || ty == rtb::VT_WL_BANNER)
#define IS_MOBILE_DEVICE(vt) (vt >= rtb::VT_WL_FULL)
#define IS_WAP_TS(ty) (ty == rtb::VT_WL_FIXED_WEB || ty == rtb::VT_WL_WEB_OVERLAY)
//today toutiao
#define ADTYPE_APP_DOWNLOAD(type) ((type) == 4 || (type) == 3 || (type) == 9 || (type) == 7)
#define ADTYPE_SPLASH(type) ((type) == 13 || (type) == 14)
#define ADTYPE_DETAIL_BANNER(type) ((type) == 6 || (type) == 9)
#define ADTYPE_HAS_URLS(type) ((type) == 11 || (type) == 13 || (type) == 14)

#define IMPRESSON_MAX_AD_NUM 3
#define UDP_AMX_BUFF_LEN 65536

#define TANX_PRICE_MACRO  "%%SETTLE_PRICE%%"
#define DEFAULT_IMG_URL_KEY "img"
#define DEFAULT_CLICK_URL_KEY "click"
#define DEFAULT_CREATIVE_HEIGHT_KEY "height"
#define DEFAULT_CREATIVE_WIDTH_KEY "width"
#define DEFAULT_CREATIVE_DURATION "duration"
#define NATIVE_IMG_URL_KEY "img_url"
#define NATIVE_CLICK_URL_KEY "click_url"
#define NATIVE_LANDING_URL_KEY "landing_url"
#define NATIVE_DOWNLOAD_URL_KEY "download_url"
#define NATIVE_DEEP_LINK_KEY "deeplink_url"
#define NATIVE_TITLE_KEY "title"
#define NATIVE_LANDING_TYPE_KEY "landing_type"
#define NATIVE_TEMPLATE_REQUIRED "1"
#define NATIVE_TEMPLATE_NOT_REQUIRED "2"

#define NATIVE_TEMPLATE_MULTI "3"
#define ADVTISER_NEED_PILING "1"
#define TANX_VIDEO_PRE_ROLL 0
#define TANX_VIDEO_POST_ROLL -1
#define VIDEO_MIN_MACRO_NUM 3
#define LANDINT_TYPE_DOWNLOAD "1" //landing url鎵撳紑鍚庣洿鎺ヤ笅杞?
#define SME_DSP_ID "2605912368" //鍗冩枻椤禗SP ID
#define TEST_DSP_ID "2278700252" // 鍐掔儫DSP ID

//注意此url是http request的url。不是配置文件里feeback的url
#define TANX_REQUEST_URL "/"
#define TANX_REQUEST_URL_LENGTH (sizeof TANX_REQUEST_URL) - 1
#define YOUTU_REQUEST_URL "/adx/youtu/v1"
#define YOUTU_REQUEST_URL_LENGTH (sizeof YOUTU_REQUEST_URL) - 1
#define ALIGAME_REQUEST_URL "/adx/aligame/v1"
#define ALIGAME_REQUEST_URL_LENGTH (sizeof ALIGAME_REQUEST_URL) - 1
#define CHANCE_REQUEST_URL "/adx/changsi/v1"
#define CHANCE_REQUEST_URL_LENGTH (sizeof CHANCE_REQUEST_URL) - 1
#define YUNOS_REQUEST_URL "/adx/yunos/v1"
#define YUNOS_REQUEST_URL_LENGTH (sizeof YUNOS_REQUEST_URL) - 1
#define IQIYI_REQUEST_URL "/adx/iqiyi/v1"
#define IQIYI_REQUEST_URL_LENGTH (sizeof IQIYI_REQUEST_URL) - 1
#define TODAYTOUTIAO_REQUEST_URL "/adx/todaytoutiao/v1"
#define TODAYTOUTIAO_REQUEST_URL_LENGTH (sizeof TODAYTOUTIAO_REQUEST_URL) - 1
#define WAX_REQUEST_URL "/adx/wax/v1"
#define WAX_REQUEST_URL_LENGTH (sizeof WAX_REQUEST_URL) - 1
#define SOHU_REQUEST_URL "/adx/sohu/v1"
#define SOHU_REQUEST_URL_LENGTH (sizeof SOHU_REQUEST_URL) - 1

//net param
#define HTTP_MAX_HEAD_NAME_LENGTH    256
#define HTTP_MAX_HEAD_VALUE_LENGTH    1024
#define HTTP_MAX_BODY_LENGTH    1024 * 2048//2MB
#define HTTP_MAX_URL_LENGTH     1024
#define PROTOCOL_OBJECT_QUEUE_MAX_SIZE 5000

#define ATTR_ADAPTER_HTTP_INVALLID_HEAD    2213
#define ATTR_ADAPTER_HTTP_REQUEST_TIMEOUT  2354
#define ATTR_ADAPTER_HTTP_REQUEST_TOTAL    2353
#define ATTR_ADAPTER_INVALID_URL    2214
#define ATTR_ADAPTER_CHANNEL_INVALID_PROTOCOL 2215
#define ATTR_ADAPTER_CONTROL_REQ_TOTAL 2415
#define ATTR_ADAPTER_CONTROL_RESP_TOTAL 2416

#define ATTR_ADAPTER_ADX_REQUEST_TOTAL  96
#define ATTR_ADAPTER_CONTROLER_TIMEOUT  97
#define ATTR_ADAPTER_ADX_RESPON_TOTAL 2332
//tanx
#define ATTR_ADAPTER_TANX_REQUEST  88
#define ATTR_ADAPTER_TANX_EMPTY_RESPONSE  90
#define ATTR_ADAPTER_TANX_SUCC_RESPONSE  91
#define ATTR_ADAPTER_CONTROLER_TANX_REQUEST 2144
#define ATTR_ADAPTER_CONTROLER_TANX_RESPONSE  94
#define ATTR_ADAPTER_TANX_RESPON_NOT_CONNECTD 2334
//youtu
#define ATTR_ADAPTER_YOUTU_REQUEST  89
#define ATTR_ADAPTER_YOUTU_EMPTY_RESPONSE  92
#define ATTR_ADAPTER_YOUTU_SUCC_RESPONSE  93
#define ATTR_ADAPTER_CONTROLER_YOUTU_REQUEST 2145
#define ATTR_ADAPTER_CONTROLER_YOUTU_RESPONSE  95
#define ATTR_ADAPTER_YOUTU_RESPON_NOT_CONNECTD 2335
//aligame
#define ATTR_ADAPTER_ALIGAME_REQUEST    99
#define ATTR_ADAPTER_CONTROLER_ALIGAME_REQUEST 2146
#define ATTR_ADAPTER_CONTROLER_ALIGAME_RESPONSE 100
#define ATTR_ADAPTER_ALIGAME_EMPTY_RESPONSE 101
#define ATTR_ADAPTER_ALIGAME_SUCC_RESPONSE  102
#define ATTR_ADAPTER_ALIGAME_RESPON_NOT_CONNECTD 2336
//chance
#define ATTR_ADAPTER_CHANCE_REQUEST 103
#define ATTR_ADAPTER_CONTROLER_CHANCE_REQUEST 2147
#define ATTR_ADAPTER_CONTROLER_CHANCE_RESPONSE  104
#define ATTR_ADAPTER_CHANCE_EMPTY_RESPONSE  105
#define ATTR_ADAPTER_CHANCE_SUCC_RESPONSE   106
#define ATTR_ADAPTER_CHANCE_RESPON_NOT_CONNECTD 2337
//yunos
#define ATTR_ADAPTER_YUNOS_REQUEST 2148
#define ATTR_ADAPTER_CONTROLER_YUNOS_REQUEST 2152
#define ATTR_ADAPTER_CONTROLER_YUNOS_RESPONSE 2149
#define ATTR_ADAPTER_YUNOS_EMPTY_RESPONSE 2150
#define ATTR_ADAPTER_YUNOS_SUCC_RESPONSE 2151
#define ATTR_ADAPTER_YUNOS_RESPON_NOT_CONNECTD 2338

//iqiyi
#define ATTR_ADAPTER_IQIYI_REQUEST 2194
#define ATTR_ADAPTER_CONTROLER_IQIYI_REQUEST    2195
#define ATTR_ADAPTER_CONTROLER_IQIYI_RESPONSE   2196
#define ATTR_ADAPTER_IQIYI_EMPTY_RESPONSE       2197
#define ATTR_ADAPTER_IQIYI_SUCC_RESPONSE        2198
#define ATTR_ADAPTER_IQIYI_RESPON_NOT_CONNECTD  2339

//toutiao
#define ATTR_ADAPTER_TOUTIAO_REQUEST 2205
#define ATTR_ADAPTER_CONTROLER_TOUTIAO_REQUEST    2206
#define ATTR_ADAPTER_CONTROLER_TOUTIAO_RESPONSE   2207
#define ATTR_ADAPTER_TOUTIAO_EMPTY_RESPONSE       2208
#define ATTR_ADAPTER_TOUTIAO_SUCC_RESPONSE        2209
#define ATTR_ADAPTER_TOUTIAO_RESPON_NOT_CONNECTD  2340

//wax
#define ATTR_ADAPTER_WAX_REQUEST    2341
#define ATTR_ADAPTER_CONTROLER_WAX_REQUEST  2342
#define ATTR_ADAPTER_CONTROLER_WAX_RESPONSE 2343
#define ATTR_ADAPTER_WAX_EMPTY_RESPONSE     2344
#define ATTR_ADAPTER_WAX_SUCC_RESPONSE      2345
#define ATTR_ADAPTER_WAX_RESPON_NOT_CONNECTD    2346

//sohu
#define ATTR_ADAPTER_SOHU_REQUEST               2347
#define ATTR_ADAPTER_CONTROLER_SOHU_REQUEST     2348
#define ATTR_ADAPTER_CONTROLER_SOHU_RESPONSE    2349
#define ATTR_ADAPTER_SOHU_EMPTY_RESPONSE        2350
#define ATTR_ADAPTER_SOHU_SUCC_RESPONSE         2351
#define ATTR_ADAPTER_SOHU_RESPON_NOT_CONNECTD   2352

#define ATTR_ADAPTER_LATECY_LESS10      2189
#define ATTR_ADAPTER_LATECY_LESS30      2190
#define ATTR_ADAPTER_LATECY_LESS50      2191
#define ATTR_ADAPTER_LATECY_LESS100     2192
#define ATTR_ADAPTER_LATECY_GREAT100    2193

#define ASINT(OBJ) (atoi(OBJ.asString().c_str()))
#define SURE_CVALUE(rtb, obj) if(!obj.empty()) rtb(obj.asCString())

void adapter_replace_all(char *src, char word_to_be_find, char word_to_be_replace);

}
}

#endif
