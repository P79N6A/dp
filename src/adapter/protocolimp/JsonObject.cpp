#include "JsonObject.h"


using namespace poseidon;
using namespace poseidon::adapter;
using namespace OpenDspJson;

JsonObject::JsonObject() : reader_(Features::strictMode())
{
    //ctor
}

JsonObject::~JsonObject()
{
    //dtor
}

void JsonObject::OnThreadInitStatic()
{

}
