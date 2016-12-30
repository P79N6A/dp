import os
import sys
import redis
from rediscluster import StrictRedisCluster

redis_nodes =  [{'host':'cnode773','port':7000},
                {'host':'cnode774','port':7000},
                {'host':'cnode775','port':7000},
                {'host':'cnode776','port':7000},
                {'host':'cnode777','port':7000},
                {'host':'cnode781','port':7000}
                ]
try:
  redisconn = StrictRedisCluster(startup_nodes=redis_nodes)
except Exception,e:
  print e
  sys.exit(-1)

def delete_ucid_tag(ucid):
  bucket_key=ucid[0:-2]
  index_key=ucid[-2:]
  global redisconn
  try:
    print "ucid="+ucid+"`key="+bucket_key+"`index="+index_key+"`ret="+str(redisconn.hdel(bucket_key,index_key))
  except Exception,e:
    print e
    sys.exit(-1)
    

data_path="./dmp.data"
for son_path in os.listdir(data_path):
  son_path=data_path+"/"+son_path
  for file_path in os.listdir(son_path):
    file_path=son_path+"/"+file_path
    try:
      tag_file=open(file_path,"r")
      print "tag file : "+file_path
      for line in tag_file:  
        ucid=line.split("`")[0]
        delete_ucid_tag(ucid)
    except Exception,e:
      print e
      continue
    finally:
      tag_file.close