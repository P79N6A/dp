import redis
import hashlib
import os
import sys
import ctypes
import zlib
from rediscluster import StrictRedisCluster

redis_nodes =  [{'host':'cnode773','port':7000},
                {'host':'cnode774','port':7000},
                {'host':'cnode775','port':7000},
                {'host':'cnode776','port':7000},
                {'host':'cnode777','port':7000},
                {'host':'cnode781','port':7000}
                ]
try:
  redis_conn = StrictRedisCluster(startup_nodes=redis_nodes)
except Exception,e:
  print e
  sys.exit(-1)

def write_to_redis(imei):
  md5_context=hashlib.md5()
  md5_context.update(imei)
  md5_str = md5_context.hexdigest()
  hash_int=ctypes.c_uint32(zlib.crc32(md5_str)).value
  bucket_key=hash_int/128
  if(redis_conn.hset(str(bucket_key),md5_str,imei)==1):
    print "imei="+imei+"`bucket="+str(bucket_key)

file_path=sys.argv[1]
try:
  tag_file=open(file_path,"r")
  print "tag file : "+file_path
  for line in tag_file:  
    imei=line.split("`")[0]
    write_to_redis(imei)
except Exception,e:
  print e
finally:
  tag_file.close
