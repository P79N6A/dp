#!/usr/bin/env python
# coding=utf-8
# Copyright 2011 eTao Inc. All Rights Reserved.
"""A class that drives a request sender."""
import datetime
import optparse
import os
import random
import threading
import time

import generator
import log
import sender


GOOD_LOG_TEMPLATE = 'good-%s.log'
PROBLEMATIC_LOG_TEMPLATE = 'problematic-%s.log'
INVALID_LOG_TEMPLATE = 'invalid-%s.log'
ERROR_LOG_TEMPLATE = 'error-%s.log'
SNIPPET_LOG_TEMPLATE = 'snippets-%s.html'
VAST_XML_LOG_TEMPLATE = 'sinppets-video-%s.html'

def CreateRequesters(num_senders, max_qps, url, logger_obj, tanx_ids=None,
                     seconds=0, requests=0, interval=0, mobile_rate=0,
		     mobile_device_ids=None, video_rate=0, video_ad_type=0):
  """创建num_senders线程，并为每个创建sender.HTTPSender对象

  Args:
   num_senders：发件人线程创建
   max_qps：最大的整体QPS
   url：发件人将请求发送到的URL
   logger_obj：log.Logger对象
   tanx_ids：Tanx用户ID或没有随机生成的用户ID列表
   seconds：继续发送请求的秒数。
   requests：请求发送。
   interval：秒之间等待的线程创建的数量。
   mobile_rate: 按照多少比例产生含有mobile消息的bid请求
   mobile_device_ids: mobile消息内部的device_id列表
   video_rate:  产生视频广告bid请求的比例
   video_ad_type: 视频广告的类型, 0表示非线性广告, 1表示线性广告

  Returns:
    A list of Requester objects.
  """
  seconds = seconds or 0
  requests = requests or 0
  # Create at most max_qps/10 threads, giving each thread at least 10 QPS.
  num_senders = min(num_senders, max_qps / 10)
  num_senders = max(num_senders, 1)  # Avoid setting num_senders to 0.
  send_rate_per_sender = num_senders / float(max_qps)
  requests_per_sender = requests / num_senders
  requesters = []
  for i in xrange(num_senders):
    generator_obj = generator.RandomBidGenerator(tanx_ids,
		    mobile_rate, mobile_device_ids, video_rate, video_ad_type)
    sender_obj = sender.HTTPSender(url)
    requester = Requester(generator_obj, logger_obj, sender_obj,
                          send_rate_per_sender, seconds, requests_per_sender)
    requester.name = 'requester-thread-%d' % i
    requesters.append(requester)
    if interval:
      time.sleep(interval)
  return requesters


class Requester(threading.Thread):
  """A thread which generates and sends bid requests.

  A Requester can generate either a specific number of requests, or generate
  requests for a specific number of seconds. In either case requests are sent at
  a specific interval configured through the time_between_requests parameter to
  the constructor.
  """

  def __init__(self, generator_obj, logger_obj, sender_obj,
               time_between_requests, seconds=None, requests=None):
    """初始化 Requester 对象.

    Args:
      generator_obj: 一个随机产生的竞价对象.
      logger_obj: 日志记录对象.
      sender_obj: 竞价请求对象.
      time_between_requests: 发送时间间隔.
      seconds: 发送时间.
      requests: 发送请求次数.

    Raises:
      ValueError: If none or both of seconds and requests are specified.
    """
    super(Requester, self).__init__()
    self._generator = generator_obj
    self._logger = logger_obj
    self._sender = sender_obj
    self._time_between_requests = float(time_between_requests)
    self._generated_requests = 0
    self._last_request_start_time = 0.0
    if ((seconds and requests) or
        (not seconds and not requests)):
      raise ValueError('Exactly one of seconds and requests must be'
                       ' specified')
    if seconds:
      self._timedelta = float(seconds)
      self._use_requests_as_stop_signal = False
    else:
      self._max_requests = requests
      self._use_requests_as_stop_signal = True

  def run(self):
    """Thread's run method, simply calls Start().

    This is separate from Start so that unit-tests can invoke start without
    starting a new thread.
    """
    self.Start()

  def Start(self):
    """执行竞价请求."""
    if not self._use_requests_as_stop_signal:
      self._start_time = self._GetCurrentTime()
      self._stop_time = self._start_time + self._timedelta

    while self._ShouldSendMoreRequests():
      request = self._GenerateRequest()
      payload = request.SerializeToString()
      request_start_time = self._GetCurrentTime()
      status, data = self._sender(payload)
      self._logger.LogSynchronousRequest(request, status, data)
      self._Wait()
      self._last_request_start_time = request_start_time

  def _Wait(self):
    """Waits some time to throttle request rate.

    It's convenient to have this as a separate method for mocking.
    """
    time_to_wait = self._time_between_requests

    # 计算最后一个请求开始以来的时间
    if self._last_request_start_time:
      time_since_last_request = (self._GetCurrentTime() -
                                 self._last_request_start_time)
      time_to_wait = max(0, time_to_wait - time_since_last_request)

    # Don't sleep if too much time has passed since the last request.
    if time_to_wait:
      time.sleep(time_to_wait)

  def _ShouldSendMoreRequests(self):
    """判断是否需要发送更多请求

    Returns:
      True if more requests should be sent, False otherwise.
    """
    if self._use_requests_as_stop_signal:
      return self._generated_requests < self._max_requests
    else:
      return self._GetCurrentTime() < self._stop_time

  def _GenerateRequest(self):
    """生成一个竞价请求
    """
    # %1的概率生成ping请求
    if random.random() < 0.01:
      bid_request = self._generator.GeneratePingRequest()
    else:
      bid_request = self._generator.GenerateBidRequest()
    self._generated_requests += 1
    return bid_request

  def _GetCurrentTime(self):
    """取得当前时间
    """
    return time.time()


