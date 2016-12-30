#ifndef JSONOBJECT_H
#define JSONOBJECT_H

#include "ProtocolObject.h"
#include "utility/json.h"


namespace poseidon
{
namespace adapter
{


class JsonObject : public ProtocolObject
{
public:
    JsonObject();
    virtual ~JsonObject();
    static void OnThreadInitStatic();
protected:
    OpenDspJson::Reader reader_;
    OpenDspJson::Value root_;
    OpenDspJson::Value response_;
};

}
}

#endif // JSONOBJECT_H
