/*
 * main.cpp
 * Created on: 2016-10-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <wait.h>
#include <sys/types.h>

#include "comm_event_factory.h"
#include "memory_mgr.h"
#include "data_server.h"
#include "config.h"
#include "util/log.h"
#include "util/func.h"

#define VERSION "dataserver version 1.0.1"

void usage(const char * program) {
	fprintf(stderr, "%s [options]\n", program);
	fprintf(stderr, "    -h|--help show help\n");
	fprintf(stderr, "    -v|--version show version\n");
	fprintf(stderr, "    -c|--conf specify the config file\n");
	fprintf(stderr, "    -f|--foreground run in foreground\n");
	exit(1);
}

int run(poseidon::mem_sync::Config &config,
	dc::common::comm_event::CommFactoryInterface & factory)
{
	return factory.run();
}

int init(poseidon::mem_sync::Config &config,
	dc::common::comm_event::CommFactoryInterface & factory) {

	/* daemonize this process */
	int res = 0;
	do {
		if (!config.GetForeGround())
			poseidon::util::Func::DaemonInit();

		if (!LOG_INIT(config.LogConf(), config.LogCategory())) {
			fprintf(stderr, "LOG_INIT error[%s, %s]\n",
				config.LogConf(),
				config.LogCategory());
			res = -1;
			break;
		}

		if (!poseidon::util::Func::single_instance(config.PidFile())) {
			LOG_ERROR("Create PidFile failed!");
			break;
		}

		res = factory.init();
		if (res != 0) {
			LOG_ERROR("CommFactoryInterface::instance().init() return error\n");
			break;
		}

		/* init network framework */
		using poseidon::mem_sync::server::DataServer;
		DataServer &server = DataServer::get_mutable_instance();

		res = factory.add_comm_tcp(&server);
		if (res != 0) {
			LOG_ERROR("add_comm_tcp error\n");
			break;
		}

		res = server.init();
		if (res != 0) {
			LOG_ERROR("init DataServer failed!\n");
			break;
		}

		server.listen_on_addr(config.LocalIP(), config.ServerPort(), 512);
		if (config.GetForeGround()) {
			/* in the case of foreground, only 1 process is allowed */
			using poseidon::mem_sync::server::MemoryMgr;
			MemoryMgr &mgr = MemoryMgr::get_mutable_instance();
			mgr.Init(config.ZKList(), config.LocalIP(), config.ServerPort());
			factory.re_init();
			exit(run(config, factory));
		}

		int c, process = config.WorkerCount();
		std::map<pid_t, int> proc_map;

		for (c = 0; c < process; c++) {
			pid_t pid = fork();
			if (pid > 0) {
				proc_map[pid] = c;
			} else if (pid == 0) {
				/* Worker Run */
				config.ProcessIdx(c);
				using poseidon::mem_sync::server::MemoryMgr;
				MemoryMgr &mgr = MemoryMgr::get_mutable_instance();
				mgr.Init(config.ZKList(), config.LocalIP(), config.ServerPort());
				factory.re_init();
				exit(run(config, factory));
			} else {
				LOG_ERROR("fork error: %s", strerror(errno));
				exit(-1);
			}
		}

		while (true) {
			int status;
			pid_t pid = wait(&status);
			if (pid < 0) {
				/* DO NOTHING */;
			} else {
				int index = proc_map[pid];
				LOG_ERROR("child process exit, rerun: index[%d] pid[%d]", index, pid);
				pid = fork();
				if (pid > 0) {
					proc_map[pid] = index;
				} else if (pid == 0) {
					/* Worker Run */
					config.ProcessIdx(index);
					using poseidon::mem_sync::server::MemoryMgr;
					MemoryMgr &mgr = MemoryMgr::get_mutable_instance();
					mgr.Init(config.ZKList(), config.LocalIP(), config.ServerPort());
					factory.re_init();
					exit(run(config, factory));
				}
			}
		}
		exit(0);
	} while (0);
	return res;
}

int main(int argc, char * argv[])
{
	int c;
	struct option long_options[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "conf", required_argument, NULL, 'c' },
		{ "version", no_argument, NULL, 'v' },
		{ "foreground", no_argument, NULL, 'f' },
		{ NULL, 0, NULL, 0 } // sentinel
	};

	poseidon::mem_sync::Config &config =
		poseidon::mem_sync::Config::get_mutable_instance();

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
}
