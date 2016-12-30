/*
 * main.cpp
 * Created on: 2016-10-24
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "comm_event_factory.h"
#include "data_agent.h"
#include "config.h"
#include "util/log.h"
#include "util/func.h"
#include "storage.h"
#include "zk_client.h"
#include "monitor_api.h"

#define VERSION "dataagent version 1.1.0"

void usage(const char * program) {
    fprintf(stderr, "%s [options]\n", program);
    fprintf(stderr, "    -h|--help show help\n");
    fprintf(stderr, "    -c|--conf specify the config file\n");
    fprintf(stderr, "    -f|--forground run in foreground\n");
    fprintf(stderr, "    -v|--version show version\n");
    exit(1);
}

int init(poseidon::mem_sync::agent::Config &config,
    dc::common::comm_event::CommFactoryInterface & factory) {

    /* daemonize this process */
    int res = 0;
    do {
        if (!config.GetForeGround())
            poseidon::util::Func::DaemonInit();

        if (!LOG_INIT(config.LogConf(), config.LogCategory())) {
            fprintf(stderr, "LOG_INIT error[%s, %s]\n", config.LogConf(),
                config.LogCategory());
            res = -1;
            break;
        }

        res = poseidon::monitor::Api::get_mutable_instance().init();
        if (res != 0) {
            LOG_ERROR("MonitorAPI init failed!");
            /* don't break, just let monitor failed. */
        }

        if (!poseidon::util::Func::single_instance(config.PidFile())) {
            LOG_ERROR("Create PidFile failed!");
            res = -1;
            break;
        }

        poseidon::mem_sync::agent::Storage & storage =
            poseidon::mem_sync::agent::Storage::get_mutable_instance();

        res = storage.Init(config.AgentConf());
        if (res != 0) {
            LOG_ERROR("Storage Init failed!");
            break;
        }

        res = factory.init();
        if (res != 0) {
            LOG_ERROR("CommFactoryInterface::instance().init() return error\n");
            break;
        }

        /* init network framework */
        using poseidon::mem_sync::agent::DataAgent;
        DataAgent &agent = DataAgent::get_mutable_instance();
        res = factory.add_comm_tcp(&agent);
        if (res != 0) {
            LOG_ERROR("ADD TCP failed");
            break;
        }

        agent.InitDataIDs(storage.GetDataIDs());

        res = agent.init();
        if (res != 0) {
            LOG_ERROR("Agent Init failed!");
            break;
        }

    } while (0);
    return res;
}

int run(poseidon::mem_sync::agent::Config &config,
    dc::common::comm_event::CommFactoryInterface & factory) {

    return factory.run();
}

int main(int argc, char * argv[]) {
    int c;
    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        {"conf", required_argument, NULL, 'c' },
        { "version", no_argument, NULL, 'v' },
        { "foreground", no_argument, NULL, 'f' },
        { NULL, 0, NULL, 0 }  // sentinel
    };

    poseidon::mem_sync::agent::Config &config =
        poseidon::mem_sync::agent::Config::get_mutable_instance();

    dc::common::comm_event::CommFactoryInterface &factory =
        dc::common::comm_event::CommFactoryInterface::instance();

    while ((c = getopt_long(argc, argv, "hvc:f", long_options, NULL)) != -1) {
        switch (c) {
        default:
        case 'h':
            usage(argv[0]);
            break;
        case 'f':
            config.SetForeGround(true);
            break;
        case 'c':
            config.Parse(optarg);
            break;
        case 'v':
            fprintf(stdout, "%s\n", VERSION);
            exit(1);
        }
    }

    if (!config.IsParse()) {
        fprintf(stderr, "conf must be specified!\n");
        usage(argv[0]);
    }
    /* 0 for OK */
    if (init(config, factory) != 0) {
        fprintf(stderr, "init failed!\n");
        exit(1);
    }
    exit(run(config, factory));
}

