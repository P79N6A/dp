#include <string>
#include <iostream>
#include <locale>
#include <vector>
#include <algorithm>
#include <boost/locale/encoding_utf.hpp>
#include <boost/algorithm/string.hpp>
#include "src/model_updater/api/algo_model_data_api.h"
#include "src/model_updater/api/algo_model_data_dumper.h"

using namespace poseidon::model_updater;
int main ()
{
    AlgoModelDataApi::get_mutable_instance().Init();

    poseidon::ors::AdxBaseParam param;
    AlgoModelDataApi::get_mutable_instance().GetBaseParamValue(2, &param);
    param.PrintDebugString();

    AlgoModelDataDumper<BaseParamKey, poseidon::ors::AdxBaseParam> dumper;
    dumper.Init();
    dumper.DumpPB(11);
}