def PrintSummary(logger, encrypted_price, video_source_file):
  """打印结果信息

  Args:
    logger: A log.Logger object.
    encrypted_price: 确认价格.
  """
  logger.Done()
  summarizer = log.LogSummarizer(logger)
  if encrypted_price:
    summarizer.SetSampleEncryptedPrice(encrypted_price)
  else:
    summarizer.SetSampleEncryptedPrice("")
  summarizer.SetMediaVideoSource(video_source_file)
  summarizer.Summarize()
  timestamp = str(datetime.datetime.now())
  timestamp = timestamp.replace(' ', '-', timestamp.count(' '))
  timestamp = timestamp.replace(':', '', timestamp.count(':'))
  good_log_filename = GOOD_LOG_TEMPLATE % timestamp
  good_log = open(good_log_filename, 'w')
  problematic_log_filename = PROBLEMATIC_LOG_TEMPLATE % timestamp
  problematic_log = open(problematic_log_filename, 'w')
  invalid_log_filename = INVALID_LOG_TEMPLATE % timestamp
  invalid_log = open(invalid_log_filename, 'w')
  error_log_filename = ERROR_LOG_TEMPLATE % (timestamp)
  error_log = open(error_log_filename, 'w')
  snippet_log_filename = SNIPPET_LOG_TEMPLATE % (timestamp)
  snippet_log = open(snippet_log_filename, 'w')
  video_vast_log_filename = VAST_XML_LOG_TEMPLATE % (timestamp)
  video_vast_log = open(video_vast_log_filename, 'w') 
  # todo
  summarizer.WriteLogFiles(good_log, problematic_log, invalid_log, error_log,
                           snippet_log, video_vast_log)
  good_log.close()
  problematic_log.close()
  invalid_log.close()
  error_log.close()
  snippet_log.close()
  video_vast_log.close()
  summarizer.PrintReport()

  # 数据清理
  for file_name in [good_log_filename, problematic_log_filename,
                    invalid_log_filename, error_log_filename,
                    snippet_log_filename]:
    if not os.path.getsize(file_name):
      os.remove(file_name)


