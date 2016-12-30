#!/usr/bin/python
# coding=utf-8
# Copyright 2011 etao Inc. All Rights Reserved.

import base64
import random
import sha
import time
import md5

import tanx_bidding_pb2

PROTOCOL_VERSION = 3

BID_REQUEST_ID_LENGTH = 32  # In bytes.
COOKIE_LENGTH = 8  # In bytes.
TID_VERSION = 1
CATEGORY_VERSION = 1
MAX_BID_COUNT = 2
MAX_CREATIVE_TYPE = 3
MAX_MIN_CPM_PRICE = 100 
MAX_PAGE_SESSION_AD_IDX = 10
PAGE_SESSION_ID_LENGHT = 32


# Placement.
CHANNELS = ['12345']

URL_CATEGORY = [
    40101,
    40102,
    40103,
    40104,
    40401,
    40402,
    40403,
    40702,
    40703,
    40704,
    40805,
    40806,
    40905,
    40906,
    41303,
    41304,
    41601,
    42602,
]

BRANDED_URLS = [
    "http://www.taobao.com/",
    "http://www.ifeng.com/",
    "http://digi.163.com/14/0526/10/9T5PODVM0016677T.html",
    "http://yule.2258.com/mingxing/pandian/883057_3.html",
    "http://news.163.com/14/0526/02/9T4TU8SA00014JB6.html",
    "http://news.ifeng.com/a/20140526/40453565_0.shtml",
    "http://www.miercn.com/bdnews/201405/314643_3.html",
]

MOBILE_PACKAGE_NAME = [
    "com.sotu",
    "foo.bar.demo.app",
    "com.moji.MojiWeather",
]

MOBILE_DEVICE_INFO = [
    ('android', 'alps', 'r811', 'android', '4.0.4', '32.13458833333333', '112.71706833333334', '480x320'),
     ('android', 'bbk', 'vivo+y13', 'android', '4.2.2', '37.461338', '112.027212', '854x480'),
     ('android', 'htc', 'htc+t328d', 'android', '4.0.3', '38.30641', '114.38836', '800x480'),
     ('android', 'sony+ericsson', 'lt26i', 'android', '4.1.2', '', '', '1280x720'),
     ('android', 'unknown', 'haier+hw-w910', 'android', '4.0.4', '', '', '1184x720'),
     ('android', 'xiaomi', 'mi+3c', 'android', '4.3', '30.282966', '120.023275', '1920x1080'),
     ('android', 'zte', 'zte+u960s3', 'android', '4.0.4', '', '', '728x480'),
     ('iphone', '', 'iphone3,2', 'ios', '7.1.1', '39.90212595496143', '116.4683414601789', '960x640'),
     ('iphone', '', 'iphone5,2', 'ios', '7.0.4', '30.28305666715499', '120.0231560101276', '1136x640'),
]

MOBILE_DEVICE_PIXEL_RATIO = [
    800, 1000, 1500, 2000
]

CLICKTHROUGH_URLS = [
    "http://clickthrough.domain1.com",
    "http://clickthrough.domain2.com",
    "http://clickthrough.domain3.com",
    "http://clickthrough.domain4.com",
    "http://clickthrough.domain5.com",
    "http://clickthrough.domain6.com",
    "http://clickthrough.domain7.com",
    "http://clickthrough.domain8.com",
    "http://clickthrough.domain9.com",
]
MAX_EXCLUDED_CLICKTHROUGH_URLS = 3

MAX_CAMPAIGN_ID = 99999999
MAX_ADGROUP_ID = 99999999
MAX_MATCHING_ADGROUPS = 3

DIMENSIONS = [
    (468, 60),
    (120, 600),
    (728, 90),
    (300, 250),
    (250, 250),
    (336, 280),
    (120, 240),
    (125, 125),
    (160, 600),
    (180, 150),
    (110, 32),
    (120, 60),
    (180, 60),
    (420, 600),
    (420, 200),
    (234, 60),
    (200, 200),
]

VIEW_INFO = [
    1,           
    5,
    7,
]
MOBILE_VIEW_INFO = [
    102,
    103,
]


CREATIVE_TYPE = [
    1,           
    2,           
    3,           
    4,           
    5,           
    6,           
    7,           
    8,           
    9,           
]


ADZONE_LOCATION = [
    0,          
    1,          
    2,          
    3,
    4,
    5,
    6,
]

