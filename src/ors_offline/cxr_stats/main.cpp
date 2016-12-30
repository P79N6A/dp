#include <unistd.h>
#include <fstream>
#include "gflags/gflags.h"
#include "util/log.h"
#include "util/func.h"
#include "src/ors_offline/common/utility.h"
#include "stat_hierarchy.h"
#include "utils.h"

using namespace std;
using namespace poseidon::ors_offline;
using namespace poseidon::ors;
using namespace poseidon::util;

int initStatFromDays(StatHierarchy* weighted_stat, int days,
        const CXRStatsConfig_ModelParams& config);
std::string StatFileDay(int day);
int saveStatRatePB(string output_dir, StatHierarchy &weighted_stat,
        StatHierarchy &today_stat);

static string DATA_DIR;
static string RAW_STAT_FILE_NAME;

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_int32(restart_from_days, 7, "初始化使用历史数据的时长");
DEFINE_string(estat_path, "", "历史加权数据文件，若非空则以此文件初始化，与restart_from_days互斥");
DEFINE_string(data_dir, "../data", "配置数据目录路径");
DEFINE_string(raw_stat_file_name, "model_data", "原始输入统计文件名称");
DEFINE_string(estat_file_name, "estat", "加权统计结果文件名称");
DEFINE_string(pb_file_dir, "./", "pb结果文件目录");

int main(int argc, char** argv) {
    if (1 == argc) {
        fprintf(stderr, "use ./%s --help to show usage\n", argv[0]);
        return -1;
    }

    //load conf
    google::SetVersionString("1.0.0");
    ::google::ParseCommandLineFlags(&argc, &argv, true);

    DATA_DIR = FLAGS_data_dir;
    RAW_STAT_FILE_NAME = FLAGS_raw_stat_file_name;

    if ("" == FLAGS_estat_file_name) {
        fprintf(stderr, "Plz set estat_file_name\n");
        return -1;
    }

    if ("" == FLAGS_raw_stat_file_name) {
        fprintf(stderr, "Plz set raw_stat_file_name\n");
        return -1;
    }

    if ("" == FLAGS_pb_file_dir) {
        fprintf(stderr, "Plz set pb_file_dir\n");
        return -1;
    }

    CXRStatsConfig config;
    if (!ParseProtoFromTextFormatFile(FLAGS_conf.c_str(), &config)) {
        fprintf(stderr, "Failed to parse cxr_stat config %s\n",
                FLAGS_conf.c_str());
        return -1;
    }

    if (!LOG_INIT(config.log4config().config_file(),
            config.log4config().category())) {
        fprintf(stderr, "LOG_INIT error [%s, %s]",
                config.log4config().config_file().c_str(),
                config.log4config().category().c_str());
        return -1;
    }

    if (!poseidon::util::Func::single_instance("../run.pid")) {
        LOG_ERROR("Create PidFile failed!");
        return -1;
    }

    try {
        //init
        StatHierarchy his_stat;
        his_stat.Init(config.model_params());
        if (FLAGS_estat_path != "") { //从文件直接加载
            CheckError(his_stat.ReadStatFromFile(FLAGS_estat_path.c_str()),
                    "Fail to read stat from file");
        } else { //从历史数据初始化
            CheckError(
                    initStatFromDays(&his_stat, FLAGS_restart_from_days,
                            config.model_params()),
                    "Fail to init weighted stat");
        }

        int try_load_times = 0;
        bool is_across_day = false;
        int period = config.model_params().update_cycle_time();

        //run
        while (true) {
            time_t start = time(0);
            string run_time = getYmdHMS(start);
            LOG_INFO("Start stat at %s", run_time.c_str());
            string now_h = getH(start);
            //跨天逻辑
            if ("00" == now_h) {
                is_across_day = true;
                sleep(period);
                continue;
            }
            if (is_across_day) {
                LOG_INFO("Update daily at %s", run_time.c_str());
                string stat_file_yest = StatFileDay(1);
                StatHierarchy stat_yest;
                stat_yest.Init(config.model_params());
                CheckError(
                        stat_yest.ReadBottomStatFromFile(
                                stat_file_yest.c_str()),
                        "Fail to read bottom stat from file %s",
                        stat_file_yest.c_str());
                stat_yest.GroupUp();
                his_stat.EAdd(&stat_yest);
                is_across_day = false;
            }

            //正常流程
            string stat_file = StatFileDay(0);
            StatHierarchy stat_new, weighted_stat;

            stat_new.Init(config.model_params());
            //当天数据可能生成较晚，等待多次
            if (stat_new.ReadBottomStatFromFile(stat_file.c_str()) != 0) {
                LOG_WARN("Fail to read bottom stat from file %s", stat_file.c_str());
                try_load_times++;
                if (try_load_times > 40) {
                    throw -1;
                }
                sleep(period);
                continue;
            }
            try_load_times = 0;

            stat_new.GroupUp();

            weighted_stat.Init(config.model_params());
            weighted_stat.Clone(his_stat);

            weighted_stat.EAdd(&stat_new);
            weighted_stat.CalERates();
            stat_new.CalCpx();

            string estat_file = DATA_DIR + string("/") + GetYmdDaysBefore(0)
                    + string("/") + FLAGS_estat_file_name + string("_")
                    + run_time;
            CheckError(weighted_stat.SaveEStatFile(estat_file.c_str()),
                    "Fail to save estat file");

            CheckError(
                    saveStatRatePB(FLAGS_pb_file_dir, weighted_stat, stat_new),
                    "Fail to save pb");

            //周期等待
            time_t end = time(0);
            time_t wait = period + start - end;
            if (wait > 0) {
                sleep(wait);
            }
        }

    } catch (int e) {
        AlertSMS(config, "Cxr stat is dead!");
        return e;
    }
}

