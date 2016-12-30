#!/usr/bin/env python
# coding=utf-8
# Copyright 2011 eTao Inc. All Rights Reserved.
"""Contains classes for logging Real Time Bidder requests and responses."""

import base64
import cgi
import datetime
import httplib
import random
import re
import sys
import threading
import urllib
import urlparse

import google.protobuf.message
import tanx_bidding_pb2

PROTOCOL_VERSION = 3
TANX_BID_RULE = 1
TANX_SUCCESS_CODE = 0
AD_INDEX = 0
class Record(object):
  """A record of each request/response pair."""
  def __init__(self, bid_request, status_code, payload):
    #竞价请求
    self.bid_request = bid_request
    #竞价返回码
    self.status = status_code
    #竞价信息
    self.payload = payload

    #一个问题说明的字符串列表
    self.problems = []
    # A tanx_bidding_pb2.BidResponse instance.
    self.bid_response = None
    # A map of ad index -> validated HTML snippet (宏替换之后)
    self.html_snippets = {}
    # A map of ad index -> validated VAST-XML snippet (宏替换之后)
    self.xml_snippets = {}
    
    self.resource_address = {}
    # A list of bid_price , index -> price (未竞价)
    self.bid_price_list = {} 


class LoggerException(Exception):
  """Logger 无效使用抛出的异常. """
  pass


class Logger(object):
  """保留了所有的竞价和出价信息. """

  def __iter__(self):
    if not self._done:
      raise LoggerException("Only locked Loggers are iterable.")
    return self

  def __getitem__(self, item):
    if not self._done:
      raise LoggerException("Only locked Loggers are iterable.")
    return self._records[item]

  def next(self):
    if not self._done:
      raise LoggerException("Only locked Loggers can are iterable.")
    if self._current_iteration >= len(self._records):
      self._current_iteration = 0
      raise StopIteration

    c = self._current_iteration
    self._current_iteration += 1
    return self._records[c]

  def __init__(self):
    # Stores Record objects.
    self._records = []
    self._record_lock = threading.Lock()
    self._current_iteration = 0
    self._done = False

  def Done(self):
    """Signals that logging is done, locking this object to modifications."""
    self._record_lock.acquire()
    try:
      self._done = True
    finally:
      self._record_lock.release()

  def IsDone(self):
    """Returns True if this logger has been locked for updates."""
    self._record_lock.acquire()
    try:
      return self._done
    finally:
      self._record_lock.release()

  def LogSynchronousRequest(self, bid_request, status_code, payload):
    """记录同步请求.

    Args:
      bid_request: A tanx_bidding_pb2.BidRequest object.
      status_code: The HTTP status code.
      payload: The HTTP response payload.

    Returns:
      True if the request was logged, False otherwise.
    """
    if self.IsDone():
      return False

    self._record_lock.acquire()
    try:

      record = Record(bid_request, status_code, payload)
      self._records.append(record)
    finally:
      self._record_lock.release()

    return True


def EscapeUrl(input_str):
  """Returned the URL-escaped version of input_str.

  Args:
    input_str: A string to URL-escape.

  Returns:
    The URL-escaped version of input_str.
  """
  return urllib.quote_plus(input_str, "!()*,-./:_~")


