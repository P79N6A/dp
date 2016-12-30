/*
 * main.cpp
 * Created on: 2016-12-16
 */

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <wait.h>

#include "config.h"
#include "util/log.h"
#include "util/func.h"

#include "ha/ha.h"
#include "co2/co2.h"
#include "fb_co/fb_server.h"

/* CO ROUTINE VERSION OF FEEDBACK SERVICE */
#define VERSION "fb_server version 2.0.0"

void usage(const char * program) {
    fprintf(stderr, "%s [options]\n", program);
    fprintf(stderr, "    -h|--help show help\n");
    fprintf(stderr, "    -v|--version show version\n");
    fprintf(stderr, "    -c|--conf specify the config file\n");
    fprintf(stderr, "    -f|--foreground run in foreground\n");
    exit(1);
}

struct option long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "conf", required_argument, NULL, 'c' },
    { "version", no_argument, NULL, 'v' },
    { "foreground", no_argument, NULL, 'f' },
    { NULL, 0, NULL, 0 } // sentinel
};

using poseidon::feedback::Config;
using poseidon::feedback::FBServer;

int Run(Config &config)
{
    int res = -1;
    if (!config.GetForeground())
        poseidon::util::Func::DaemonInit();

    if (!LOG_INIT(config.LogConf(), config.LogCategory())) {
        fprintf(stderr, "LOG_INIT error[%s, %s]\n",
            config.LogConf(),
            config.LogCategory());
        return res;
    }

    if (!poseidon::util::Func::single_instance(config.PidFile())) {
        LOG_ERROR("Create PidFile failed!");
        return res;
    }

    co2::Init();
    FBServer server;
    if (server.Init() != 0) {
        LOG_ERROR("Server Init failed!");
        return res;
    }

    server.GetRedisPool().SetMaxConnection(config.RedisPoolSize());
    server.GetRedisPool().AddServer(config.RedisList());
    res = server.Bind(config.LocalIP(), config.Port());

    if (res != 0) {
        LOG_ERROR("Bind[%s:%d] failed!", config.LocalIP(), config.Port());
        return res;
    }

    LOG_INFO("FBServer Starting ...");
    int c, process = config.WorkerCount();
    std::map<pid_t, int> proc_map;

    for (c = 0; c < process; c++) {
        pid_t pid = fork();
        if (pid > 0) {
            proc_map[pid] = c;
        } else if (pid == 0) {
            /* Worker Run */
            config.ProcessIdx(c);
            if (config.HaOn()) {
                if (0 != HA_INIT(config.ZKList())) {
                    LOG_ERROR("Init HA failed!");
                    return res;
                }
                if (0 != HA_REG("fb", config.LocalIP(), config.Port())) {
                    LOG_ERROR("Reg to HA failed!");
                    return res;
                }
            }
            exit(server.ServeForever());
        } else {
            LOG_ERROR("fork error: %s", strerror(errno));
            break;
        }
    }

    /* only parent can reach here. */
    while (proc_map.size()) {
        int status;
        pid_t pid = wait(&status);
        if (pid < 0) {
            /* DO NOTHING */;
        } else {
            int index = proc_map[pid];
            LOG_ERROR("child process exit, rerun: index[%d] pid[%d]",
                index, pid);
            proc_map.erase(pid);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    Config &config = Config::get_mutable_instance();

    int c;
    while ((c = getopt_long(argc, argv, "hvc:f", long_options, NULL)) != -1) {
        switch (c) {
        default:
        case 'h':
            usage(argv[0]);
            break;
        case 'f':
            config.SetForeground(true);
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
    exit(Run(config));
}
