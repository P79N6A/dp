/**
**/

#ifndef _ORS_BID_PRICE_FUNCTOR_H_
#define _ORS_BID_PRICE_FUNCTOR_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class BidPriceFunctor : public Functor
{

public:
    BidPriceFunctor();
    virtual ~BidPriceFunctor();

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);
    virtual int Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor);
    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    float CalBidPrice(AdAccessor* ad_accessor);
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_BID_PRICE_FUNCTOR_H_

