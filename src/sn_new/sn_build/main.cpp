/**
 **/

#include <stdio.h>

#include "gflags/gflags.h"
#include "invertedindex_builder.h"

#define LOG_ERROR(fmt, a...) fprintf(stderr, "[%d in %s]" fmt, __LINE__, __FILE__, ##a)

DEFINE_string(data, "index_data.txt", "索引数据文件");
DEFINE_string(zk, "127.0.0.1:2181", "zk_iplist");
DEFINE_int32(dsport, 20000, "data_server port");

int main(int argc, char *argv[])
{
    int rt = 0;
    do {
        google::SetVersionString("1.0.0");
        google::ParseCommandLineFlags(&argc, &argv, true);

        rt = poseidon::sn::InvertedIndexBuilder::get_mutable_instance().Init(
            FLAGS_zk, FLAGS_dsport);
        if (rt != 0) {
            break;
        }
        rt = poseidon::sn::InvertedIndexBuilder::get_mutable_instance()
                 .BuildInvertedIndex(FLAGS_data);
        if (rt != 0) {
            break;
        }

    } while (0);
    return rt;
}
