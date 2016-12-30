#ifndef COMMONPB_H
#define COMMONPB_H

#include "ProtocolObject.h"

namespace poseidon
{
namespace adapter
{

class CommonPb : public ProtocolObject
{
public:
    CommonPb();
    virtual ~CommonPb();
    static void OnThreadInitStatic();
};

}
}

#endif // COMMONPB_H
