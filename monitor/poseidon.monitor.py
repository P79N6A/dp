import json
import sys
import MySQLdb
import httplib 
import time
from kazoo.client import KazooClient

def send_alarm(name,members,data):
  last_send_record_path="/tmp/poseidon_monitor_record_"+name
  record_file=None
  last_check_time=0
  last_send_time=0
  last_send_no=None
  need_send=False
  send_no=0
  send_time=0
  now_time=time.time()
  try:
    record_file=open(last_send_record_path,"r")
    file_text=record_file.read()
    paras=file_text.split(",")
    last_check_time=float(paras[0])
    last_send_time=float(paras[1])
    last_send_no=int(paras[2])
    send_time=last_send_time
    record_file.close()
    if(now_time-last_check_time>120):
      need_send=True
      send_time=now_time
    else:
      send_no=last_send_no+1
      interval=None
      if(send_no==1):
        interval=300
      elif(send_no==2):
        interval=600
      elif(send_no==3):
        interval=1800
      else:
        interval=3600
      if(now_time-last_send_time>interval):
        need_send=True
        send_time=now_time
      else:
        send_no=last_send_no
  except IOError,e:
    need_send=True
    send_time=now_time
  
  record_file=open(last_send_record_path,"w")
  write_text=str(now_time)+","+str(send_time)+","+str(send_no)
  record_file.write(write_text)
  record_file.close()

    
  if(need_send):
    for member in members:
      msg_data='{"address": "'+member["phone"]+'", "content": "'+data+'"}'
      print "send to : "+member["name"]
      print "send msg : "+ msg_data
      
      httpClient = None
      try:
          headers = {"Content-type": "application/json"}
       
          httpClient = httplib.HTTPConnection("api.stat.ucgc.local", 8080, timeout=30)
          httpClient.request("POST", "/api/v2/util/message/sms", msg_data, headers)
       
          response = httpClient.getresponse()
          print "http response code : "+str(response.status)
          print "recv data : "+response.read()
      except Exception, e:
          print e
      finally:
          if httpClient:
              httpClient.close()

print "-----------------------------------------------"
print "poseidon monitor start"
print time.strftime("%Y-%m-%d %H:%M:%S",time.localtime(time.time()))
config_file=None
try:
  config_file=open(sys.argv[1])
  config_text=config_file.read()
  config_json=json.loads(config_text)
  db_host=config_json["indicator_monitor"]["db_config"]["host"]
  db_user=config_json["indicator_monitor"]["db_config"]["user"]
  db_pw=config_json["indicator_monitor"]["db_config"]["passwd"]
  db_name=config_json["indicator_monitor"]["db_config"]["db"]
  db_port=config_json["indicator_monitor"]["db_config"]["port"]
  
  try:
    conn=MySQLdb.connect(db_host,db_user,db_pw,db_name,db_port)
    #threshold_monitor
    continued=config_json["indicator_monitor"]["threshold_monitor"]["continued"]
    for monitor_info in config_json["indicator_monitor"]["threshold_monitor"]["monitor_infos"]:
      alarm_min=monitor_info["min"]
      alarm_max=monitor_info["max"]
      host_group_str=monitor_info["host_group"]
      member_group_str=monitor_info["alarm_group"]
      member_group=config_json["alarm_groups"][member_group_str]
      alarm_data="warning!!!"   
      alarm_send=False
      for host_name in config_json["host_groups"][host_group_str]:
        cur1=conn.cursor()
        sql="use "+host_name
        try:
          cur1.execute(sql)
          cur1.close;
        except MySQLdb.Error,e:
          cur1.close;
          continue;
        
        cur2=conn.cursor()
        sql="show tables like 'attr_"+str(monitor_info["code"])+"'"
        cur2.execute(sql)
        results = cur2.fetchall()
        cur2.close;
        if(len(results)==0):
          continue;
        
        sql="select time_min,value from attr_"+str(monitor_info["code"])+" order by time_min desc limit "+str(continued)+";"
        #print sql;
        cur3=conn.cursor()
        cur3.execute(sql)
        results = cur3.fetchall()
        cur3.close;
        min_sum=0
        max_sum=0
        lastest_value=-1;
        for row in results:
          if(lastest_value==-1):
            lastest_value=row[1]
          time_min = row[0]
          value = row[1]
          #print str(time_min)+" "+str(value)
          if(alarm_min>=0) and (value<alarm_min):
            min_sum+=1
          if(alarm_max>=0) and (value>alarm_max):
            max_sum+=1   
        if(min_sum>=continued):
          alarm_data+="host["+host_name+"]"+monitor_info["name"]+" is "+str(lastest_value)+" lt "+str(alarm_min)+";"
          alarm_send=True
        if(max_sum>=continued):
          alarm_data+="host["+host_name+"]"+monitor_info["name"]+" is "+str(lastest_value)+" gt "+str(alarm_max)+";"
          alarm_send=True
      if(alarm_send):
        send_alarm(str(monitor_info["code"]),member_group,alarm_data)
    conn.close()
  except MySQLdb.Error,e:
    print "Mysql Error %d: %s" % (e.args[0], e.args[1])  
  
  member_group=config_json["alarm_groups"][config_json["thread_monitor"]["alarm_group"]]
  poseidon_path="/poseidon/server/"
  zk = KazooClient(hosts='10.32.50.182:2181')
  zk.start()
  for thread_monitor_node in config_json["thread_monitor"]["monitor_infos"]:
    server_group_path=poseidon_path+thread_monitor_node["name"]
    for server_host in thread_monitor_node["hosts"]:
      thread_exist=False
      zk_servers=zk.get_children(server_group_path)
      for zk_server in zk_servers:
        zk_server_ip=zk_server.split(":")[0]
        if(zk_server_ip==server_host):
          thread_exist=True
          break;
      if(thread_exist==False):
        alarm_data="warning!!!host["+server_host+"] has no "+thread_monitor_node["name"]
        send_alarm(server_host,member_group,alarm_data)
  zk.stop()
  
finally:
  config_file.close()
  exit()