std::string StatFileDay(int day) {
    return DATA_DIR + string("/") + GetYmdDaysBefore(day) + string("/")
            + RAW_STAT_FILE_NAME;
}

int initStatFromDays(StatHierarchy* weighted_stat, int days,
        const CXRStatsConfig_ModelParams& config) {
    try {
        string stat_file = StatFileDay(days);
        CheckError(weighted_stat->ReadBottomStatFromFile(stat_file.c_str()),
                "Fail to read bottom stat from file", stat_file.c_str());
        LOG_INFO("load raw stat data from %s", stat_file.c_str());
        weighted_stat->GroupUp();
        for (int i = days - 1; i > 0; i--) {
            stat_file = StatFileDay(i);
            StatHierarchy new_stat;
            new_stat.Init(config);
            CheckError(new_stat.ReadBottomStatFromFile(stat_file.c_str()),
                    "Fail to read bottom stat from file %s", stat_file.c_str());
            LOG_INFO("load raw stat data from %s", stat_file.c_str());
            new_stat.GroupUp();
            weighted_stat->EAdd(&new_stat);
        }
        return 0;
    } catch (int e) {
        return e;
    }
}

int saveStatRatePB(string output_dir, StatHierarchy &weighted_stat,
        StatHierarchy &today_stat) {
    string file_name = "stat_rate_new.pb";
    string output_path = output_dir + string("/") + file_name;
    string output_tag = output_path + string(".tag");

    StatRateModel stat_rate_model;

    std::vector<StatLayer*>* weighted_layers = weighted_stat.GetLayerList();
    std::vector<StatLayer*>* today_layers = today_stat.GetLayerList();

    for (size_t i = 0; i < StatRateMaxLevel; i++) {

        boost::unordered_map<string, StatInfo>* wmap = weighted_layers->at(i)
                ->StatInfoMap();
        boost::unordered_map<string, StatInfo>* tmap = today_layers->at(i)
                ->StatInfoMap();

        boost::unordered_map<string, StatInfo>::iterator it = wmap->begin();
        while (it != wmap->end()) {
            StatRateItem* item = stat_rate_model.add_items();
            uint32_t source_id = 0, view_type_id = 0, campaign_id = 0,
                    ad_id = 0, creative_id = 0, os_id = 0;
            char pos_id[100] = "";
            sscanf(it->first.c_str(), "%u`%u`%[^`]`%u`%u`%u`%u", &source_id,
                    &os_id, pos_id, &view_type_id, &campaign_id, &ad_id,
                    &creative_id);
            item->set_source_id(source_id);
            item->set_os_id(os_id);
            item->set_pos_id(pos_id);
            item->set_view_type_id(view_type_id);
            item->set_campaign_id(campaign_id);
            item->set_ad_id(ad_id);
            item->set_creative_id(creative_id);

            item->set_ctr((*wmap)[it->first].ctr);
            item->set_cvr((*wmap)[it->first].cvr);
            item->set_cpa((*wmap)[it->first].ecpa);

            item->set_cpm((*tmap)[it->first].cpm);
            item->set_cpc((*tmap)[it->first].cpc);
            item->set_impressions((*tmap)[it->first].imprs);
            item->set_costs((*tmap)[it->first].costs);
            item->set_clicks((*tmap)[it->first].clicks);
            item->set_binds((*tmap)[it->first].binds);

            it++;
        }
    }
    fstream fout(output_path.c_str(), ios::out | ios::trunc | ios::binary);
    if (!fout.good()) {
        LOG_ERROR("Fail to write %s", output_path.c_str());
        fout.close();
        return -1;
    }
    if (!stat_rate_model.SerializeToOstream(&fout)) {
        LOG_ERROR("Fail to serialize stat rate pb");
        fout.close();
        return -1;
    }
    fout.close();

    string md5str;
    Func::BinaryFileMD5(output_path.c_str(), &md5str);
    LOG_INFO("md5sum %s %s", output_path.c_str(), md5str.c_str());
    fstream fout_tag(output_tag.c_str(), ios::out | ios::trunc);
    fout_tag << md5str << "  " << file_name << endl;
    fout_tag.close();

    return 0;
}