PID_SIZE = [
    ('mm_123_456_789', '468x60'),
    ('mm_123_456_789', '120x600'),
    ('mm_123_456_789', '728x90'),
    ('mm_123_456_789', '300x250'),
    ('mm_123_456_789', '250x250'),
    ('mm_123_456_789', '336x280'),
    ('mm_123_456_789', '120x240'),
    ('mm_123_456_789', '468x60'),
    ('mm_123_456_789', '468x60'),
]

APP_PID_SIZE = [
    ('mm_123_456_789', '240x290'),
    ('mm_123_456_789', '320x50'),
]

MAX_ADZ_SLOT_ID = 200

# Verticals.
MAX_NUM_VERTICALS = 5
VERTICALS = [
    66, 563, 607, 379, 380, 119, 570, 22, 355, 608, 540, 565, 474, 433, 609,
    23, 24,
]

# Geo.
LANGUAGE_CODES = [
    'en',
    'zh',
    ]

REGIONS = [
    "US-NY", "US-CA", "US-FL",
    "US-NC", "US-AK", "US-MA",
    "CA-ON", "CA-NS", "CA-NB",
    "AU-NSW", "AU-ACT", "AU-QLD",
    "AU-VIC"
]

REGION_TO_METRO_CITY_MAP = {
    "US-NY": [(501, "New York"),
              (514, "Buffalo"),
              (501, "Long Island City"),],
    "US-CA": [(807, "Sunnyvale"),
              (807, "Belmont"),
              (803, "Los Angeles"),],
    "US-FL": [(539, "Tampa"),
              (528, "Ft Lauderdale"),
              (548, "Boca Raton"),],
    "US-NC": [(560, "Timberlake"),
              (518, "Greensboro"),
              (545, "Kinston"),],
    "US-AK": [(743, "Anchorage"),
              (743, "Wasilla"),
              (745, "Healy"),],
    "US-MA": [(506, "Burlington"),
              (506, "Boston"),
              (543, "Springfield"),],
# Metro information is defined only for the USA.
    "CA-ON": [(0, "Toronto"),
              (0, "Mississauga"),
              (0, "Vaughan"),],
    "CA-NS": [(0, "Halifax"),
              (0, "Bridgewater"),
              (0, "Dartmouth"),],
    "CA-NB": [(0, "Moncton"),
              (0, "Eindhoven"),
              (0, "Helmond"),],
    "AU-NSW": [(0, "Richmond"),
               (0, "Orange"),
               (0, "Albury"),],
    "AU-ACT": [(0, "Canberra"),],
    "AU-QLD": [(0, "Townsville"),
               (0, "Cairns"),
               (0, "Rockhampton")],
    "AU-VIC": [(0, "Melbourne"),
               (0, "Shepparton"),
               (0, "Sunbury"),],
}

# User info.
USER_AGENTS = [
    "Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.0.2)"
    "Gecko/2008092313 Ubuntu/8.04 (hardy) Firefox/3.1",
    "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.2pre)"
    "Gecko/20070118 Firefox/2.0.0.2pre",
    "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.7pre) Gecko/20070815"
    "Firefox/2.0.0.6 Navigator/9.0b3",
    "Mozilla/5.0 (Macintosh; U; PPC Mac OS X 10_4_11; en) AppleWebKit/528.5+"
    "(KHTML, like Gecko) Version/4.0 Safari/528.1",
    "Mozilla/5.0 (Macintosh; U; PPC Mac OS X; sv-se) AppleWebKit/419"
    "(KHTML, like Gecko) Safari/419.3",
    "Mozilla/5.0 (Windows; U; MSIE 7.0; Windows NT 6.0; en-US)",
    "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0;)",
    "Mozilla/4.08 (compatible; MSIE 6.0; Windows NT 5.1)",
]

# Criteria.
MAX_EXCLUDED_ATTRIBUTES = 3
CREATIVE_ATTRIBUTES = [1, 2, 3, 4, 5, 6, 7, 8, 9]

MAX_EXCLUDED_BUYER_NETWORKS = 2

MAX_INCLUDED_VENDOR_TYPES = 10
VENDOR_TYPES = [
    1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 17, 18, 19, 26, 55, 57, 60, 65,
    69, 70, 92, 94, 96, 110, 113,
]

MAX_EXCLUDED_SENSITIVE_CATEGORIES = 4
SENSITIVE_CATEGORIES = [
    50001,
    50003,
    50004,
    50005,
    50006,
    50007,
    50008,
]

MAX_CONTENT_CATEGORIES = 6
CONTENT_CATEGORIES = [
    80101,
    80110,
    80201,
    80301,
    80404,
    80504,
    80602,
    80801,
    80902,
    81009,
    81105,
    81202,
    81213,
    81307,
    81309,
    81405,
    82002,
    82003,
    82505,
]

