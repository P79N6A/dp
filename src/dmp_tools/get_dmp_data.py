import json
import sys
import time
import datetime
import os

if(len(sys.argv)!=3):
  print sys.argv[0]+" <dmp.tag.dict> <all/update>"
  exit()

dmp_tag_dict=None
dmp_tag_list=[]
try:
  dmp_tag_dict=open(sys.argv[1])
  for line in dmp_tag_dict.readlines():
    print line
    if(line.startswith("#")):
        contine
    paras=line.split("`")
    dmp_id=paras[0];
    dmp_no=int(paras[1])
    dmp_tag_list.append(dmp_id);
except IOError,e:
  print "dmp tag dict can not found!"
  exit()

file_path="dmp.data"
os.system("rm -fr "+file_path)
os.mkdir(file_path)
two_day_ago=datetime.datetime.now() + datetime.timedelta(days=-2)  
data_str = two_day_ago.strftime('%Y%m%d')
for dmp_id in dmp_tag_list:
  data_file_path=file_path+"/"+dmp_id
  if(sys.argv[2]=="all"):
    os.mkdir(data_file_path)
    hdfs_path = "/user/tag/"+dmp_id+"/dt=new/*"
    print hdfs_path
    command="hadoop fs  -get "+hdfs_path+" "+data_file_path
    print command
    os.system(command)
  elif(sys.argv[2]=="update"):
    os.mkdir(data_file_path)
    hdfs_path="/user/tag/"+dmp_id+"/"+data_str+"/update/"+dmp_id+"_"+data_str+"_merge"
    print hdfs_path
    command="hadoop fs  -get "+hdfs_path+" "+data_file_path
    print command
    os.system(command)
  else:
    print sys.argv[2]+" is error command"
    exit()
