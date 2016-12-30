#include "TanxAdapter.h"
#include "net/HttpRequest.h"
#include "common/common.h"
#include "poseidon_tanx.pb.h"
#include "poseidon_download.pb.h"
#include "poseidon_feedinfo.pb.h"
#include "poseidon_trdinfo.pb.h"
#include "utility/StringSplitTools.h"
#include "utility/rapidxml.hpp"
#include "utility/rapidxml_print.hpp"
#include "conf/Configer.h"

#include <vector>
#include <sstream>
#include <fcntl.h>

#include <boost/algorithm/string.hpp>
#include <muduo/base/Logging.h>
#include <muduo/base/LogStream.h>
#include <muduo/net/EventLoop.h>

/**
    auth: xxxx
    date: 2016/05
**/

using namespace std;
using namespace rapidxml;
using namespace poseidon;
using namespace poseidon::adapter;
using namespace poseidon::tanx;
using namespace poseidon::rtb;

extern Configer * volatile configer;



static CacheMap<int, int> g_CreativeCateCache;
static CacheMap<int, int> g_ContentCateCache;
static CacheMap<string, int> g_NativeTemplateIdCache;
///////////////////////////////////////////////////////////////////////////////////////////////////


TanxAdapter::TanxAdapter(MaMaPBObject *obj) :
    mama_pb_object_(obj)
{
    //ctor
}

TanxAdapter::~TanxAdapter()
{
    //dtor
}


int TanxAdapter::SetVideoAds(tanx::BidRequest_AdzInfo *pAdz,
                             rtb::Impression *pImp,
                             poseidon::tanx::BidRequest &request)
{
    return 0;
    rtb::Video *pRtbVideo = pImp->mutable_video();
    tanx::BidRequest_Video *pTanxVideo = request.mutable_video();
    // set video fomat, tanx nonsupport temporarily

    // set VideoLinearity
    if (IS_NON_LINEAR_VIDEO_AD(pAdz->view_type(0))) {
        pRtbVideo->set_linearity(rtb::VIDEO_LINEARITY_NON_LINEAR);
    } else if (IS_LINEAR_VIDEO_AD(pAdz->view_type(0))) {
        pRtbVideo->set_linearity(rtb::VIDEO_LINEARITY_LINEAR);
    }


    // set min_duration
    if (pTanxVideo->has_min_ad_duration()) {
        pRtbVideo->set_min_duration(pTanxVideo->min_ad_duration());
    }

    // set max_duration
    if (pTanxVideo->has_max_ad_duration()) {
        pRtbVideo->set_max_duration(pTanxVideo->max_ad_duration());
    }

    // set video protocol
    if (pTanxVideo->has_protocol()) {
        pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_30);
    }

    // set width of adz or player
    // set heigh of adz or player
    if (pAdz->has_size()) {
        vector<string> vSz;
        boost::split(vSz, pAdz->size(), boost::is_any_of("x"));
        if (vSz.size() == 2) {
            pRtbVideo->set_width(atoi(vSz[0].c_str()));
            pRtbVideo->set_height(atoi(vSz[1].c_str()));
        } else {
            LOG_ERROR << "size of video adzone parse errror!";
        }
    }
    // set start delay
    if (pTanxVideo->has_videoad_start_delay()) {
        if (TANX_VIDEO_PRE_ROLL ==
                pTanxVideo->videoad_start_delay()) {
            pRtbVideo->set_start_delay(rtb::VIDEO_START_DELAY_PRE_ROLL);
        } else if (TANX_VIDEO_POST_ROLL ==
                   pTanxVideo->videoad_start_delay()) {
            pRtbVideo->set_start_delay(rtb::VIDEO_START_DELAY_POST_ROLL);
        } else  {
            pRtbVideo->set_start_delay(rtb::VIDEO_START_DELAY_MID_ROLL);
        }
    }

    // set sequence, use of multi-impression, not support now
    // set blocked_creative_attributes, tanx nonsupport temporarily
    // set max_extended, tanx nonsupport temporarily

    // set min_bitrate and max_bitrate, tanx nonsupport temporarily
    // set boxing_allowed, tanx nonsupport temporarily
    // set playback_methods, tanx nonsupport temporarily
    // set deliveries, tanx nonsupport temporarily
    // set position, tanx nonsupport temporarily
    // set companion_ads, tanx nonsupport temporarily
    // set api
    for (int i = 0; i < pAdz->api_size(); ++i) {
        pRtbVideo->add_api(pAdz->api(i));
    }
    // set companion_type, tanx nonsupport temporarily

    // set ext of video
    rtb::Video_Ext* pRtbVideoExt = pRtbVideo->mutable_ext();
    // set section_start_delay
    if (pTanxVideo->has_videoad_section_start_delay()) {
        pRtbVideoExt->set_section_start_delay(pTanxVideo->videoad_section_start_delay());
    }
    // set blocked_creative_types
    for (int j = 0; j < pAdz->excluded_filter_size(); ++j) {
        pRtbVideoExt->add_blocked_creative_types(pAdz->excluded_filter(j));
    }

    // set content
    if (pTanxVideo->has_content()) {
        tanx::BidRequest_Video_Content* pTanxVideoContent = pTanxVideo->mutable_content();
        rtb::Content* pRtbVideoContent = pRtbVideoExt->mutable_content();
        if (pTanxVideoContent->has_title()) {
            pRtbVideoContent->set_title(pTanxVideoContent->title());
        }
        if (pTanxVideoContent->has_duration()) {
            pRtbVideoContent->set_len(pTanxVideoContent->duration());
        }
        for (int kInx = 0; kInx < pTanxVideoContent->keywords_size(); ++kInx) {
            pRtbVideoContent->add_keywords(pTanxVideoContent->keywords(kInx));
        }
    }

    //��鵱ǰ���λ�Ƿ������ض���������λ����������Ҫת��viewtype
    auto iter = MaMaPBObject::pid_viewtype_map_.find(pAdz->pid());
    if(iter != MaMaPBObject::pid_viewtype_map_.end() && rtb::ViewType_IsValid(iter->second)) {
        pImp->mutable_ext()->set_view_type(iter->second);
    }

    return 0;
}


int TanxAdapter::SetBanner(tanx::BidRequest_AdzInfo *pAdz, rtb::Impression *pImp)
{
    // banner
    rtb::Banner* pBan = pImp->mutable_banner();
    // set adzoneid
    pBan->set_id(pAdz->pid()); //xxxx: old tanxPid
    // set adzone size
    if (pAdz->has_size() && pAdz->size().length() > 0) {
        vector<string> sz;
        boost::split(sz, pAdz->size(), boost::is_any_of("x"));
        if ( 2 == sz.size() ) {
            pBan->set_width(atoi(sz[0].c_str()));
            pBan->set_height(atoi(sz[1].c_str()));
        } else {
            LOG_ERROR << "size of banner adzone parse errror!";
        }
    }
    // set position, tanx nonsupport temporarily
    // set blocked creative type , follow by tanx
    for (int j = 0; j < pAdz->excluded_filter_size(); ++j) {
        pBan->add_blocked_creative_types(pAdz->excluded_filter(j));
    }
    // set blocked creative attr, but tanx nonsupport temporarily
    // set creative allow mime type, but tanx nonsupport temporarily
    // set top frame, but tanx nonsupport temporarily
    // set expandable direction, but tanx nonsupport temporarily
    // set iframe busters, but tanx nonsupport temporarily
    // set api
    for (int k = 0; k < pAdz->api_size(); ++k) {
        pBan->add_api(pAdz->api(k));
    }
    return 0;
}