MAX_EXCLUDED_CATEGORIES = 10
AD_CATEGORIES = [
    70602,
    71103,
    72002,
    72204,
    70104,
    70105,
    71902,
    71904,
    72107,
    71704,
    70707,
    70804,
    72504,
    71610,
    71003,
    70406,
    70207,
    71303,
]

VIDEO_FORMAT = [
    0, #VIDEO_FLASH
    1, #VIDEO_HTML5
]

VIDEO_CONTENT_INFO = [
    ('Dad, where are we going', 8105),
    ('I am not a movie star', 4273),
    ('The Shawshank Redemption', 2701),
    ('Forrest Gump', 679),
    ('Nuovo cinema Paradiso', 2792)
]

VIDEO_KEY_WORDS = [
"test_1",
"test_2",
"test_3",
"test_4",
"test_5",
"test_6",
"test_7",
"test_8",
"test_9",
"test_10",
"test_11",
]

VIDEOAD_START_DELAY = [
    0,
    -1
]

VIDEOAD_SECTION_START_DELAY = [
    0,
    15000,
    30000,
    45000,
]

MAX_TARGETABLE_CHANNELS = 3
TARGETABLE_CHANNELS = ['all top banner ads', 'right hand side banner',
                       'sports section', 'user generated comments',
                       'weather and news',]

random.seed(time.time())


