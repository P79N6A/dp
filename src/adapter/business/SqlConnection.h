#ifndef SQLCONNECTION_H
#define SQLCONNECTION_H

#include <string>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <mysql_public_iface.h>



namespace poseidon
{

namespace adapter
{

class EntieBlackWite
{
public:
  char type;
  std::string field_name;
  std::string field_value;
};

class EntieAgainstCheat
{
public:
  char source;
  std::string func;
  std::string field_names;
  std::string expression;
};


class SqlConnection : boost::noncopyable
{
public:
    SqlConnection();
    virtual ~SqlConnection();
    int Connect();
    void GetBlackWite(std::vector<EntieBlackWite> &vec);
    void GetAgainstCheat(std::vector<EntieAgainstCheat> &vec);
private:
    sql::Driver *driver_;
    boost::scoped_ptr<sql::Connection> sqlcon_;
};

}
}

#endif // SQLCONNECTION_H
