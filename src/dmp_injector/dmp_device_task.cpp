#include "dmp_device_task.h"
#include "redis_client.h"
DEFINE_string(mysql_host, "db-mysql2", "mysql_host");
DEFINE_int32(mysql_port, 13346, "mysql_port");
DEFINE_string(mysql_user, "games_basedata", "mysql_user");
DEFINE_string(mysql_pw, "cwLasICRRcQkNNwxSqxl", "mysql_pw");
DEFINE_string(mysql_table, "games_basedata", "mysql_table");

namespace poseidon
{
  void DeviceTask::process()
  {
    if(_running)
      return;
    util::ScopeWMutex swm(&_run_mutex);
    if(_running)
      return;
    _running=true;
    mysql_init(&_mysql);
    mysql_options(&_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    mysql_options(&_mysql, MYSQL_INIT_COMMAND, "SET NAMES utf8");
    if(!mysql_real_connect(&_mysql,FLAGS_mysql_host.c_str(),FLAGS_mysql_user.c_str(),
                        FLAGS_mysql_pw.c_str(),FLAGS_mysql_table.c_str(),FLAGS_mysql_port,NULL,0))
    {
      cout<<"mysql connect error"<<endl;
      return;
    }
    string sql="select concat_ws('`',t.brand,t.model),t1.brand,t1.model,CASE  WHEN t1.price<='500' THEN '1' WHEN t1.price<='1000' THEN '2' WHEN t1.price<='2000' THEN '3' WHEN t1.price<='3000' THEN '4' WHEN t1.price>'3000' THEN '7' END as price  from phones_model_cnt t,phones_model_standard t1 where t.standard_id = t1.id;";
    int res = mysql_query(&_mysql,sql.c_str());
    if(!res)
    {
      MYSQL_RES *result=mysql_store_result(&_mysql);
      MYSQL_ROW sql_row;
      if(result)
      {
        dmp::DmpDevicePriceDatas price_datas;
        while(sql_row=mysql_fetch_row(result))
        {
          dmp::DmpDevicePriceDatas::DevicePriceData * data=price_datas.add_datas();
          data->set_key(sql_row[0]);
          data->set_brand(sql_row[1]);
          data->set_model(sql_row[2]);
          data->set_price(sql_row[3]);
        }
        wrire_redis(price_datas);
      }
    }
    mysql_close(&_mysql);
  }
  void DeviceTask::wrire_redis(dmp::DmpDevicePriceDatas & datas)
  {
    cout<<"device price data len : "<<datas.ByteSize()<<endl;
    char buffer[1024*1024*8]={0};
    datas.SerializeToArray(buffer,sizeof(buffer));
    RedisClient redis_client;
    if(!redis_client.init())
    {
      cout<<"redis client init error..."<<endl;
      return;
    }
    redisContext * redis_context=redis_client.get_redis_context(DEVICE_PRICE_TAG_REDIS_KEY);
    redisReply* r2=(redisReply*)redisCommand(redis_context, "set %s %b",DEVICE_PRICE_TAG_REDIS_KEY,buffer,datas.ByteSize());
        ScopeReply sr2(r2);
    if (NULL == r2) 
    {  
      cout<<"DeviceTask set error,reply is null"<<endl;
      return;
    } 
    if(!(r2->type == REDIS_REPLY_STATUS && strcasecmp(r2->str,"OK")==0))
    {
      cout<<"DeviceTask redis hset error,type="<<r2->type <<endl;
      return;
    }
    cout<<"device price data inject redis succ"<<endl;
  }
  
  //原有注入sql语句
  /*select concat_ws('`',t.brand,t.model),concat_ws('`',t1.brand,t1.model,CASE  WHEN t1.price<='500' THEN '1' WHEN t1.price<='1000' THEN '2' WHEN t1.price<='2000' THEN '3' WHEN t1.price<='3000' THEN '4' WHEN t1.price>'3000' THEN '7' END ) as data from phones_model_cnt t,phones_model_standard t1 where t.standard_id = t1.id;*/
}