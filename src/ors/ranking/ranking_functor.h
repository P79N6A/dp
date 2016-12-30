/**
**/

#ifndef _ORS_RANKING_FUNCTOR_H_
#define _ORS_RANKING_FUNCTOR_H_
//include STD C/C++ head files

//include third_party_lib head files
#include "src/ors/common/functor.h"

namespace poseidon
{
namespace ors
{
class RankingFunctor : public Functor
{

public:
    RankingFunctor();
    virtual ~RankingFunctor();
    virtual void SetName(const std::string& name)
    {
        m_functor_name = name;
    }

    virtual const std::string& GetName()
    {
        return m_functor_name;
    }

    virtual bool Init();
    virtual void Fini();

    virtual int BeginWork(QueryAccessor* query_accessor);

    virtual int Work(AccessorProvider* accessor_provider);

    virtual int EndWork(QueryAccessor* query_accessor);

protected:
    bool m_enable_pdb_simple_mode;
    std::set<std::string> m_bid_dev_id_set;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_RANKING_FUNCTOR_H_

