#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string.h>

#include "third_party/boost/include/boost/lexical_cast.hpp"
#include "third_party/boost/include/boost/algorithm/string/classification.hpp" // Include boost::for is_any_of
#include "third_party/boost/include/boost/algorithm/string/split.hpp" // Include for boost::split
#include "protocol/src/poseidon_ors_model.pb.h"

using namespace std;

int main(int argc, char *argv[]){

    fstream log("run.log", ios::out | ios::trunc | ios::binary);

    if (argc != 3){
        log << "params number less than 2\n";
        log << "params: config_file input_data output_data\n" ;
        return -1;
    }

    char * input_data = argv[1];
    char * output_data = argv[2];

    poseidon::ors::BiddingProposalModel  biddingproposalModel;
    map<string, vector<vector<float> > > res;

    try{
        fstream input(input_data, ios::in);
        log << "input data name: "<<input_data << "\n";
        string str;
        while( getline(input,str) )
        {
            log << "input line data: "<< str << "\n";
            vector<string> pids;
            boost::split(pids, str, boost::is_any_of(","), boost::token_compress_on);
            if ( pids.size() != 4 ){
                log << "parse size error! " << "\n";
                return -1;
            }

            vector<float> tmp_res;
            for( size_t i= 1; i != pids.size(); ++i ){
                tmp_res.push_back( boost::lexical_cast<float>(pids[i]));
            }

            try {
                map<string, vector<vector<float> > >::iterator iterator = res.find(pids[0]);
                if ( iterator != res.end()){
                    iterator->second.push_back(tmp_res);
                }else{
                    vector<vector<float > > tmp_v;
                    tmp_v.push_back(tmp_res);
                    res[pids[0]] = tmp_v;
                }
            }
            catch (const boost::bad_lexical_cast &) {
                log << "error occur: len should be 2" << std::endl;
            }

        }

        for(map<string, vector<vector<float> > >::iterator it = res.begin(); it != res.end(); ++it){
           poseidon::ors::BiddingProposalItem * biddingproposalItem = biddingproposalModel.add_items();
           biddingproposalItem->set_pos_id(it->first);
           for(vector<vector<float > >::iterator iter = it->second.begin(); iter != it->second.end(); ++iter ){
                    poseidon::ors::BiddingProposalItem_ApproachBidding * approchbidding = biddingproposalItem->add_approach_biddings();
                    vector<float > tmp_res;
                    tmp_res.assign(iter->begin(), iter->end());
                    approchbidding->set_bid_price(tmp_res[0]);
                    approchbidding->set_bid_cost(tmp_res[1]);
                    approchbidding->set_approach_bid_price(tmp_res[2]);
                    log << tmp_res[0] << "\t" << tmp_res[1] << "\t" << tmp_res[2] << "\n";
           }

        }

        fstream output(output_data, ios::out | ios::trunc | ios::binary);

        if (!biddingproposalModel.SerializeToOstream(&output)) {
            log << "Failed to write msg.\n" ;
            return -1;
        }

    }catch (exception &e){

    }

    return 0;
}