int TanxAdapter::SetPmP(int idx, tanx::BidRequest_AdzInfo *pAdz,
                        rtb::Impression *pImp,
                        poseidon::tanx::BidRequest &request)
{
    // set PMP
    if (idx < request.deals_size()) {
        rtb::PMP* pPmp = pImp->mutable_pmp();
        // set private auction, 1 only specific deals 0 unlimited
        pPmp->set_private_auction(PA_SPECIFIC_DEALS);
        // set deal and preferred deal
        for(int n = 0; n < request.deals_size(); ++n) {
            tanx::BidRequest_Deal* pDeal = request.mutable_deals(n);//���׶���deal
            if (pDeal->has_priv_auc()) { //˽�о���
                tanx::BidRequest_Deal_PrivateAuction* pPriAuc = pDeal->mutable_priv_auc();
                rtb::PMP_Deal* pPmpDeal = pPmp->add_deals();
                char deal_id[32];
                snprintf(deal_id, sizeof(deal_id), "%u", pPriAuc->dealid());
                pPmpDeal->set_id(deal_id);
                if (0 < pPriAuc->buyer_rules_size()) {
                    tanx::BidRequest_Deal_PrivateAuction_BuyerRule* pRule = pPriAuc->mutable_buyer_rules(0);
                    if(pRule->has_min_cpm_price()) {
                        pPmpDeal->set_bidfloor(pRule->min_cpm_price());
                    }
                    for(int i = 0; i < pRule->advertiser_ids_size(); ++i) {
                        stringstream sAdIds;
                        string idTemp;
                        sAdIds << pRule->advertiser_ids(i);
                        sAdIds >> idTemp;
                        pPmpDeal->add_wseat(idTemp);
                    }
                    pPmpDeal->set_bidfloorcur(CurrencyCode[CHINA]);
                    if(1 < pPriAuc->buyer_rules_size()) {
                        LOG_WARN << "buyer rutanx::es of deal is more than 1, and the real size is" << pPriAuc->buyer_rules_size();
                    }
                }
            }
            if (pDeal->has_prefer_deal()) { //���Ƚ���
                tanx::BidRequest_Deal_PreferredDeal* pPerferDeal = pDeal->mutable_prefer_deal();
                rtb::PMP_Ext* pPmpExt = pPmp->mutable_ext();
                rtb::PMP_Ext_PreferredDeal* pPEPreferDeal = pPmpExt->add_preferred_deals();
                pPEPreferDeal->set_dealid(pPerferDeal->dealid());
                if(pPerferDeal->has_fix_cpm_price()) {
                    pPEPreferDeal->set_fix_cpm_price(pPerferDeal->fix_cpm_price());
                }

                for(int j = 0; j < pPerferDeal->advertiser_ids_size(); ++j) {
                    pPEPreferDeal->add_advertiser_ids(pPerferDeal->advertiser_ids(j));
                }
                stringstream ssDealId;
                ssDealId << pPerferDeal->dealid();
                const string& sKey = ssDealId.str();
                /*
                string sValue;
                if (m_pSysDefaultAds->fetch(sKey, sValue)) //xxxx TODO
                {
                    mini_request.need_sys_default_ads = true;
                    //m_dealId = sKey; //xxxx TODO
                    LOG_DEBUG << "this deal should need system default ads, deal_id:" << sKey;
                }
                else
                {
                    LOG_DEBUG << "this deal not need system default ads, deal_id:" << sKey;
                }
                */
            }
        }
    }
    return 0;
}




int TanxAdapter::SetExcludSenCateg(rtb::Impression_Ext *pImExt, poseidon::tanx::BidRequest &request)
{
    for (int i = 0;
            i < request.excluded_sensitive_category_size(); ++i) {
        int creativeCate = request.excluded_sensitive_category(i);
        pImExt->add_excluded_sensitive_category(creativeCate);//xxxx modify
        /*vector<int> mappedCates;
        if (g_CreativeCateCache.find(creativeCate, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                    it != mappedCates.end(); ++it) {
                pImExt->add_excluded_sensitive_category(*it);
            }
            LOG_DEBUG << "hit CreativeCateCache, using cache data, at setting excluded sensitive categories";
        }
        else {
            stringstream ssCate;
            string cat;
            string mapCat;
            ssCate << creativeCate;
            ssCate >> cat;//xxxx:creativeCate(int) -> cat (string)
            if (m_pCreativeCateMap->fetch(cat, mapCat)) {
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapCat, kCtrlA, res);

                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it)
                {
                    int catVal = atoi((*it).c_str());
                    pImExt->add_excluded_sensitive_category(catVal);
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_CreativeCateCache.store(creativeCate, cacheValue);
                LOG_DEBUG << "miss CreativeCateCache, using tdbm mapping data, at setting excluded sensitive categories";
            }
            else {
                LOG_WARN << "fetch mapping creative category failed at tanx adapter when set excluded sensitive categories, and the search key is:" << creativeCate;
            }
        }*/
    }
    return 0;
}

int TanxAdapter::SetExcludAdCateg(rtb::Impression_Ext *pImExt, poseidon::tanx::BidRequest &request)
{
    // set excluded ad category
    for (int i = 0;
            i < request.excluded_ad_category_size(); ++i) {
        int creativeCate = request.excluded_ad_category(i);
        pImExt->add_excluded_ad_category(creativeCate);//xxxx modify
        /*vector<int> mappedCates;
        if (g_CreativeCateCache.find(creativeCate, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                    it != mappedCates.end(); ++it) {
                pImExt->add_excluded_ad_category(*it);
            }
            LOG_DEBUG << "hit CreativeCateCache, using cache data, at setting excluded ad categories";
        }
        else {
            stringstream ssCate;
            string cat;
            string mapCat;
            ssCate << creativeCate;
            ssCate >> cat;
            if (m_pCreativeCateMap->fetch(cat, mapCat)) {
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapCat, kCtrlA, res);

                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it)
                {
                    int catVal = atoi((*it).c_str());
                    pImExt->add_excluded_ad_category(catVal);
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_CreativeCateCache.store(creativeCate, cacheValue);
                LOG_DEBUG << "miss CreativeCateCache, using tdbm mapping data, at setting excluded ad categories";
            }
            else {
                LOG_WARN << "fetch mapping creative category failed at tanx adapter when set excluded ad categories, and the search key is:" << creativeCate;
            }
        }*/
    }
    return 0;
}

//�ƹ�λ��Ϣ
int TanxAdapter::RtbAdzInfo(int index, tanx::BidRequest_AdzInfo *pAdz,
                            rtb::Impression *pImp,
                            poseidon::tanx::BidRequest &request)
{
    // set ext of impression
    rtb::Impression_Ext* pImExt = pImp->mutable_ext();//����Impression::Ext�ֶ�
    // set view type ���video���ܻ����view_type
    if(rtb::ViewType_IsValid(pAdz->view_type(0))) {
        pImExt->set_view_type(pAdz->view_type(0));    //չ������
    } else {
        LOG_ERROR << "Invalid view_type:" << pAdz->view_type(0);
    }

    // set video
    //�ж���video��������ʹ��tanx::video����rtb::BidRequest::Impression::video�ֶ�
    if ( IS_VIDEO_ADS(pAdz->view_type(0))) {
        SetVideoAds(pAdz, pImp, request);
    } else  {
        SetBanner(pAdz, pImp);
    }

    // set display_manage, but tanx nonsupport temporarily
    // set display_manager_ver, but tanx nonsupport temporarily
    // set interstitial, but tanx nonsupport temporarily
    // set special tag id, but tanx nonsupport temporarily
    // set secure, 0 http, 1 https
    pImp->set_secure(NS_HTTP);//
    SetPmP(index, pAdz, pImp, request);//����Impression::pmp�ֶ�
    // set CPM of bidfloorcur
    pImp->set_bidfloor(pAdz->min_cpm_price());//��͵�CPM����
    // specified currencies
    pImp->set_bidfloorcur(CurrencyCode[CHINA]);//���Ҵ���


    // set ad num, tanx adx default is 2, but not adapte every dsp
    pImExt->set_ad_num(
        (pAdz->ad_bid_count() > IMPRESSON_MAX_AD_NUM) ?
        IMPRESSON_MAX_AD_NUM : pAdz->ad_bid_count());
    // set view_screen
    if (pAdz->has_view_screen()) {//�ƹ�λ��ҳ�����ڵ�����
        pImExt->set_view_screen(static_cast<int>(pAdz->view_screen()));
    }
    for (int uInx = 0;
            uInx < request.excluded_click_through_url_size(); ++uInx) {
        pImExt->add_excluded_click_through_url(request.excluded_click_through_url(uInx));
    }
    // set excluded sensitive category
    SetExcludSenCateg(pImExt, request);
    SetExcludAdCateg(pImExt, request);


    // set allowed_creative_level
    if (pAdz->has_allowed_creative_level()) {
        pImExt->set_allowed_creative_level(pAdz->allowed_creative_level());
    }
    return 0;
}


