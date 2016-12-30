/**
 **/

#include <list>
#include "src/sn_new/inverted_index/inverted_index.h"
#include "src/sn_new/common/comm_macro.h"

#define MAKE_TAG(tag, value) (((int64_t)tag<< 32) | (int64_t)value)

int main(int argc, char * argv []) {
    int rt = 0;
    do {
        poseidon::sn::Query query;

        poseidon::sn::InvertedIndex ii;
        rt = ii.Init();
        if(rt != 0) {
            printf("ii.Init return error[%d]\n", rt);
            break;
        }
        std::list<poseidon::common::Ad> ad_list;
        query.push_back(MAKE_TAG(poseidon::sn::SOURCE_TARG_ID, 6));
        query.push_back(MAKE_TAG(poseidon::sn::DEAL_ID_TARG_ID, 0));
        query.push_back(MAKE_TAG(poseidon::sn::VIEW_TYPE_TARG_ID, 450));
        query.push_back(MAKE_TAG(2002, 2));
        query.push_back(MAKE_TAG(2004, 1));

        rt = ii.QueryIndex(query, ad_list);
        if(rt != 0) {
            printf("ii.QueryIndex return error[%d]\n", rt);
            break;
        }
        std::list<poseidon::common::Ad>::iterator it;
        for(it = ad_list.begin(); it != ad_list.end(); ++it)
        {
            printf("Ad[%s]\n", it->DebugString().c_str());
        }


    } while(0);
    return rt;
}
