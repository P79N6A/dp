/**
**/

#ifndef _ORS_FUNCTOR_H_
#define _ORS_FUNCTOR_H_
//include STD C/C++ head files

//include third_party_lib head files
#include "src/ors/common/accessor_provider.h"

namespace poseidon
{
namespace ors
{
class Functor
{

public:
    Functor();
    virtual ~Functor();
    virtual void SetName(const std::string& name)
    {
        m_functor_name = name;
    }

    virtual const std::string& GetName()
    {
        return m_functor_name;
    }

    virtual bool Init() = 0;
    virtual void Fini()= 0;

    virtual int BeginWork(QueryAccessor* query_accessor) = 0;

    // overwrite in scoring
    virtual int Work(AdAccessor* /*ad_accessor*/, QueryAccessor* /*query_accessor*/)
    {
        return 0;
    }

    // overwrite in ranking/reranking
    virtual int Work(AccessorProvider* /*accessor_provider*/)
    {
        return 0;
    }

    virtual int EndWork(QueryAccessor* query_accessor) = 0;

protected:
    std::string m_functor_name;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_FUNCTOR_H_