int TanxAdapter::RtbMobileInfo(poseidon::tanx::BidRequest &request, rtb::App *pApp, std::string &publisherId)
{
    tanx::BidRequest_Mobile *pMobile = request.mutable_mobile();
    // set app id, but tanx is empty temporarily

    // set app name, but tanx is empty temporarily
    if (pMobile->has_app_name()) {
        pApp->set_name(pMobile->app_name());
    }

    // set app categories
    for(int acIdx = 0; acIdx < pMobile->app_categories_size(); ++acIdx) {
        tanx::BidRequest_Mobile_AppCategory* pAppCat = pMobile->mutable_app_categories(acIdx);
        int appCate = pAppCat->id();
        pApp->add_app_categories(appCate);//xxxx modify
        /*vector<int> mappedCates;
        if (g_ContentCateCache.find(appCate, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                    it != mappedCates.end(); ++it) {
                pApp->add_app_categories(*it);
            }
            LOG_DEBUG << "hit ContentCateCache, using cache data, at setting app categories";
        }
        else {

            stringstream ssCate;
            string cat;
            string mapCat;
            ssCate << appCate;
            ssCate >> cat;
            if (m_pContentCateMap->fetch(cat, mapCat)) {
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapCat, kCtrlA, res);

                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it) {
                    int catVal = atoi((*it).c_str());
                    pApp->add_app_categories(catVal);
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_ContentCateCache.store(appCate, cacheValue);
                LOG_DEBUG << "miss ContentCateCache, using tdbm mapping data, at setting app categories";
            }
            else {
                //xxxx commment TODO?
                //LOG_WARN << "fetch mapping app category failed at tanx adapter, and the search key:" <<
                //        appCate << ", impid:" << tanxPid.c_str();
            }
        }*/
    }
    // set section categories, but tanx is empty temporarily

    // set page/view categories
    for (int cc = 0; cc < request.content_categories_size(); ++cc) {
        tanx::BidRequest_ContentCategory* pPc = request.mutable_content_categories(cc);
        int pc_id = pPc->id();
        pApp->add_page_categories(pc_id);
        /*vector<int> mappedCates;
        if (g_ContentCateCache.find(pc_id, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                      it != mappedCates.end(); ++it) {
                pApp->add_page_categories(*it);
            }
            LOG_DEBUG << "hit ContentCateCache, using cache data at setting app page category";
        }
        else {
            stringstream ssId;
            string id;
            string mapId;
            ssId << pc_id;
            ssId >> id;
            if (m_pContentCateMap->fetch(id, mapId)) {//TODO
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapId, kCtrlA, res);
                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it) {
                    int catVal = atoi((*it).c_str());
                    pApp->add_page_categories(catVal);
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_ContentCateCache.store(pc_id, cacheValue);
                LOG_DEBUG << "miss ContentCateCache, using tdbm mapping data, at setting app page categories";
            }
            else  {
                //xxxx comment TODO?
                //LOG_WARN << "fetch mapping app page category failed at tanx adapter, and search key is:" <<
                //        pc_id <<", pid:" << tanxPid.c_str();
            }
        }*/
    }
    // set app version, but tanx is empty temporarily
    // set app bundle(package name)
    if (pMobile->has_package_name()) {
        pApp->set_bundle(pMobile->package_name());
    }
    // set app privacy policy, but tanx is empty temporarily
    // set app whether to pay, but tanx is empty temporarily
    // set publisher, only publisher id
    if (!publisherId.empty()) {
        rtb::Publisher* pPub = pApp->mutable_publisher();
        pPub->set_id(publisherId);
    }
    // set content, mainly video content, opendsp do not support at p1
    // set keywords
    for (int kws = 0; kws < pMobile->ad_keyword_size(); ++kws) {
        pApp->add_keywords(pMobile->ad_keyword(kws));
    }
    // set app stroe url, but tanx is empty temporarily

    // set Ext info: native_template_ids and landing_types
    //xxxx: nativeģ����Ϣ��
    rtb::App_Ext* pAppExt = NULL;
    for (int ntIdx = 0; ntIdx < pMobile->native_template_id_size(); ++ntIdx) {
        if (NULL == pAppExt) {
            pAppExt = pApp->mutable_ext();
        }
        // native template id mapping
        string tanxNativeTempId = pMobile->native_template_id(ntIdx);
        // check native_template_id for new template serial number, old demo: 1.x.x.x
        if (tanxNativeTempId.compare(0,2,"1.") == 0) {
            LOG_DEBUG << "old template serial number, so mapping job should be skipped";
            continue;
        }
        //xxxx modify:
        pAppExt->add_native_template_ids(atoi(tanxNativeTempId.c_str()));//xxxx modify


        /*vector<int> mappedTempId;
        if (g_NativeTemplateIdCache.find(tanxNativeTempId, mappedTempId)) {
            for (vector<int>::iterator it = mappedTempId.begin();
                    it != mappedTempId.end(); ++it) {
                pAppExt->add_native_template_ids(*it);
            }
            LOG_DEBUG << "hit NativeTemplateIdCache, using cache data at setting native_template_ids of app Ext";
        }
        else {
            string sMappedTempId;
            if (m_pNativeTemplateIdMap->fetch(tanxNativeTempId, sMappedTempId)) {//TODO
                int id = atoi(sMappedTempId.c_str());
                pAppExt->add_native_template_ids(id);
                vector<int> cacheValue;
                cacheValue.push_back(id);
                // store mapping data to cache
                g_NativeTemplateIdCache.store(tanxNativeTempId, cacheValue);
                LOG_DEBUG << "miss NativeTemplateIdCache, using tdbm mapping data at setting app categories";
            }
            else {
                //LOG_WARN << "fetch mapping native template id at tanx adapter, and the search key:" <<
                //        tanxNativeTempId << ", impid:" << tanxPid << ", so it will drop it";
                continue;
            }
        }*/
    }


    //xxxx add ���£�
    for(int i = 0; i < pMobile->native_ad_template_size(); ++i) {
        //�����±�ȡ������native_ad_template
        tanx::BidRequest_Mobile_NativeAdTemplate *tanx_native_template = pMobile->mutable_native_ad_template(i);
        //RTB�������Ӧ��������һ������
        rtb::App_Ext_NativeAdTemplate *rtb_native_template = pAppExt->add_native_ad_template();
        //native_templateԪ������ģ��id
        rtb_native_template->set_native_template_id(atoi(tanx_native_template->native_template_id().c_str()));
        //ѭ��tanx��������Ԫ��native_template�е�area����
        for(int j = 0; j < tanx_native_template->areas_size(); ++j) {
            //�����±�ȡ��tanx�е�area
            tanx::BidRequest_Mobile_NativeAdTemplate_Area *tanx_area = tanx_native_template->mutable_areas(j);
            tanx::BidRequest_Mobile_NativeAdTemplate_Area_Creative *tanx_creative = tanx_area->mutable_creative();

            rtb_native_template->set_area_id(tanx_area->id());
            //rtb_native_template->set_native_template_id(atoi(tanx_native_template->native_template_id().c_str()));

            vector<string> vSz;
            boost::split(vSz, tanx_creative->image_size(), boost::is_any_of("x"));
            if (vSz.size() == 2) {
                rtb_native_template->set_w(atoi(vSz[0].c_str()));
                rtb_native_template->set_h(atoi(vSz[1].c_str()));
            } else {
                LOG_ERROR << "native_template image_size field has no value!";
            }
        }
    }

    for (int tpIdx = 0; tpIdx < pMobile->landing_type_size(); ++tpIdx) {
        if (NULL == pAppExt) {
            pAppExt = pApp->mutable_ext();
        }
        pAppExt->add_landing_types(pMobile->landing_type(tpIdx));
    }
    return 0;
}


int TanxAdapter::RtbSiteInfo(poseidon::tanx::BidRequest &request,
                             rtb::Site* pSite,
                             const std::string &publisherId,
                             const std::string &tanxid)
{
    // set site id
    vector<string> vPid;
    boost::split(vPid, tanxid, boost::is_any_of("_"));
    if(vPid.size() == 4) {
        pSite->set_site_id(vPid[2]);
    }
    // set site name, but tanx nonsupport temporarily

    // set site domain, but tanx nonsupport temporarily

    // set site categories, using tanx site categories
    if (request.has_category()) {
        int siteCate = static_cast<int>(request.category());
        pSite->add_site_categories(siteCate);//xxxx modify
        /*vector<int> mappedCates;
        if (g_ContentCateCache.find(siteCate, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                    it != mappedCates.end(); ++it) {
                pSite->add_site_categories(*it);
            }
            LOG_DEBUG << "hit ContentCateCache, using cache data, at setting site categories";
        }
        else {
            stringstream ssCate;
            string cat;
            string mapCat;
            ssCate << siteCate;
            ssCate >> cat;
            if (m_pContentCateMap->fetch(cat, mapCat)) { //TODO:
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapCat, kCtrlA, res);

                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it) {
                    int catVal = atoi((*it).c_str());
                    pSite->add_site_categories(catVal);
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_ContentCateCache.store(siteCate, cacheValue);
                LOG_DEBUG << "miss ContentCateCache, using tdbm mapping data, at setting site categories";
            }
            else {
                LOG_WARN << "fetch mapping site category failed at tanx adapter, and the search key is" <<
                        siteCate << ", impid:" << tanxPid;
            }
        }*/
    }
    // set page content cata, but tanx nonsupport temporarily
    // set sectioncat, but tanx nonsupport temporarily
    // set page url
    if (request.has_url()) {
        pSite->set_page(request.url());
    }
    // set privacy policy, but tanx nonsupport temporarily
    // set refer url, but tanx nonsupport temporarily
    // set search string, but tanx nonsupport temporarily
    // set publisher, only publisher id
    if (!publisherId.empty()) { //TODO
        rtb::Publisher* pPub = pSite->mutable_publisher();
        pPub->set_id(publisherId);
    }
    // set content, mainly video content, opendsp do not support at p1



    // set ext
    for (int cc = 0; cc < request.content_categories_size(); ++cc) {
        rtb::Site_Ext* pSiteExt = pSite->mutable_ext();
        tanx::BidRequest_ContentCategory* pPc = request.mutable_content_categories(cc);
        int pc_id = pPc->id();
        pSite->add_page_categories(pc_id);//xxxx modify
        rtb::Site_Ext_PageCategory* pPageCate = pSiteExt->add_page_category();//xxxx modify
        //��ҳ��ĿID
        pPageCate->set_id(pc_id);//xxxx modify
        //��Ӧ��ҳ��ĿID�����ŷ���confidence_levelȡֵ��Χ[0,1000]
        pPageCate->set_weight(pPc->confidence_level());  //xxxx modify
        // set page category
        /*
        vector<int> mappedCates;
        if (g_ContentCateCache.find(pc_id, mappedCates)) {
            for (vector<int>::iterator it = mappedCates.begin();
                    it != mappedCates.end(); ++it) {
                pSite->add_page_categories(*it);
                rtb::Site_Ext_PageCategory* pPageCate = pSiteExt->add_page_category();
                pPageCate->set_id(*it);
                pPageCate->set_weight(pPc->confidence_level());
            }
            LOG_DEBUG << "hit ContentCateCache, using cache data at setting page category";
        }
        else
        {
            stringstream ssId;
            string id;
            string mapId;
            ssId << pc_id;
            ssId >> id;
            if (m_pContentCateMap->fetch(id, mapId)) //TODO:
            {
                vector<string> res;
                vector<int> cacheValue;
                StringSplitTools::splitString(mapId, kCtrlA, res);
                for (vector<string>::iterator it = res.begin();
                        it != res.end(); ++it)
                {
                    int catVal = atoi((*it).c_str());
                    pSite->add_page_categories(catVal);
                    rtb::Site_Ext_PageCategory* pPageCate = pSiteExt->add_page_category();
                    pPageCate->set_id(catVal);
                    pPageCate->set_weight(pPc->confidence_level());
                    cacheValue.push_back(catVal);
                }
                // store mapping data to cache
                g_ContentCateCache.store(pc_id, cacheValue);
                LOG_DEBUG << "miss ContentCateCache, using tdbm mapping data, at setting page categories";
            }
            else
            {
                //LOG_WARN << "fetch mapping page category failed at tanx adapter, and search key is:" <<
                //            pc_id << "pid:" << tanxPid;
            }
        }*/
    }
    return 0;
}