class RandomBidGenerator(object):

  def __init__(self, tanx_id_list=None, 
               mobile_rate=0, mobile_device_ids=None, video_rate=0, video_ad_type=0):
    self._tanx_id_list = tanx_id_list
    self._mobile_rate = mobile_rate;
    self._mobile_device_ids = mobile_device_ids;
    self._mobile_is_app = True;
    self._video_rate = video_rate;
    self._video_ad_type = video_ad_type;

  def GenerateBidRequest(self):
    if self._mobile_rate > random.random():
      self._is_mobile = True;
      self._mobile_is_app = True;
    else:
      self._is_mobile = False;
      self._mobile_is_app = False;
    if self._video_rate > random.random():
      self._is_video_ad = True;
    else:
      self._is_video_ad = False;
    bid_request = tanx_bidding_pb2.BidRequest() 
    bid_request.version = PROTOCOL_VERSION            
    bid_request.is_test = 1                           
    bid_request.bid = self._GenerateBid()
    self._GeneratePageInfo(bid_request)               
    self._GenerateUserInfo(bid_request)               
    self._GenerateAdzone(bid_request)
    self._GeneratePubInfo(bid_request)
    if self._is_mobile:
      self._GenerateMobileInfo(bid_request)
    if self._is_video_ad:
      self._GenerateVideoInfo(bid_request)

    return bid_request

  def GeneratePingRequest(self):
    bid_request = tanx_bidding_pb2.BidRequest()
    bid_request.version = PROTOCOL_VERSION
    bid_request.bid = self._GenerateBid()
    bid_request.is_ping = True
    return bid_request
  
  def _GenerateTanxTid(self, bid_request):
    if self._mobile_is_app == False:
      if self._tanx_id_list:
        bid_request.tid = random.choice(self._tanx_id_list)
      else:
        tid_str = self._GenerateId(COOKIE_LENGTH)
        bid_request.tid = base64.standard_b64encode(tid_str)

  def _GenerateBid(self):
    now_Time=str(time.time())[0:65535]
    return md5.md5(now_Time+'Tanx').hexdigest()


  def _GenerateIP(self, length = 4):
    random_id = ''
    for _ in range(length):
      random_id += str(random.randint(0, 255)) + '.'
    return random_id[:-1]
    
  def _GenerateId(self, length):
    random_id = ''
    for _ in range(length):
      random_id += chr(random.randint(0, 255))
    return random_id

  def _GenerateStr(self, length):
    random_id = ''
    for _ in range(length):
      random_id += random.choice('abcdef0123456789');
    return random_id

  def _GeneratePageInfo(self, bid_request):
    if self._mobile_is_app == True:
      return ;
    bid_request.url = random.choice(BRANDED_URLS)             
    
    bid_request.category = random.choice(URL_CATEGORY)
    
    bid_request.category_version = CATEGORY_VERSION

    bid_request.page_session_id = self._GenerateStr(PAGE_SESSION_ID_LENGHT);

    num_content_categories = random.randint(1, MAX_CONTENT_CATEGORIES)
    for content_category in self._GenerateSet(CONTENT_CATEGORIES,num_content_categories):
	content_categories = bid_request.content_categories.add();
	content_categories.id = content_category
	content_categories.confidence_level = random.randint(0, 1000)

  def _GenerateAdzone(self, bid_request):
    
    adzone = bid_request.adzinfo.add()                          
    adzone.id = 0
    if self._mobile_is_app :
      adzone.pid, adzone.size = random.choice(APP_PID_SIZE)
      adzone.view_type.append(random.choice(MOBILE_VIEW_INFO))
    elif self._is_video_ad :
      adzone.pid = 'mm_123_456_789'
      adzone.size = '648x480'
      if self._video_ad_type == 0:
        adzone.view_type.append(12)
      elif self._video_ad_type == 1:
        adzone.view_type.append(11)
    else:
      adzone.pid, adzone.size = random.choice(PID_SIZE)
      adzone.view_type.append(random.choice(VIEW_INFO))
      
    adzone.ad_bid_count = random.randint(1,MAX_BID_COUNT) 

    
    
    
    num_excluded_filter = random.randint(1, MAX_CREATIVE_TYPE)
    for excluded_filter in self._GenerateSet(CREATIVE_TYPE,num_excluded_filter):
      adzone.excluded_filter.append(excluded_filter)
    
    adzone.min_cpm_price = random.randint(1, MAX_MIN_CPM_PRICE)
    
    adzone.view_screen = random.choice(ADZONE_LOCATION);

    adzone.page_session_ad_idx = random.randint(0, MAX_PAGE_SESSION_AD_IDX)

    bid_request.adx_type = 0

  def _GenerateUserInfo(self, bid_request):
    
    self._GenerateTanxTid(bid_request)
    bid_request.tid_version = TID_VERSION
    
    if self._mobile_is_app == False:
      bid_request.user_agent = random.choice(USER_AGENTS)
    
    ip = self._GenerateIP(4)
    bid_request.ip = ip

    bid_request.timezone_offset = 480

  def _GenerateMobileInfo(self, bid_request):
    mobile = bid_request.mobile;

    mobile.is_app = True;

    mobile.is_fullscreen = random.choice([True, False]);

    mobile.package_name = random.choice(MOBILE_PACKAGE_NAME);

    device = mobile.device;

    (device.platform, device.brand, device.model, device.os,
     device.os_version, device.longitude, device.latitude, 
     device.device_size) = random.choice(MOBILE_DEVICE_INFO);

    device.network = random.randint(0,4);

    device.operator = random.randint(0,3);

    if self._mobile_device_ids:
      device.device_id = random.choice(self._mobile_device_ids);

    device.device_pixel_ratio = random.choice(MOBILE_DEVICE_PIXEL_RATIO);

  def _GenerateVideoInfo(self, bid_request):
    video = bid_request.video
    video.video_format.append(random.choice(VIDEO_FORMAT))
    content = video.content
    (content.title, content.duration) = random.choice(VIDEO_CONTENT_INFO)
    content.keywords.append(random.choice(VIDEO_KEY_WORDS))
    content.keywords.append(random.choice(VIDEO_KEY_WORDS))
    content.keywords.append(random.choice(VIDEO_KEY_WORDS))
    video.videoad_start_delay = random.choice(VIDEOAD_START_DELAY)
    video.videoad_section_start_delay = random.choice(VIDEOAD_SECTION_START_DELAY)
    video.min_ad_duration = 15000
    video.max_ad_duration =  15000
    video.protocol = '3.0'

  def _GeneratePubInfo(self, bid_request):

    num_excluded_clickthrough_urls = random.randint(1, MAX_EXCLUDED_CLICKTHROUGH_URLS)
    for excluded_url in self._GenerateSet(CLICKTHROUGH_URLS,num_excluded_clickthrough_urls):
      bid_request.excluded_click_through_url.append(excluded_url)
  
    
    if random.random() < 0.2:
      num_excluded_sensitive_categories = random.randint(1, MAX_EXCLUDED_SENSITIVE_CATEGORIES)
      for excluded_sensitive_category in self._GenerateSet(SENSITIVE_CATEGORIES,num_excluded_sensitive_categories):
        bid_request.excluded_sensitive_category.append(excluded_sensitive_category)
    if random.random() < 0.2:
      num_excluded_categories = random.randint(1, MAX_EXCLUDED_CATEGORIES)
      for excluded_category in self._GenerateSet(AD_CATEGORIES,num_excluded_categories):
        bid_request.excluded_ad_category.append(excluded_category)


  def _GenerateSet(self, collection, set_size):
    unique_collection = set(collection)
    if len(unique_collection) < set_size:
      return unique_collection

    s = set()
    while len(s) < set_size:
      s.add(random.choice(collection))

    return s