def SetupCommandLineOptions():
  """从终端读取输入数据

  Returns:
    An optparse.OptionParser object.
  """
  parser = optparse.OptionParser()
  parser.add_option('--url', help='URL of the bidder.')
  parser.add_option('--max_qps', type='int',
                    help='Maximum queries per second to send to the bidder.')
  parser.add_option('--seconds', type='int',
                    help='Total duration in seconds. Specify exactly one of '
                    '--seconds or --requests.')
  parser.add_option('--requests', type='int',
                    help='Total number of requests to send. Specify exactly '
                    'one of --seconds or --requests.')
  parser.add_option('--num_threads', type='int',
                    default=20, help='Maximum number of threads to use. The '
                    'actual number of threads may be lower.')
  parser.add_option('--thread_interval', type='int',
                    default=0.2, help='Interval between thread creation, allows'
                    ' for a gradual rampup')
  parser.add_option('--encoded_price', type='string',
                    help='Use the given string as the encrypted price when'
                    'rendering snippets. Pass one of the sample encrypted'
                    'prices provided by Tanx.')
  parser.add_option('--tanx_user_ids_file', type='string',
                    help='Path to a file containing a list of Tanx IDs one '
                    'per line. These will be used instead of randomly '
                    ' generated ID if specified.')
  parser.add_option('--mobile', type='float', 
                    dest="mobile_rate", default=0,
                    help='Proportion of BidRequest for Mobile messsage'
		    '([0.0-1.0], 0.0 by default)')
  parser.add_option('--mobile_device_ids_file', type='string',
                    help='Path to a file containing a list of device_id one '
                    'per line. These will be used instead of randomly '
                    ' generated device ID if specified.')
  parser.add_option('--video', type='float',
                    dest="video_rate", default=0,
                    help='Proportion of BidRequest for video advertisement'
                    ' ([0.0-1.0]), 0.0 by default')
  parser.add_option('--video_type', type='int',
                    dest="video_ad_type", default=0,
                    help='The type of video advertisement, include NonLinear and Linear,'
                    '0 is Nonlinear-Ad, 1 is Linear-Ad')
  parser.add_option('--video_source_file', type='string',
                    default="http://d.tv.taobao.com/TB2iPe.XVXXXXbfXXXXXXXXXXXX.flv?auth_key=F55432572D04DE3AB9A76FED212BF5CE-2034304458&ttr=taotanx",
                    help='video ads creative url of dsp, it is must be flv format, example: --video_source_file=http://www.xxx.com/file/xxx.flv')
  return parser


def ParseCommandLineArguments(parser):
  """解析终端参数, 并进行一些相关检查
  Args:
    parser: An optparse.OptionParser object initialized with options.

  Returns:
    The result of OptionParser.parse after it has been checked for errors.
  """
  opts, args = parser.parse_args()
  if args:
    parser.error('unexpected positional arguments "%s".' % ' '.join(args))
  if ((opts.requests and opts.seconds) or
      (not opts.requests and not opts.seconds)):
    parser.error('exactly one of --requests and --seconds requires a value.')
  if not opts.url:
    parser.error('--url requies a value.')
  if not opts.max_qps:
    parser.error('--max_qps requires a value.')
  return opts


def GetTanxUserIdsFromFile(tanx_user_ids_filename):
  """读取tanx的user_ids_file, 返回数据list

  Args:
     tanx_user_ids_filename:tanx的user_ids_file

  """
  if not os.path.exists(tanx_user_ids_filename):
    print 'Invalid file: %s' % tanx_user_ids_filename
    return None

  try:
    tanx_user_ids_file = open(tanx_user_ids_filename, 'r')
    tanx_user_ids = tanx_user_ids_file.readlines()
    tanx_user_ids_file.close()
  except IOError:
    print 'Could not open file for reading: %s' % tanx_user_ids_filename
    return None
  tanx_user_ids = map(lambda x: x.rstrip('\r\n'), tanx_user_ids)
  return tanx_user_ids

def GetTanxMobileDeviceIdsFromFile(mobile_device_ids_filename):
  """读取mobile 的 device_id, 返回数据list
  """
  if not os.path.exists(mobile_device_ids_filename):
    print 'Invalid file: %s' % mobile_device_ids_filename
    return None

  try:
    mobile_device_ids_file = open(mobile_device_ids_filename, 'r')
    mobile_device_ids = mobile_device_ids_file.readlines()
    mobile_device_ids_file.close()
  except IOError:
    print 'Could not open file for reading: %s' % mobile_device_ids_filename 
    return None
  mobile_device_ids = map(lambda x: x.rstrip('\r\n'), mobile_device_ids)
  return mobile_device_ids 



def main():
  '''程序入口. '''
  parser = SetupCommandLineOptions() #传入参数解析
  opts = ParseCommandLineArguments(parser)
  logger_obj = log.Logger()

  tanx_user_ids = None #取得cookie from file
  if opts.tanx_user_ids_file:
    tanx_user_ids = GetTanxUserIdsFromFile(opts.tanx_user_ids_file)
  mobile_device_ids = None #取得device id from file
  if opts.mobile_device_ids_file:
    mobile_device_ids = GetTanxMobileDeviceIdsFromFile(opts.mobile_device_ids_file)

  #生成请求对象
  requesters = CreateRequesters(opts.num_threads, opts.max_qps, opts.url,
                                logger_obj, tanx_user_ids, opts.seconds,
                                opts.requests, opts.thread_interval, 
				opts.mobile_rate, mobile_device_ids,
				opts.video_rate, opts.video_ad_type)
  #循环执行竞价请求
  for requester in requesters:
    requester.start()

  for requester in requesters:
    requester.join()

  #输出日志
  PrintSummary(logger_obj, opts.encoded_price, opts.video_source_file)

if __name__ == '__main__':
  main()