int TanxAdapter::SetOtherDevice(rtb::Device *pDevice, poseidon::tanx::BidRequest &request,
                                LoopContextPtr &loopctx)
{
    tanx::BidRequest_Mobile_Device* pMdevice = request.mutable_mobile()->mutable_device();
    const std::set<std::string> &filter = configer->FilterImei();
    // set device id, may be need decryption
    if (pMdevice->has_device_id()) {
        // decryption device_id
        string plainDevId;
        string encrypt_key = configer->GetProperty<string>("base.encrypt_key", "");
        loopctx->devid_decoder.decode(pMdevice->device_id(), plainDevId, encrypt_key);
        //xxxx add:��Щûȡ����ȷIMEI��ģ�����0.��ʱ���μӾ��ۣ�ֱ�ӷ��ء�
        if(atoll(plainDevId.c_str()) == 0) { //ƻ���豸idfa��Ҫȡ����if
            //LOG_ERROR << "bid:" << request.bid() << " no device_id!";
            return -1;
        }


        pDevice->set_id(plainDevId);

        if(filter.size() > 0 && request.ip() != "14.152.64.74") {
            if(filter.find(plainDevId) == filter.end()) {
                LOG_TRACE << "Filter imei enabled, But current imei:" << plainDevId <<
                          " is not in filter list, So it will not be process!";
                return -1;
            }
        }

        //LOG_INFO << "----------------------DEVICE_ID:" << plainDevId << "---------------------";
    } else if(filter.size() > 0) { //�����ָ�����ˣ��ҵ�ǰ����û���豸�ŵĻ�����ôҲ���˵���
        return -1;
    }

    // set brand
    if (pMdevice->has_brand()) {
        pDevice->set_brand(pMdevice->brand());
    }
    // set model
    if (pMdevice->has_model()) {
        pDevice->set_model(pMdevice->model());
    }
    // set os
    if (pMdevice->has_os()) {
        std::string os = pMdevice->os();
        boost::to_lower(os);
        //std::transform(os.begin(), os.end(), os.begin(), tolower);
        pDevice->set_os(os);
    }
    // set os ver
    if (pMdevice->has_os_version()) {
        pDevice->set_os_ver(pMdevice->os_version());
    }
    // set connection type
    if (pMdevice->has_network()) {
        // 0-unknown, 1-wifi, 2-2g, 3-3g, 4-4g
        switch (pMdevice->network()) {
        case 0 :
            pDevice->set_connection_type
            (rtb::CONNECTION_TYPE_UNKNOWN);
            break;
        case 1 :
            pDevice->set_connection_type
            (rtb::CONNECTION_TYPE_WIFI);
            break;
        case 2 :
            pDevice->set_connection_type
            (rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
            break;
        case 3 :
            pDevice->set_connection_type
            (rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
            break;
        case 4 :
            pDevice->set_connection_type
            (rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
            break;
        }
    }
    // set network operator
    if (pMdevice->has_operator_()) {
        // 0 unknown 1 cmcc 2 cucc 3 ctc
        pDevice->set_carrier(pMdevice->operator_());
    }
    // set geo info, tanx only have lon and lat
    if (pMdevice->has_latitude() || pMdevice->has_longitude()) {
        rtb::Geo* pGeo = pDevice->mutable_geo();
        if (pMdevice->has_latitude()) {
            pGeo->set_lat(atof(pMdevice->latitude().c_str()));
        }
        if (pMdevice->has_longitude()) {
            pGeo->set_lon(atof(pMdevice->longitude().c_str()));
        }
    }
    // set device type, 2 Mobile Phone 3 Tablet
    if (0 == pMdevice->platform().compare(TanxPlatformList[TANX_PF_IPAD]) ||
            0 == pMdevice->platform().compare(TanxPlatformList[TANX_PF_TABLET])) {
        pDevice->set_device_type(DT_TABLET);
    } else {
        pDevice->set_device_type(DT_PHONE);
    }

    // set ifa, in tanx, only apple device support it, os version later than 6.0
    if (0 == pMdevice->os().compare(TanxOSList[TANX_OS_IOS])) {
        //pDevice->set_ifa(pMdevice->device_id());
        pDevice->set_ifa(pDevice->has_id() ? pDevice->id() : "");
    }
    // set dev resolution and device pixel ratio
    if (pMdevice->has_device_size() || pMdevice->has_device_pixel_ratio()) {
        rtb::Device_Ext* pDExt = pDevice->mutable_ext();
        pDExt->set_dev_resolution(pMdevice->device_size());
        pDExt->set_device_pixel_ratio(pMdevice->device_pixel_ratio());
    }
    return 0;
}

int TanxAdapter::Process(tanx::BidRequest &request, RtbReqSharedPtr &rtb_request, LoopContextPtr &loopctx)
{
    //LOG_INFO << "IN-bid:" << request.bid();
    if(request.adzinfo_size() == 0) {
        LOG_ERROR << "request.adzinfo is NULL!@";
        return -1;
    }

    /*
    mini_request.traffic_source = (request.adx_type() == 1 ? GOOGLE : TANX);
    mini_request.page_pv_id = request.page_session_id();
    mini_request.imp_id = request.mutable_adzinfo(0)->pid();
    */

    if (!request.has_ip()) {
        if (1 == request.adx_type()) { // google traffic
            LOG_DEBUG << "no ip address in google traffic, and will be build default ip, \
                    bid:" << request.bid();
            request.set_ip("0.0.0.1");
        } else {
            //LOG_ERROR << "no ip address in tanx traffic, bid: " << request.bid();
            //rtb::BidResponse_Ext* pMoBidExt = mama_pb_object_->.rtb_response.mutable_ext();
            //pMoBidExt->set_machine_path("-tanx.no.ip");
            return -1;
        }

    } else {
        LOG_TRACE << "tanx traffic client ip:" << request.ip();
    }

    //string dsp_id;
    rtb_request->set_id(request.bid());

    string tanxPid;
    string publisherId;
    //BidRequest::AdzInfo�ṹ�崦���ƹ�λ��Ϣ��
    for (int i = 0; i < request.adzinfo_size(); ++i) {
        tanx::BidRequest_AdzInfo* pAdz = request.mutable_adzinfo(i);
        rtb::Impression* pImp = rtb_request->add_impressions();
        if( pAdz->has_publisher_id()) {
            publisherId = pAdz->publisher_id();
        }
        tanxPid = pAdz->pid();
        pImp->set_id(tanxPid);
        if(pAdz->view_type_size() == 0) {
            LOG_ERROR << "view type is empty from tanx traffic, this is not allows";
            return -1;
        }
        //ѭ����rtb::impressions���鸳ֵ
        RtbAdzInfo(i, pAdz, pImp, request);
    }

    //BidRequest::Mobile�ṹ�崦��(APPӦ��)

    if(request.has_mobile() && request.mobile().has_is_app() && request.mobile().is_app()) {
        RtbMobileInfo(request, rtb_request->mutable_app(), publisherId);
    } else {
        RtbSiteInfo(request, rtb_request->mutable_site(),
                    publisherId, tanxPid);
    }

    rtb::Device* pDevice = rtb_request->mutable_device();
    pDevice->set_ip(request.ip());



    if (request.has_user_agent()) {
        pDevice->set_user_agent(request.user_agent());
    }

    //����mobile��������device��Ϣ
    if (request.has_mobile() && request.mobile().has_device()) {
        //����rtb::device�ֶ�ֵ
        if(SetOtherDevice(pDevice, request, loopctx) != 0) {
            return -1;
        }

    } else if(IS_MOBILE_DEVICE(request.mutable_adzinfo(0)->view_type(0))) {
        pDevice->set_device_type(DT_PHONE);
    } else {
        pDevice->set_device_type(DT_PC);
    }


    //pDevice->set_id("352324076339892"); //xxxx TEST ! TODO
    // set whether support javascript, but tanx nonsupport temporarily
    // set flash version, but tanx nonsupport temporarily
    // set user info
    rtb::User* pUser = rtb_request->mutable_user();
    // set id and acookie
    if (request.has_tid()) {
        pUser->set_id(request.tid());
        rtb::User_Ext* pUExt = pUser->mutable_ext();
        pUExt->set_acookie(request.tid());
        //mini_request.aid_info.acookie = request.tid();//xxxx add
    }
    // set aid
    if (request.has_aid()) {
        rtb::User_Ext* pUExt = pUser->mutable_ext();
        pUExt->set_aid(request.aid());
        //mini_request.aid_info.aid = request.aid();
    }
    // set nick-name
    if (request.private_info_size() > 0 &&
            request.mutable_private_info(0)->has_nick_name()) {
        rtb::User_Ext* pUExt = pUser->mutable_ext();
        pUExt->set_nick_name(request.mutable_private_info(0)->nick_name());
    }
    // set birth day of user, but tanx nonsupport temporarily
    // set gender, but tanx nonsupport temporarily
    // set keywords, but tanx nonsupport temporarily
    // set geo of user's home, but tanx nonsupport temporarily
    // set extend data, but tanx nonsupport temporarily
    // set auction_type, but tanx is empty temporarily
    // set max timeout, but tanx is empty temporarily
    // set seats, but tanx is empty temporarily
    // set all impressions, but tanx is empty temporarily
    // set traffic source 0.JS-PT 1.S2S-PT 2.TANX
    // set timezone offset
    if (request.has_timezone_offset()) {
        rtb_request->mutable_ext()->set_timezone_offset(request.timezone_offset());
    }
    // set page pvid
    if (request.has_page_session_id()) {
        rtb_request->mutable_ext()->set_page_pv_id(request.page_session_id());
    }
    // set dsp id
    //��Ϊ��ProtocolObject.cpp��ͳһ�趨
    //rtb_request->mutable_ext()->set_dsp_id(mama_pb_object_->DspID());

    /*if (pDevice->device_type() != DT_PC)  // not pc
    {
        if(0 == pDevice->os().compare(MobileOSList[OS_IOS]))
        {
            string os_ver = pDevice->os_ver();
            char num[2] = {0};
            num[0] = os_ver[0];
            if (atoi(num) < 6)
            {
                mini_request.aid_info.mac = pDevice->id();
            }
            else
            {
                mini_request.aid_info.idfa = pDevice->id();
            }
        }
        else if(0 == pDevice->os().compare(MobileOSList[OS_ANDROID]))
        {
            mini_request.aid_info.imei = pDevice->id();
        }
    }*/
    LOG_DEBUG << "[-->Tanx Request<--]\n" << request.DebugString() <<
              "\n[<--Rtb Request-->]\n" << rtb_request->DebugString();
    return 0;
}

int TanxAdapter::buildEmptyTanxResp(int version, const std::string &bid, std::string &emptyresp)
{
    poseidon::tanx::BidResponse tanxresp;
    tanxresp.set_version(version);
    tanxresp.set_bid(bid);
    //LOG_DEBUG << "Empty resp:" << tanxresp.DebugString();
    tanxresp.SerializeToString(&emptyresp);
    return 0;
}

int TanxAdapter::buildPingResp(tanx::BidRequest &request, string& pingResp)
{
    LOG_DEBUG << "building ping response to Tanx";
    poseidon::tanx::BidResponse tanxresp;
    tanxresp.set_version(request.version());
    tanxresp.set_bid(request.bid());
    tanxresp.SerializeToString(&pingResp);
    return 0;
}

int TanxAdapter::buildTestResp(tanx::BidRequest &request, string& testResp)
{
    //build a normal ad response
    //default ad from tdbm, to do...
    poseidon::tanx::BidResponse tanxresp;
    tanxresp.set_version(request.version());
    tanxresp.set_bid(request.bid());
    LOG_DEBUG << "building test ads response to Tanx";

    return 0;
}


int TanxAdapter::ProcessToTanxResp(rtb::BidResponse &dsp_resp, std::string &content)
{
    poseidon::tanx::BidResponse tanxresp;
    //TanxAdDataRepack tanx_data_pack(mini_request.rtb_request, dsp_resp);

    // clear noisy data
    tanxresp.Clear();
    // set tanx BidResponse version
    tanxresp.set_version(mama_pb_object_->mama_tanx_request_.version());
    // set tanx bid
    tanxresp.set_bid(mama_pb_object_->mama_tanx_request_.bid());
    // set tanx ads
    int adNumIndex = 0;
    string adJsonInfo;

    int view_type = mama_pb_object_->mama_tanx_request_.adzinfo(0).view_type(0);
    for (int i = 0; i < dsp_resp.bid_seats_size(); ++i) {
        rtb::BidSeat* pBidSeat = dsp_resp.mutable_bid_seats(i);

        for (int j = 0; j < pBidSeat->bids_size(); ++j) {
            rtb::Bid* pBid = pBidSeat->mutable_bids(j); //������ϢBid[j]
            if (!pBid->has_ext() || !pBid->mutable_ext()->has_creative_template_id()
                    || !pBid->has_image_url()) {
                LOG_ERROR << "BidResponse from controller miss necessary fields(ext) in tanx adapter, and bid is :"
                          <<tanxresp.bid();
                continue;
            }
            //xxxx 2016/07/19 BidResponse����traffic_bid_flag�ֶ�

            //xxxx add:�ж��Ƿ�native��������������Ҫ���MobileCreateive��Ϣ
            bool  bNativeTraffic = IS_NATIVE_TRAFFIC(view_type);

            rtb::Bid_Ext* pExt = pBid->mutable_ext();
            //xxxx add:tanx::resp����Ҫ�����ֶξ���message Ads
            //attention:��DSP���Ա������󱨼ۣ���Ҫ���ñ��ֶ�
            tanx::BidResponse_Ads* pAds = tanxresp.add_ads();
            // set adzinfo id, set as 0
            pAds->set_adzinfo_id(0);
            // set max_cpm_price
            pAds->set_max_cpm_price(pBid->price());
            // set ad bid count index
            pAds->set_ad_bid_count_idx(adNumIndex++);

            // set html_snippet
            //xxxx add:Ŀǰ�ȹ̶�һ��snippet����������Ƶ
            string snippet = configer->GetProperty<string>("base.snippet", "");
            if(snippet.length() == 0) {
                LOG_ERROR << "Snippet is empty!";
                return -1;
            }

            //xxxx add:���õ����ַ
            if(pExt->has_click_url() && pExt->click_url().length() > 0) {
                string clickUrlEncode;
                string clickUrl = "u=";
                string urlStr;
                unsigned int crc16_unused;
                mama_pb_object_->StringEncode(pExt->click_url(), crc16_unused, clickUrlEncode);
                clickUrl += clickUrlEncode;
                mama_pb_object_->BuildClick(dsp_resp, pBid, clickUrl, urlStr);
                //�ƶ��豸�ϵ�APP��������Mobile.is_appΪ1����DSP�����ص�BidResponse���Բ���
                //�������
                StringCodec::Replace(snippet, "%%CLICK_URL%%", urlStr);
                //pAds->add_click_through_url(urlStr);

                /*
                //���ߺ����Ҫ��htmlsnip������%%CLICK_URL%%��
                if(VT_WL_BANNER == view_type)
                {
                    //�������ַ��tanx����%%CLICK_URL%%�滻�ɴ�ֵ
                    pAds->add_click_through_url(urlStr);
                }
                else
                {
                    StringCodec::Replace(snippet, "%%CLICK_URL%%", urlStr);
                }
                */
            }
            /*string snippet = "<a href=\"%%CLICK_URL%%\"><img src=\"%%EXPOSE_URL%%\"/>
                    style='width:1px;height:1px;display:block;'/></a>";
            */
            //����ع�url
            StringCodec::Replace(snippet, EXPOSE_MACRO, pBid->image_url());
            // native traffic
            if (bNativeTraffic) { //xxxx:�����native����������view_type����������mobilecreateive(api�ĵ�)
                if(!buildMobileCreative(dsp_resp, pBid, pAds, adJsonInfo)) {
                    LOG_ERROR << "build mobile creative meta info error, and bid is :" << dsp_resp.id();
                    return -1;
                }
            } else { // non native traffic(banner or video)
                if(IS_VIDEO_ADS(view_type)) {
                    snippet = "";
                    string feeback_url;
                    mama_pb_object_->BuildExpose(dsp_resp, pBid, "s=%%SETTLE_PRICE%%", feeback_url);
                    if(BuildVideoSnippet(snippet, feeback_url, pBid, pAds) != 0) {
                        //LOG_ERROR << "build video snippet error, and bid is :" << dsp_resp.id();
                        return -1;
                    }
                }
            }
            // add display feedback url, if it's mobile traffic, not append it
            if (IS_OUTER_FEEDBACK(view_type)) {
                std::string efUrl;
                mama_pb_object_->BuildExpose(dsp_resp, pBid, "s=%%SETTLE_PRICE%%", efUrl);
                pAds->set_feedback_address(efUrl);//xxxx:չ�ַ���
            } else if (IS_VIDEO_ADS(view_type)) {
                //LOG_DEBUG << "video traffic, not need append ef-url, and bid is " << tanxresp.bid();
            } else { // pc traffic or mobile web  xxxx add:�ƶ���ҳ������
                if (string::npos == snippet.rfind(EF_MACRO)) {
                    snippet.append("<img src=\"%%EF_URL%%\" style='width:1px;height:1px;display:block;'/>");
                }
                mama_pb_object_->BuildExpose(dsp_resp, pBid, "s=%%SETTLE_PRICE%%", snippet);
                // 第三方曝光串埋点
                /*const string tdUrl = tanx_data_pack.getTdUrl();
                if (!tdUrl.empty())
                {
                    if (string::npos == snippet.rfind(TD_MACRO))
                    {
                        snippet.append("<img src=\"%%TD_URL%%\" style='width:1px;height:1px;display:block;'/>");
                    }
                    StringCodec::Replace(snippet, TD_MACRO, tdUrl);
                }
                */
            }

            // set html snippet
            if (IS_VIDEO_ADS(view_type)) {
                pAds->set_video_snippet(snippet);
            } else if(!bNativeTraffic) { //xxxx add:native��������Ҫ����snippet�ֶΣ�
                //xxxx add:������������Ҫ���õ����
                pAds->set_html_snippet(snippet);
            }

            // set ad category
            for (int cateIdx = 0; cateIdx < pExt->category_size(); ++cateIdx) {
                int in_cate = pExt->category(cateIdx);
                pAds->add_category(in_cate);//xxxx modify
            }
            // set creative type
            if (pExt->has_creative_format()) {
                pAds->add_creative_type((int)pExt->creative_format());
            }
            // set destination url
            pAds->add_destination_url(pExt->dest_url());
            // set creative id
            pAds->set_creative_id(pBid->creative_id());
            // set resource address, just for mobile wireless ground

            // set deal id
            if (pBid->has_deal_id()) {
                pAds->set_dealid(atoi(pBid->deal_id().c_str()));
            }
            // set advertiser ids
            if (pExt->has_advertiser_id()) {
                pAds->add_advertiser_ids(pExt->advertiser_id());
            }
            // set download complete feed back url, just for mango H5 traffic, exclude native traffic,
            // 1 means direct download
            if (!bNativeTraffic && pExt->has_download_url() && pExt->download_url().length() > 0) {
                //string dlUrl;
                //mama_pb_object_->BuildDownloadcomplete(dsp_resp, pBid, dlUrl);
                //pAds->set_download_complete(dlUrl);
            }
            // set creative adaptive-type for wap traffic, 1 等比 2 定高
            if (IS_WAP_TS(view_type)) {
                //pAds->set_creative_adaptive_type(tanx_data_pack.getAdapType());
            }
        }
    }
    const std::string &deb_info = tanxresp.DebugString();
    if (deb_info.length() < 1024 * 19) {
        LOG_INFO << "[<--Tanx Response-->]\n" << deb_info;
    } else {
        LOG_WARN << "Tanx resp size is too big:" << deb_info.length();
        //cout << deb_info << endl;
        LOG_INFO << "[<--Tanx Response-->]\n" << deb_info.substr(0, 1024* 19);
    }
    tanxresp.SerializeToString(&content);
    return 0;
}


/*
���: http://127.0.0.1:10008/adx/tanx/v1/pc/click
c: crc16ЧӦ��
q: ���ܺ�Ĵ�
u: ���ܺ���Ҫ��ת��url������ http://www.163.com ���ܺ�Ĵ�
*/

bool TanxAdapter::buildMobileCreative(const rtb::BidResponse &rtb_resp, rtb::Bid* pBid, tanx::BidResponse_Ads* pTanxAds,
                                      const string& jsonContent)
{
    tanx::MobileCreative* pMobileCreative = pTanxAds->mutable_mobile_creative();
    rtb::Bid_Ext* pExt = pBid->mutable_ext();
    // set version, bid, and view_type of MobileCreative
    pMobileCreative->set_version(mama_pb_object_->mama_tanx_request_.version());
    pMobileCreative->set_bid(mama_pb_object_->rtb_request_->id());
    pMobileCreative->set_view_type(mama_pb_object_->mama_tanx_request_.adzinfo(0).view_type(0));

    // native template id mapping
    const string &creativeTempId = pExt->creative_template_id();
    pMobileCreative->set_native_template_id(creativeTempId);

    /*
    if(mama_pb_object_->mama_tanx_request_.has_mobile())
    {
        if(mama_pb_object_->mama_tanx_request_.mobile().native_template_id_size() > 0)
            pMobileCreative->set_native_template_id(
                                                    mama_pb_object_->mama_tanx_request_.mobile().native_template_id(0));
    }
    */


    tanx::BidRequest_Mobile *pMobile = mama_pb_object_->mama_tanx_request_.mutable_mobile();
    for(int i = 0; i < mama_pb_object_->mama_tanx_request_.mutable_mobile()->native_ad_template_size(); ++i) {
        //�����±�ȡ������native_ad_template
        tanx::BidRequest_Mobile_NativeAdTemplate *tanx_native_template = pMobile->mutable_native_ad_template(i);
        if(tanx_native_template->native_template_id() != creativeTempId) {
            continue;
        }
        //ѭ��tanx��������Ԫ��native_template�е�area����
        for(int j = 0; j < tanx_native_template->areas_size(); ++j) {
            //�����±�ȡ��tanx�е�area
            tanx::BidRequest_Mobile_NativeAdTemplate_Area *tanx_area = tanx_native_template->mutable_areas(j);
            //���������Լ����ֶ�
            const tanx::BidRequest_Mobile_NativeAdTemplate_Area_Creative *tanx_creative = tanx_area->mutable_creative();

            tanx::MobileCreative_Area *resp_area = pMobileCreative->add_areas();
            resp_area->set_id(tanx_area->id());
            tanx::MobileCreative_Creative *resp_area_creative =
                resp_area->add_creatives();

            if(pExt->has_dest_url()) {
                //xxxx add:��ת���յ�ַ�����ֶ�ֵĿǰ��ext->click_url��ͬ
                resp_area_creative->set_destination_url(pExt->dest_url());
            } else {
                LOG_ERROR << "dest_url is empty!";
                return false;
            }
            resp_area_creative->set_creative_id(pBid->creative_id());


            /*
            if (pBid->has_w() && pBid->has_h())
            {
                char size[22] = {0};
                snprintf(size, sizeof(size), "%dx%d", pBid->w(), pBid->h());
                resp_area_creative->set_img_size(size);
            }

            if(!pBid->has_image_url()) {
                LOG_ERROR << "templateid:" << creativeTempId << " image_url is empty!";
                return false;
            }
            resp_area_creative->set_img_url(pBid->image_url());//3ͼƬ

            if(pBid->ext().has_click_url())
            {
                string clickUrlEncode;
                string urlStr;
                unsigned int crc;
                std::string encodestr;

                mama_pb_object_->StringEncode(pBid->ext().click_url(), crc, encodestr);
                StringCodec::UrlEncode(encodestr, clickUrlEncode);
                clickUrlEncode = "u=" + clickUrlEncode;

                mama_pb_object_->BuildClick(pBid, clickUrlEncode, urlStr);
                resp_area_creative->set_click_url(urlStr);
            }
            */

            //�ѱ�ѡ��ѡ��һ������ѡ�ֶκϲ�һ��
            std::set<int> required_recommended_fields;
            //��ѡ�ֶ�ȫ��ȡ��
            for(int ii = 0; ii < tanx_creative->required_fields_size(); ++ii) {
                required_recommended_fields.insert(tanx_creative->required_fields(ii));
            }

            //��ѡһ������ȡdownload_url
            std::set<int> multichoice;
            for(int i = 0; i < tanx_creative->multichoice_fields_size(); ++i) {
                multichoice.insert(tanx_creative->multichoice_fields(i));
            }
            if(multichoice.size() > 0) {
                auto iter = multichoice.find(13);
                if(iter != multichoice.end()) {
                    required_recommended_fields.insert(*iter);//����ȡdownload_url
                } else {
                    required_recommended_fields.insert(*(multichoice.begin()));    //ֻȡ��һ��
                }
            }
            //ѡ��
            for(int ii = 0; ii < tanx_creative->recommended_fields_size(); ++ii) {
                required_recommended_fields.insert(tanx_creative->recommended_fields(ii));
            }

            for(auto iter = required_recommended_fields.begin(); iter != required_recommended_fields.end(); ++iter) {
                //1:����;2:�����;3:ͼƬ;4:�۸�;5:�ۿۼ�;
                //6:����;7:click_url;8:landing_type;9:����;10:�򿪷�ʽ;
                //11:���ط�ʽ;12:deep_link;13:����
                int field_id = *iter;
                switch(field_id) {
                case 1:
                    if(pBid->ext().has_title()) {
                        resp_area_creative->set_title(pBid->ext().title());
                    } else {
                        LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:title!";
                    }
                    break;
                case 2:
                    if(pBid->ext().has_ad_words()) {
                        tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                        pAttr->set_name("ad_words");//2�����
                        pAttr->set_value(pBid->ext().ad_words());
                    } else {
                        LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:ad_words!";
                    }
                    break;
                case 3://img_url
                    if (pBid->has_w() && pBid->has_h()) {
                        char size[22] = {0};
                        snprintf(size, sizeof(size), "%dx%d", pBid->w(), pBid->h());
                        resp_area_creative->set_img_size(size);
                    }

                    if(!pBid->has_image_url()) {
                        LOG_ERROR << "templateid:" << creativeTempId << " image_url is empty!";
                        break;
                    }
                    resp_area_creative->set_img_url(pBid->image_url());//3ͼƬ
                    break;
                case 4:
                    LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:price!";
                    break;
                case 5:
                    LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:promoprice!";
                    break;
                case 6:
                    LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:����!";
                    break;
                case 7: { //click_url
                    //�������ֲ�����������Ҫ��case��{}�Ѵ���������
                    std::string clickUrlEncode;
                    std::string clickurl;
                    unsigned int crc;
                    std::string encodestr;
                    mama_pb_object_->StringEncode(pBid->ext().click_url(), crc, encodestr);
                    StringCodec::UrlEncode(encodestr, clickUrlEncode);
                    clickUrlEncode = "u=" + clickUrlEncode;
                    //if(creativeTempId == "29")
                    mama_pb_object_->BuildClick(rtb_resp, pBid, clickUrlEncode, clickurl);
                    resp_area_creative->set_click_url(clickurl);
                    break;
                }
                case 8:
                    LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:loading_type!";
                    break;
                case 9:
                    if(pBid->ext().has_ad_words()) {
                        tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                        pAttr->set_name("description");//xxxx:�ѹ������Ϊdesc??
                        pAttr->set_value(pBid->ext().ad_words());
                    } else {
                        LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:description!";
                    }
                    break;
                case 10:
                    //if(pBid->ext().has_open_type())
                {
                    tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                    pAttr->set_name("open_type");//10�򿪷�ʽ;
                    //char buf[8];
                    //xxxx: 20160824  ��Ǭ����ͨ��open_type�̶���Ϊ��������򿪣�����webkit�򿪣�������
                    //snprintf(buf, sizeof(buf), "%u", /*pBid->ext().open_type()*/2);
                    pAttr->set_value("2");
                }
                    //else LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:open_type!";

                break;
                case 11:
                    if(pBid->ext().has_download_type()) {
                        tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                        pAttr->set_name("download_type");//11���ط�ʽ
                        char buf[8];
                        snprintf(buf, sizeof(buf), "%u", pBid->ext().download_type());
                        pAttr->set_value(buf);
                    } else {
                        LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:download_type!";
                    }

                    break;
                case 12:
                    if(pBid->ext().has_deeplink_url()) {
                        tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                        pAttr->set_name("deeplink_url");//12deep_link
                        pAttr->set_value(pBid->ext().deeplink_url());
                    } else {
                        LOG_WARN << "templateid:" << creativeTempId << " Missing nessary field:deeplink_url!";
                    }

                    break;
                case 13: {
                    //xxxx 201607add:��ʱdownload_url�ǿյġ���ʱ�����ҳ��ַ��Ϊ���ص�ַ��
                    const std::string &downloadurl =
                        (pBid->ext().has_download_url() ? pBid->ext().download_url() : pBid->ext().dest_url());

                    tanx::MobileCreative_Creative_Attr* pAttr = resp_area_creative->add_attr();
                    pAttr->set_name("download_url");//13����
                    pAttr->set_value(downloadurl);
                    break;
                }
                default:
                    LOG_WARN << "templateid:" << creativeTempId << " Unknow nessary field:" << field_id;
                    break;
                }
            }


            //���������ӣ���ô�����������url
            if(required_recommended_fields.find(13) != required_recommended_fields.end()) {
                //tanx::MobileCreative_Creative_TrackingEvents* pMoTrackEvent =
                //    resp_area_creative->mutable_tracking_events();
                //std::string fbUrl;
                //mama_pb_object_->BuildDownloadcomplete(rtb_resp, pBid, fbUrl);
                //pMoTrackEvent->add_download_complete_event(fbUrl);
            }

            //�����ع������
            /*string clickurl;
            mama_pb_object_->BuildClick(rtb_resp, pBid, "", clickurl);
            tanx::MobileCreative_Creative_TrackingEvents* pMoTrackEvent =
                resp_area_creative->mutable_tracking_events();
            pMoTrackEvent->add_click_event(clickurl);
            */
        }
    }


    /*
    // set native creative every field
    tanx::MobileCreative_Creative* pCreative = pMobileCreative->add_creatives();
    if(pExt->has_dest_url())
    {
        pCreative->set_destination_url(pExt->dest_url());//xxxx add:��ת���յ�ַ�����ֶ�ֵĿǰ��ext->click_url��ͬ
    }
    else {
        LOG_ERROR << "dest_url is empty!";
        return false;
    }
    pCreative->set_creative_id(pBid->creative_id());

    //temp:
    //if(pBid->creative_id() == "1021")
    //    pMobileCreative->set_native_template_id("6");
    //temp

    if (pBid->has_w() && pBid->has_h())
    {
        char size[22] = {0};
        snprintf(size, sizeof(size), "%dx%d", pBid->w(), pBid->h());
        pCreative->set_img_size(size);
    }
    if(!pBid->has_image_url()) {
        LOG_ERROR << "image_url is empty!";
        return false;
    }
    pCreative->set_img_url(pBid->image_url());

    if(pBid->has_ext() && pBid->ext().has_click_url() && pBid->ext().click_url().length() > 0)
    {
        string clickUrlEncode;
        string urlStr;
        stringstream ss;
        buildMobileClick(pBid, "", urlStr);
        ss << urlStr << "&";
        StringCodec::UrlEncode(pBid->ext().click_url(), clickUrlEncode);
        ss << "u=" << clickUrlEncode;
        pCreative->set_click_url(ss.str());
    }
    pCreative->set_title(pBid->ext().title());

    if(pBid->ext().has_ad_words())
    {
        tanx::MobileCreative_Creative_Attr* pAttr = pCreative->add_attr();
        pAttr->set_name("ad_words");
        pAttr->set_value(pBid->ext().ad_words());
    }
    if(pBid->ext().has_download_type())
    {
        tanx::MobileCreative_Creative_Attr* pAttr = pCreative->add_attr();
        pAttr->set_name("download_type");
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", pBid->ext().download_type());
        pAttr->set_value(buf);
    }
    if(pBid->ext().has_open_type())
    {
        tanx::MobileCreative_Creative_Attr* pAttr = pCreative->add_attr();
        pAttr->set_name("open_type");
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", pBid->ext().open_type());
        pAttr->set_value(buf);
    }
    if(pBid->ext().has_deeplink_url())
    {
        tanx::MobileCreative_Creative_Attr* pAttr = pCreative->add_attr();
        pAttr->set_name("deeplink_url");
        pAttr->set_value(pBid->ext().deeplink_url());
    }
    if(pBid->ext().has_download_url())
    {
        tanx::MobileCreative_Creative_Attr* pAttr = pCreative->add_attr();
        pAttr->set_name("download_url");
        pAttr->set_value(pBid->ext().download_url());
    }
    // �����ж���������Ҫ���download����: 1.����������Ϣ�а���download_url, 2.��landing_type, �ҵ���1
    if (pBid->ext().has_download_url() && pBid->ext().download_url().length() > 0)
    {
        tanx::MobileCreative_Creative_TrackingEvents* pMoTrackEvent =
            pCreative->mutable_tracking_events();
        string fbUrl;
        if (buildDownloadedFeedAddr(pBid, fbUrl))
        {
            pMoTrackEvent->add_download_complete_event(fbUrl);
            //LOG_DEBUG << "add downloaded feedback url to tanx-biding proto succeed";
        }
        else
        {
            LOG_ERROR << "build downloaded feedback url failed";
            return false;
        }

    }

    // �ж��Ƿ���Ҫ��ӵ�����trd���
    if (pBid->ext().has_deeplink_url() && pBid->ext().deeplink_url().length() > 0)
    {
        tanx::MobileCreative_Creative_TrackingEvents* pMoTrackEvent =
            pCreative->mutable_tracking_events();
        string trdUrl;
        if (buildTrdUrl(pBid, trdUrl))
        {
            pMoTrackEvent->add_click_event(trdUrl);
            LOG_DEBUG << "add trd url for deep link succeed";
        }
        else
        {
            LOG_ERROR << "build trd url for deep link failed";
            return false;
        }
    }
    */
    return true;
}


int TanxAdapter::BuildVideoSnippet(std::string& snippet, const std::string &feeback_url,
                                   rtb::Bid* pVideoBid, tanx::BidResponse_Ads* pVideoTanxAds)
{
    xml_document<> doc;
    xml_node<>* rot = doc.allocate_node(rapidxml::node_element, "VAST");
    rot->append_attribute(doc.allocate_attribute("version","3.0"));
    doc.append_node(rot);

    xml_node<>* ad = doc.allocate_node(node_element,"Ad","information");
    ad->append_attribute(doc.allocate_attribute("id","overlay-1"));
    rot->append_node(ad);

    xml_node<>* in_line = doc.allocate_node(node_element,"InLine");
    ad->append_node(in_line);

    in_line->append_node(doc.allocate_node(node_element,"AdSystem","xxxx.com"));
    in_line->append_node(doc.allocate_node(node_element,"AdTitle"));
    xml_node<>* impression = doc.allocate_node(node_element,"Impression");
    in_line->append_node(impression);
    //impression add cdata
    //չ�ַ�����ǩ�����Զ����������1������۸�����%%SETTLE_PRICE%%
    impression->append_node(doc.allocate_node(node_cdata, 0, feeback_url.c_str(), 0, feeback_url.length()));

    xml_node<>* creatives = doc.allocate_node(node_element,"Creatives");
    in_line->append_node(creatives);

    xml_node<>* creative = doc.allocate_node(node_element,"Creative");
    creatives->append_node(creative);

    int view_type = mama_pb_object_->mama_tanx_request_.adzinfo(0).view_type(0);
    if (IS_NON_LINEAR_VIDEO_AD(view_type)) { //��Ƶ��ͣ
        xml_node<>* nonlinear_ads = doc.allocate_node(node_element,"NonLinearAds");
        creative->append_node(nonlinear_ads);

        //�����Ա�ǩ����ͣҳ������ѡ��ǩ����ѡ���ԣ�width��height����ѡ���ԣ�apiFramework��id
        xml_node<>* nonlinear = doc.allocate_node(node_element,"NonLinear");
        char buff[64];
        snprintf(buff, sizeof(buff), "%d", pVideoBid->h());
        nonlinear->append_attribute(doc.allocate_attribute("height", buff));

        snprintf(buff, sizeof(buff), "%d", pVideoBid->w());
        nonlinear->append_attribute(doc.allocate_attribute("width", buff));
        nonlinear_ads->append_node(nonlinear);

        //�ز���Դ��ǩ����ѡ���ԣ�type����Դurl��tanxϵͳ��ǰ֧��typeȡֵ��
        //image/jpeg��image/png��application/x-shockwave-flash
        xml_node<>* staticResource = doc.allocate_node(node_element, "StaticResource");
        int creative_format = pVideoBid->ext().creative_format();
        //TODO:judge creative_format!!
        staticResource->append_attribute(doc.allocate_attribute("creativeType", "image/jpeg"));
        //staticResource add cdata
        staticResource->append_node(doc.allocate_node(node_cdata, 0, pVideoBid->image_url().c_str(),
                                    0, pVideoBid->image_url().length()));
        nonlinear->append_node(staticResource);

        //�زĵ����ת��ַ��ǩ
        xml_node<>* nonLinearClickThrough = doc.allocate_node(node_element, "NonLinearClickThrough");
        //nonLinearClickThrough add cdata
        std::string click_url;
        if(pVideoTanxAds->click_through_url_size() > 0) {
            click_url = pVideoTanxAds->click_through_url(0);
        }
        nonLinearClickThrough->append_node(doc.allocate_node(node_cdata, 0, click_url.c_str(), 0, click_url.length()));
        nonlinear->append_node(nonLinearClickThrough);

        xml_node<>* nonLinearclicktracking = doc.allocate_node(node_element,"NonLinearClickTracking");
        nonlinear->append_node(nonLinearclicktracking);
    } else { //��Ƶ��Ƭ��not imp
        LOG_WARN << "Linear video current is not support!";
        return -1;

        xml_node<>* linear_ads = doc.allocate_node(node_element,"Linear");
        creative->append_node(linear_ads);

        xml_node<>* duration = doc.allocate_node(node_element,"Duration");

    }
    rapidxml::print(std::back_inserter(snippet), doc, 0);
    return 0;
}




string TanxAdapter::GetDevice_type()
{
    if(mama_pb_object_->rtb_request_->device().device_type() == DT_PC) {
        return "pc";
    } else {
        return "mobile";
    }
}

/*
//�عⷴ��
bool TanxAdapter::appendEfSnippet(rtb::Bid* pBid, rtb::Bid_Ext* pExt, std::string& snippet)
{

    std::string ef_url = configer->GetProperty<std::string>("base.fb_host", "") +
                    configer->GetProperty<std::string>("base.mama_feedback_url", "") +
                                        "/" + GetDevice_type() + "/feedback?";

    muduo::LogStream ss;
    unsigned int k;
    string str;
    string pbBase64Urlencode;
    ss << ef_url << "s=" << TANX_PRICE_MACRO << "&";//"s=%%SETTLE_PRICE%%&";
    //ss << "bid=" << pBid->id() << "&";
    buildFeedBackField(pBid, 1, str);
    mama_pb_object_->StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;

    if (snippet.empty())
    {
        snippet.assign(ss.buffer().data(), ss.buffer().length());
    }
    else
    {
        std::string result(ss.buffer().data(), ss.buffer().length());
        if (!StringCodec::Replace(snippet, EF_MACRO, result))
        {
            LOG_ERROR << "ef url replaced error!";
            return false;
        }
    }

    return true;
}
*/



bool TanxAdapter::buildTrdUrl(rtb::Bid* pBid, std::string& trd)
{
    trd::TRDInfo info;
    info.set_dsp_id(mama_pb_object_->DspID());
    if (pBid->has_impid()) {
        info.set_imp_id(pBid->impid());
    } else {
        LOG_ERROR << "miss impid in BidResponse from controller in building trd url, and the bid id is :" <<
                  mama_pb_object_->rtb_request_->id();
        return false;
    }

    info.set_pv_time(mama_pb_object_->Now());
    info.set_bid(mama_pb_object_->rtb_request_->id());
    //info.set_session_id(mini_request.session_id);
    info.set_traffic_source((unsigned int)mama_pb_object_->rtb_request_->mutable_ext()->traffic_source());
    info.set_creative_id(pBid->creative_id());
    if( pBid->has_ext() ) {
        rtb::Bid_Ext* pBidExt = pBid->mutable_ext();
        info.set_adgroup_id(pBidExt->adgroup_id());
    }
    if (pBid->has_campaign_id()) { //����ID
        char c_id[12] = {0};
        snprintf(c_id, sizeof(c_id), "%d", pBid->campaign_id());
        info.set_campaign_id(c_id);
    }
    string tempStr;
    info.SerializeToString(&tempStr);
    int len = tempStr.size();
    char* enBuf = new char[len];
    for(int i = 0; i < len; ++i) {
        enBuf[i] = tempStr[i] ^ XOR_KEY;
    }
    unsigned int c_k = StringCodec::CRC16(enBuf, len);
    string pbBase64;
    StringCodec::Base64Encode(string(enBuf, len), pbBase64);
    string pbBase64Urlencode;
    StringCodec::UrlEncode(pbBase64, pbBase64Urlencode);
    delete[] enBuf;
    unsigned int k = (XOR_KEY_VER ^ len ) + (c_k << 16);
    char intBuf[32];
    snprintf(intBuf, sizeof(intBuf), "%d", k);
    trd = string(TRD_URL_PRE) + "e=" + pbBase64Urlencode + "&k=" +
          std::string(intBuf);
    return true;
}

