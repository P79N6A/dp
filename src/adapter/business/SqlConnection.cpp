#include "SqlConnection.h"
#include "conf/Configer.h"

#include <muduo/base/Logging.h>


/**
    auth: xxxx
    date: 2016/05
**/

using namespace std;
using namespace sql;
using namespace poseidon;
using namespace boost;
using namespace poseidon::adapter;


extern Configer * volatile configer;

SqlConnection::SqlConnection() : driver_(sql::mysql::get_driver_instance())
{
    //ctor
}

SqlConnection::~SqlConnection()
{
    //dtor
}


void SqlConnection::GetBlackWite(vector<EntieBlackWite> &vec)
{
    string sql = "select type, column_name, column_val from T_BLACK_WHITE"
                 " where now() between begin_date and end_date or end_date is null"
                 " and column_name is not null and column_val is not null";
    scoped_ptr< sql::ResultSet > rs;
    try {
        scoped_ptr<sql::PreparedStatement> stmt(sqlcon_->prepareStatement(sql));
        rs.reset(stmt->executeQuery());
    } catch (sql::SQLException &e) {
        LOG_ERROR << "GetBlackWite Raise Error! " << e.what();
        return;
    }
    LOG_INFO << "T_BLACK_WHITE rowcount:" << rs->rowsCount();

    while(rs->next()) {
        EntieBlackWite entie;
        entie.type = rs->getInt("type");
        entie.field_name = rs->getString("column_name");
        entie.field_value = rs->getString("column_val");
        vec.push_back(entie);
    }
}


void SqlConnection::GetAgainstCheat(vector<EntieAgainstCheat> &vec)
{
    string sql = "select column_names, expression,func, sourcefrom from T_AGAINST_CHEATING"
                 " where now() between begin_date and end_date or end_date is null"
                 " and column_names is not null and expression is not null";
    scoped_ptr< sql::ResultSet > rs;
    try {
        scoped_ptr< sql::PreparedStatement > stmt(sqlcon_->prepareStatement(sql));
        rs.reset(stmt->executeQuery());
    } catch (sql::SQLException &e) {
        LOG_ERROR << "GetAgainstCheat Raise Error! " << e.what();
        return;
    }
    LOG_INFO << "T_AGAINST_CHEATING rowcount:" << rs->rowsCount();

    while(rs->next()) {
        EntieAgainstCheat entie;
        entie.source = rs->getInt("sourcefrom");
        entie.field_names = rs->getString("column_names");
        entie.expression = rs->getString("expression");
        entie.func = rs->getString("func");
        vec.push_back(entie);
    }
}

/*
  sql::ConnectOptionsMap connection_properties;
  connection_properties["hostName"] = hostName;
  connection_properties["userName"] = userName;
  connection_properties["password"] = password;
*/

int SqlConnection::Connect()
{
    try {
        if(configer->sql_host().length() == 0 || configer->sql_user().length() == 0 ||
                configer->sql_pass().length() == 0) {
            return 0;
        }
        LOG_DEBUG << "Connect to database:" << configer->sql_user() <<
                  "/"<<configer->sql_pass() << "@"<<configer->sql_host();
        //con = driver_->connect(configer->sql_host(), configer->sql_user(), configer->sql_pass());
        sql::ConnectOptionsMap connection_properties;
        connection_properties["hostName"] = configer->sql_host();
        connection_properties["userName"] = configer->sql_user();
        connection_properties["password"] = configer->sql_pass();
        if(configer->sql_port() != 0) {
            char port[7];
            snprintf(port, sizeof(port), "%d", configer->sql_port());
            connection_properties["port"] = string(port);
        }
        Connection *con = driver_->connect(configer->sql_host(), configer->sql_user(), configer->sql_pass());
        if(con) {
            //char buff[256];
            //snprintf(buff, sizeof(buff), "use %s;", configer.sql_db().c_str());
            sqlcon_.reset(con);
            sqlcon_->setSchema(configer->sql_db());
            return 1;
        }
    } catch (sql::SQLException &e) {
        LOG_ERROR << "Connect to database failed! " << e.what();
    }
    return 0;
}