class LogSummarizer(object):
  """输出竞价报告. """

  REQUEST_ERROR_MESSAGES = {
    "not-ok": "The HTTP response code was not 200/OK.",
  }

  RESPONSE_ERROR_MESSAGES = {
      "empty": "Response is empty (0 bytes).",
      "parse-error": "Response could not be parsed.",
      "uninitialized": "Response did not contain all required fields.",
      "wrong-protocol-version": "Response contains the wrong protocol version.",
      "no-processing-time": "Response contains no processing time information.",
      "ads-in-ping": "Response for ping message contains ads.",
  }

  AD_ERROR_TEMPLATE = "Ad %d: %s"
  AD_ERROR_MESSAGES = {
      "empty-snippet": "snippet is empty.",
      "no-price": "max_cpm_price is empty.",
      "error-price": "max_cpm_price is lower than min_price.",
      "no-adzones": "ad does not target any adzones.",
      "no-click-through-urls": "ad does not contain any click-through urls.",
      "invalid-url": "invalid click-through URL: ",
      "no-creative-type":"creative_type of outer dsp is empty",
  }

  SNIPPET_ERROR_TEMPLATE = "HTML snippet for ad %d: %s"
  SNIPPET_ERROR_MESSAGES = {
      "click-url-missing": "click url macros missing ",

  }
  ADSLOT_ERROR_TEMPLATE = "Ad %d, adzone %d: %s"
  ADSLOT_ERROR_MESSAGES = {
      "zero-bid": "0 max CPM bid.",
      "zero-min-cpm": "0 min CPM bid",
      "min-more-than-max": "min CPM >= max CPM",
      "invalid-slot-id": "ad slot id is not present in the BidRequest."
  }
  CLICK_URL = "%%CLICK_URL%%"
  TANX_CLICK_URL = "tanxclick=%%CLICK_URL%%"
  CLICK_URL_PRE_ENC = "%%CLICK_URL_PRE_ENC%%"
  CLICK_URL_PRE_UNENC = "%%CLICK_URL_PRE_UNENC%%"
  SETTLE_PRICE = "%%SETTLE_PRICE%%"
  TANX_CLICK_URL_ESC = "tanxclick=%%CLICK_URL_ESC%%"
  WINNING_PRICE = "%%WINNING_PRICE%%"
  FOREIGN_FEEDBACK = "%%FOREIGN_FEEDBACK%%"
  KEY = "0zqyShtub1w/TpfsmBnTYlQzX8m2afGcPuC+bWqsTCPW"
  CLICK_HEADER = "http://localhost/click.php?tanx_e=&tanx_k=&tanx_u="
  EVENT_TRACK_CASE_1 = "</TrackingEvents>"
  EVENT_TRACK_CASE_2 = "<TrackingEvents/>"
  CREATIVE_ID = "%%CREATIVE_ID%%"
  VIDEO_DEFAULT_CREATIVE = "<MediaFile id=\"0\" delivery=\"progressive\" type=\"video/x-flv\" bitrate=\"480\" width=\"848\" height=\"480\"><![CDATA[http://d.tv.taobao.com/TB2nQ1.XVXXXXX8XXXXXXXXXXXX.flv?auth_key=C5F359F745D632F0B57081BA62F53DEF-2034304458&ttr=taotanx]]></MediaFile><MediaFile id=\"1\" delivery=\"progressive\" type=\"video/x-flv\" bitrate=\"480\" width=\"848\" height=\"480\"><![CDATA[http://d.tv.taobao.com/TB2iPe.XVXXXXbfXXXXXXXXXXXX.flv?auth_key=F55432572D04DE3AB9A76FED212BF5CE-2034304458&ttr=taotanx]]></MediaFile>"
  WINNING_PRICE_RATIO = 0.33
 
  def __init__(self, logger):
    """Initializes a LogSummarizer.

    Args:
      logger: An iterable object containing Record instances.
    """
    self._logger = logger
    self._requests_sent = 0
    self._responses_ok = 0
    self._responses_successful_without_bids = 0
    self._processing_time_sum = 0
    self._processing_time_count = 0
    self._encrypted_price = None
    _file = open("sdk.head.html", 'r')
    self._sdk_html = _file.read()
    _file.close()
    self._box_num = 0;
    # nonlinear is 0, linear is 1, default nonlinear ad
    self._video_ad_type = 0;

    # Store records in the following buckets:
    # Good: the response can be parsed and no errors were detected.
    self._good = []
    # Problematic: the response can be parsed but has some problems.
    self._problematic = []
    # Invalid: the response can not be parsed.
    self._invalid = []
    # Error: the HTTP response had a non-200 response code.
    self._error = []

  def SetSampleEncryptedPrice(self, encrypted_price):
    """Sets the encrypted price to use for the ENCRYPTED_PRICE macro.

    Args:
      encrypted_price: A string with the encrypted price to return.
    """
    self._encrypted_price = encrypted_price

  def SetMediaVideoSource(self, video_source_file):
    """set video source file of vast xml mediafile"""
    self._media_file = "<MediaFile id=\"0\" delivery=\"progressive\" type=\"video/x-flv\" bitrate=\"480\" width=\"848\" height=\"480\"><![CDATA[" + video_source_file + "]]></MediaFile>"  
  def Summarize(self):
    """Collects and summarizes information from the logger."""
    for record in self._logger:
      self._requests_sent += 1
      if record.status == httplib.OK:
        self._responses_ok += 1
      else:
        # 返回不是200 ok , 记录错误
        record.problems.append(self.REQUEST_ERROR_MESSAGES["not-ok"])
        self._error.append(record)
        continue

      if len(record.payload) == 0: #返回数据大小为0
        record.problems.append(self.RESPONSE_ERROR_MESSAGES["empty"])
        self._invalid.append(record)
        continue

      #解析返回数据
      bid_response = tanx_bidding_pb2.BidResponse()
      try:
        bid_response.ParseFromString(record.payload)
      except google.protobuf.message.DecodeError:
        record.problems.append(self.RESPONSE_ERROR_MESSAGES["parse-error"])
        self._invalid.append(record)
        continue

      if not bid_response.IsInitialized():
        #可以解析, 但是消息没有初始化, 表示数据不是良好的, 我们认为是不可解析
        record.problems.append(self.RESPONSE_ERROR_MESSAGES["uninitialized"])
        self._invalid.append(record)
        continue

      record.bid_response = bid_response #将解析了的数据赋值给record

      if bid_response.version != PROTOCOL_VERSION: #发出的版本号和返回的版本号不同
        record.problems.append(
            self.RESPONSE_ERROR_MESSAGES["wrong-protocol-version"])

      if record.bid_request.is_ping:   #判断是否为ping请求
        self.ValidatePing(record)
      else:
        if not bid_response.ads:   #没有广告返回
          self._responses_successful_without_bids += 1
          self._good.append(record)
          continue
          # No ads returned, don't validate ads.

        for index, ads in enumerate(bid_response.ads): #有广告返回, 判断广告格式是否合法
          self.ValidateAd(ads, index, record)

      if record.problems:  #如果存在错误数据 
        self._problematic.append(record)
      else:
        self._good.append(record) #讲数据加到成功的数据结构中
  
  def ValidatePing(self, record):
    """判断ping请求是否合法
    """
    bid_response = record.bid_response
    if bid_response.ads:  #如果为ping请求, 不应该有ads返回
      record.problems.append(self.RESPONSE_ERROR_MESSAGES["ads-in-ping"])

  def ValidateAd(self, ads, index, record):
    """判断广告内容是否合法
    Args: adzinfo_id max_cpm_price ad_bid_count_idx html_snippet click_through_url category creative_type network_guid extend_data
    """
    if not ads.HasField("max_cpm_price") or not ads.max_cpm_price: #判断广告位底价和交易价格的关系
      record.problems.append(self.AD_ERROR_TEMPLATE % (
          index, self.AD_ERROR_MESSAGES["no-price"]))
    else:
        if ads.max_cpm_price < record.bid_request.adzinfo[0].min_cpm_price: #DSP出价小于广告位的底价
          record.problems.append(self.AD_ERROR_TEMPLATE % (
              index, self.AD_ERROR_MESSAGES["error-price"]))
        else:
          record.bid_price_list[index] = ads.max_cpm_price
    if (len(ads.creative_type) == 0):
      record.problems.append(self.AD_ERROR_TEMPLATE % (
          index, self.AD_ERROR_MESSAGES["no-creative-type"]))

    for click_through_url in ads.click_through_url: #循环判断click_url格式, 点击宏替换
      parsed_url = urlparse.urlparse(click_through_url)
      # Must have scheme and netloc.
      if not (parsed_url[0]
              and (parsed_url[0] == "http" or parsed_url[0] == "https")
              and parsed_url[1]):
        record.problems.append((self.AD_ERROR_TEMPLATE % (
            index, self.AD_ERROR_MESSAGES["invalid-url"])) +
            click_through_url)

    mobile_is_app = False;
    is_video_ad = False;
    if record.bid_request.HasField("mobile") and record.bid_request.mobile.is_app :
      mobile_is_app = True;

    vt = record.bid_request.adzinfo[0].view_type[0]
    if vt == 11 or vt ==12 or vt == 106 or vt == 107:
      is_video_ad = True
      if vt == 11 or vt == 106:
        self._video_ad_type = 1
      elif vt == 12 or vt == 107:
        self._video_ad_type = 0
    
    is_null = 0;
    if is_video_ad:
      is_null |= self.ValidateVideoSnippet(ads, index, record)
    else:
      if mobile_is_app:
        is_null |= self.ValidateResourceAddress(ads, index, record)
        is_null |= self.ValidateHtmlSnippet(ads, index, record)
      else:
        is_null |= self.ValidateHtmlSnippet(ads, index, record)

    if not is_null :
      record.problems.append(self.AD_ERROR_TEMPLATE % (
        index, self.AD_ERROR_MESSAGES["empty-snippet"]))

  def ValidateResourceAddress(self, ads, index, record):
    if not ads.HasField("resource_address") or not ads.resource_address: 
      return 0;
    record.resource_address[index] = ads.resource_address;
    return 1;

  def ValidateHtmlSnippet(self, ads, index, record):
    """检查html_snippet的合法化, 同时完成click宏的替换."""
    if not ads.HasField("html_snippet") or not ads.html_snippet: #返回数据中不存在html_snippet或者为空
      return 0;

    html_snippet = ads.html_snippet;
    if re.search(self.TANX_CLICK_URL, ads.html_snippet):
       html_snippet = re.sub(self.TANX_CLICK_URL,
                             "",
                             ads.html_snippet)

    elif re.search(self.CLICK_URL, ads.html_snippet):
        if ads.click_through_url:
            click_url = ads.click_through_url[0]
        else:
            click_url = ""
        html_snippet = re.sub(self.CLICK_URL,
                              click_url,
                              ads.html_snippet)
    elif re.search(self.CLICK_URL_PRE_ENC, ads.html_snippet):
       
       html_snippet = re.sub(self.CLICK_URL_PRE_ENC,
                             urllib.urlencode({"a":self.CLICK_HEADER})[2:-1],
                             ads.html_snippet)
    elif re.search(self.CLICK_URL_PRE_UNENC, ads.html_snippet):
       html_snippet = re.sub(self.CLICK_URL_PRE_UNENC,
                             self.CLICK_HEADER,
                             ads.html_snippet)
    
    if re.search(self.FOREIGN_FEEDBACK, html_snippet):
        html_snippet = re.sub(self.FOREIGN_FEEDBACK,
                           '',
                           html_snippet)

    if re.search(self.SETTLE_PRICE, html_snippet):
        html_snippet = re.sub(self.SETTLE_PRICE,
                           self._encrypted_price,
                           html_snippet)
    record.html_snippets[index] = html_snippet   #将转换完的html_snippet赋值回去
    return 1;

  def ValidateVideoSnippet(self, ads, index, record):
    """检查video_snippet的合法化, 同时完成click宏的替换. """
    if not ads.HasField("video_snippet") or not ads.video_snippet:
      return 0
    video_snippet = ads.video_snippet
    if 1==self._video_ad_type:
      if re.search(self.CREATIVE_ID, video_snippet):
        video_snippet = re.sub(self.CREATIVE_ID,
                                  self._media_file,
                                  video_snippet)
      else:
        return 0
    if 1==self._video_ad_type:
      if re.search(self.EVENT_TRACK_CASE_1, video_snippet) or re.search(self.EVENT_TRACK_CASE_2, video_snippet):
        pass
      else:
        return 0

    if re.search(self.TANX_CLICK_URL, video_snippet):
      video_snippet = re.sub(self.TANX_CLICK_URL,
                                         "",
                                         video_snippet)

    if re.search(self.SETTLE_PRICE, video_snippet):
      video_snippet = re.sub(self.SETTLE_PRICE,
                           self._encrypted_price,
                           video_snippet)
    else:
      return 0
    record.xml_snippets[index] = video_snippet   #将转换完的xml_snippets赋值回去
    return 1
    
  def FindAdzInfoInRequest(self, id, bid_request):
    """通过id取得请求数据. """
    for request_adzInfo in bid_request.adzinfo:
      if request_adzInfo.id == id:
        return request_adzInfo
    return None

  def WriteLogFiles(self, good_log, problematic_log, invalid_log, error_log,
                    snippet_log, video_vast_log):
    """写日志 for successful/error/problematic/invalid requests. """
    # 写snippet 的头
    if self._good or self._problematic: #在成功的和存在问题的数据上写头信息
      snippet_log.write("<html><head>%s<title>Rendered snippets</title></head>\n" % (self._sdk_html))
      snippet_log.write("<body id='viewer'><h1>Rendered Snippets</h1>")
      snippet_log.write("<p>Your server has returned the following renderable"
                        " snippets:</p>")
      snippet_log.write("<ul>")

    if self._problematic:
      problematic_log.write("=== Responses that parsed but had problems ===\n")

    self.WriteXMLSnippetBegin(video_vast_log)
    
    for record in self._problematic:
      problematic_log.write("BidRequest:\n")
      problematic_log.write(str(record.bid_request))
      problematic_log.write("\nBidResponse:\n")
      problematic_log.write(str(record.bid_response))
      problematic_log.write("\nProblems:\n")
      for problem in record.problems:
        problematic_log.write("\t%s\n" % problem)
      if (len(record.xml_snippets) != 0):
        self.WriteVideoMeta(record, video_vast_log)
      else:
        self.WriteSnippet(record, snippet_log)

    if self._good:
      good_log.write("=== Successful responses ===\n")
    for record in self._good:
      good_log.write("BidRequest:\n")
      good_log.write(str(record.bid_request))
      good_log.write("\nBidResponse:\n")
      good_log.write(str(record.bid_response))
      if (len(record.xml_snippets) != 0):
        self.WriteVideoMeta(record, video_vast_log)
      else:
        self.WriteSnippet(record, snippet_log)
        
    self.WriteXMLSnippetEnd(video_vast_log)
    
    if self._good or self._problematic:
      # 写snippet的文件位
      snippet_log.write("</ul></body></html>")

    if self._invalid:
      invalid_log.write("=== Responses that failed to parse ===\n")
    for record in self._invalid:
      invalid_log.write("BidRequest:\n")
      invalid_log.write(str(record.bid_request))
      invalid_log.write("\nPayload represented as a python list of bytes:\n")
      byte_list = [ord(c) for c in record.payload]
      invalid_log.write(str(byte_list))

    if self._error:
      error_log.write("=== Requests that received a non 200 HTTP response"
                      " ===\n")
    for record in self._error:
      error_log.write("BidRequest:\n")
      error_log.write(str(record.bid_request))
      error_log.write("HTTP response status code: %d\n" % record.status)
      error_log.write("\nPayload represented as a python list of bytes:\n")
      byte_list = [ord(c) for c in record.payload]
      error_log.write(str(byte_list))

  def WriteSnippet(self, record, log):
    """输出 html_snippets"""
    mobile_is_app = False;
    if record.bid_request.HasField("mobile") and record.bid_request.mobile.is_app :
      mobile_is_app = True;

    if record.html_snippets:
      src = record.html_snippets;
      type = 1;
    else:
      src = record.resource_address;
      type = 2;
    for index, snippet in src.iteritems():
      id = record.bid_response.ads[index].adzinfo_id #取得竞价id
      request_adzInfo = self.FindAdzInfoInRequest(id, record.bid_request) #取得竞价广告位信息
      if request_adzInfo is None:
        continue
      log.write("<li>")
      log.write("<h3>Bid Request</h3>")
      log.write("<pre>%s</pre>" % cgi.escape(str(record.bid_request)))
      log.write("<h3>Bid Response</h3>")
      log.write("<pre>%s</pre>" % cgi.escape(str(record.bid_response)))
      log.write("<h3>Rendered Snippet</h3>")

      width, height = request_adzInfo.size.split('x') #取得广告位的宽和高
      if False == mobile_is_app:#html 代码
        iframe = ("<iframe src='data:text/html;base64,\n%s' "
                  "width=%d height=%d scrolling=no marginwidth=0 "
                  "marginheight=0></iframe>\n" % (
                      base64.b64encode(snippet),
                      int(width),
                      int(height)))
      else :
        device_height, device_width = record.bid_request.mobile.device.device_size.split('x')
	self._box_num += 1;
	iframe = '<iframe scrolling=no marginwidth=0 marginheight=0 id="box%d" style="height:%dpx;width:%dpx"' % (self._box_num, int(height), int(width));
        if type == 1:#snippet
          iframe += '></iframe><script>\nreval=\'%s\';\nvar ifr = document.getElementById(\'box%d\').contentWindow;\nifr.document.open(\'text/html\',\'replace\');\nifr.document.write(reval);\nifr.document.close();\n</script>' % (snippet.replace('\n', '\\n').replace("'", "\\'").replace("/", "\\/"), self._box_num);
	else:#url
	  iframe += ' src="%s"></iframe>' % (snippet);

      log.write(iframe)
      log.write("</li>")
       
  def WriteXMLSnippetBegin(self, log):
    """输出 video_snippets开始部分"""
    log.write("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"zh-CN\" lang=\"zh-CN\">\n")
    log.write("<head>\n")
    log.write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n")
    if self._video_ad_type == 0:
      log.write("<title>视频暂停广告测试</title>\n")
    elif self._video_ad_type == 1:
      log.write("<title>视频贴片广告测试</title>\n")
    log.write("<h1>Rendered Snippets</h1>\n")
    log.write("<p>Your server has returned the following renderable snippets:</p>\n")
    log.write("<style>\n")
    log.write('''#wrapper {
	width:90%;
	margin: 10px 5%;
	border: none;
	padding-top: 10px;
}
p.head {
	font-weight: 800;
	font-size: 1.6em;
	color: black;
	font-size: 1.6em;
	display: block;
	margin: 0;
	padding: 0;
}
pre.contents {
	font-size: 1.0em;
	padding: 0px;
	line-height: 20px;
	display: block;
	width: 600px;
	padding-left: 10px;
}
div.row {
	float: left;
	width: auto;
	height: auto;
	display: block;
	padding: 10px;
	clear: both;
	color: black;
	font-weight: 800;
	margin: 0;
	position: relative;
}
div.row span {
	color: blue;
}
div.left, right {
	float: left;
	height: auto;
	vertical-align: middle;
}
div.right {
	padding-left: 40px;
	display: inline-block;
	padding-top: 140px;
}
div.clear {
	display: block;
	height: 1px;
	clear: both;
	float: none;
}\n''')
    log.write("</style>\n")
    log.write("</head>\n")
    log.write("<body>\n")
    log.write("<div id=\"wrapper\">\n")
    log.write("</div>\n")
    log.write("<script>\n")

  def WriteVideoMeta(self, record, log):
    """输出 video_snippets meta 数据"""
    global AD_INDEX
    for (index, snippet) in record.xml_snippets.items():
      log.write("req_" + str(AD_INDEX) + " = \"<p class='head'>" + str(AD_INDEX+1) + ".Bid Request</p><pre class='contents'>%s</pre>\";\n" % (cgi.escape(str(record.bid_request))).replace('"','\\"').replace('\\\\','\\\\\\').replace('\n','</br>'))
      log.write("resp_" + str(AD_INDEX) + " = \"<p class='head'>" + "Bid Response</p><pre class='contents'>%s</pre>\";\n" % (cgi.escape(str(record.bid_response))).replace('"','\\"').replace('\\\\','\\\\\\').replace('\n','</br>'))
      log.write("var xml_"+ str(AD_INDEX) + " = ' " +snippet+"';\n")
      log.write("var xx_" + str(AD_INDEX) + " = " + "\"vast_data=\"+ encodeURIComponent(xml_" + str(AD_INDEX) + ");\n")
      if self._video_ad_type == 0:
        log.write("var embedHTML_" + str(AD_INDEX) + " = '<embed type=\"application/x-shockwave-flash\" src=\"nonlinear_loader.swf\" width=\"400\" height=\"300\" style=\"display: block !important;\" id=\"bcv1_213154\" allowscriptaccess=\"never\" name=\"bcv1_213154\" quality=\"high\" wmode=\"transparent\" flashvars=\"' + xx_" + str(AD_INDEX) + " + '\">'\n")
      elif self._video_ad_type == 1:
        log.write("var embedHTML_" + str(AD_INDEX) + " = '<embed type=\"application/x-shockwave-flash\" src=\"linear_loader.swf\" width=\"400\" height=\"300\" style=\"display: block !important;\" id=\"bcv1_213154\" allowscriptaccess=\"never\" name=\"bcv1_213154\" quality=\"high\" wmode=\"transparent\" flashvars=\"' + xx_" + str(AD_INDEX) + " + '\">'\n")
      log.write("var child_"+ str(AD_INDEX) + " = " + "req_"+ str(AD_INDEX) +" + resp_" + str(AD_INDEX) + "+ \"<div class='row'><div class='left'>\"\n")
      log.write("+ embedHTML_"+ str(AD_INDEX) + " + \"</div></div><div class='clear'></div>\";\n")
      AD_INDEX = AD_INDEX + 1
    
  
  def WriteXMLSnippetEnd(self, log):
    """输出 video_snippets结尾部分"""
    global AD_INDEX
    log.write("document.getElementById('wrapper').innerHTML= ")
    for k in range(0, AD_INDEX-1):
      log.write("child_" + str(k) + "+")
    log.write("child_" + str(AD_INDEX-1) + ";\n")
    log.write("</script>\n")
    log.write("</body>\n")
    log.write("</html>\n")

  def PrintReport(self):
    """Prints a summary report."""
    print "=== Summary of Real-time Bidding test ==="
    print "Requests sent: %d" % self._requests_sent
    print "Responses with a 200/OK HTTP response code: %d" % self._responses_ok
    print "Responses with a non-200 HTTP response code: %d" % len(self._error)
    print "Good responses (no problems found): %d" % len(self._good)
    print "Invalid (unparseable) with a 200/OK HTTP response code: %d" % len(
        self._invalid)
    print "Parseable responses with problems: %d" % len(self._problematic)
    if self._processing_time_count:
      print "Average processing time in milliseconds %d" % (
          self._processing_time_sum * 1.0 / self._processing_time_count)
    if self._responses_successful_without_bids == self._requests_sent:
      print "ERROR: None of the responses had bids!"
