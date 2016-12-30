#include "data_api/kv_reg_mgr.h"
#include "config.h"
#include "util/log.h"
#include "data_api/kv_api.h"
#include "structs.h"
#include <stddef.h>
#include <stdio.h>
using namespace std;
using namespace poseidon::mem_sync;
using namespace poseidon::testtool;

int write_str();
int read_str();
int write_obj();
//复杂读写,包括重复，包括 null 值
int write_obj_complex();
int read_obj_complex();

int read_obj();
int write_big_obj();
int read_big_obj();
int write_null();
int write_miti();

int write_nomal();
int read_nomal();

int write_performance();
int read_performance();

int main(int argc, char *argv[]) {
	string operate = argv[1];
	string conf_file = "../conf/testtool.cfg";
	Config::get_mutable_instance().parse(conf_file);
	LOG_INIT(Config::get_mutable_instance().log_conf(),
			Config::get_mutable_instance().log_category());

	time_t start_time = time(0);
//	sleep(1);

//	string datetime;
//	util::Func::get_time_str(&datetime, "%Y%m%d");

	if (operate == "write_str") {
		write_str();
	} else if (operate == "read_str") {
		read_str();
	} else if (operate == "write_obj") {
		write_obj();
	} else if (operate == "read_obj") {
		read_obj();
	} else if (operate == "write_obj_complex") {
		write_obj_complex();
	} else if (operate == "read_obj_complex") {
		read_obj_complex();
	} else if (operate == "write_big_obj") {
		write_big_obj();
	} else if (operate == "read_big_obj") {
		read_big_obj();
	} else if (operate == "write_null") {
		write_null();
	} else if (operate == "write_miti") {
		write_miti();
	} else if (operate == "write_nomal") {
		write_nomal();
	} else if (operate == "read_nomal") {
		read_nomal();
	} else if (operate == "write_performance") {
		write_performance();
	} else if (operate == "read_performance") {
		read_performance();
	}
	time_t end_time = time(0);
	long const_time = end_time - start_time;
	LOG_INFO("[test time] %s const= [ %d s ]", operate.c_str(), const_time);

}
int write_performance() {
	//使用struct 测试

//	LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();
	int performance_test_size =
			Config::get_mutable_instance().performance_test_size();
	int performance_test_time =
			Config::get_mutable_instance().performance_test_time();

	for (int h = 0; h < performance_test_time; h++) {
		LOG_INFO("[const_time]time : %d /100", h);
		time_t start_time = time(0);
		long const_time;

		KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
		time_t end_time;
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]get_mutable_instance const_time=%d", const_time);

		//第一种init
		mgr.init(zk_ip_list, server_ip, server_port);
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]init const_time=%d", const_time);

		for (int i = 0; i < performance_test_size * 10000; i++) {
			StatRateKey s_key;
			StatRateValue s_value;
			s_key.source = i;
			s_value.binds = i;
			mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
					(char*) &s_value);
			//		LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
		}
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]write %d *10000 data const_time=%d",
				performance_test_size, const_time);

		mgr.reg_data(data_id);
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]reg_data const_time=%d", const_time);
	}
	return 0;
}

int write_nomal() {
	//使用struct 测试
	StatRateKey s_key;
	s_key.source = 1;
	StatRateValue s_value;
	s_value.binds = 1;
	LOG_INFO("operate is write_nomal");

	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();
	int big_obj_size = Config::get_mutable_instance().big_obj_size();

	LOG_INFO("[size] size : [ %d M ]",
			(sizeof(s_key) + sizeof(s_value)) * big_obj_size);

	for (int k = 0; k < 100000; k++) {
		KVApi *kv_api = new KVApi();
		kv_api->init();
		int my_version = kv_api->get_version(data_id) + 1;
		delete kv_api;

		KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
		mgr.init(zk_ip_list, server_ip, server_port);

		//写入 60M  个 key值
		for (int i = 0; i < 1024 * big_obj_size; i++) {
			StatRateKey s_key;
			StatRateValue s_value;
			s_key.source = i;
			s_value.binds = i + my_version;
			mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
					(char*) &s_value);
			//		LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
		}

		mgr.reg_data(data_id);
		sleep(60 * 10);
	}
	return 0;
}

int write_big_obj() {

	//使用struct 测试
	StatRateKey s_key;
	s_key.source = 1;
	StatRateValue s_value;
	s_value.binds = 1;
	LOG_INFO("operate is write_big_obj");

	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();
	int big_obj_size = Config::get_mutable_instance().big_obj_size();

	KVApi *kv_api = new KVApi();
	kv_api->init();
	int my_version = kv_api->get_version(data_id) + 1;

	LOG_INFO("[size] size : [ %d M ]",
			(sizeof(s_key) + sizeof(s_value)) * big_obj_size);

	KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
	mgr.init(zk_ip_list, server_ip, server_port);
	string key = "key";
	string value = "value";
	//写入 60M  个 key值
	for (int i = 0; i < 1024 * big_obj_size; i++) {
		StatRateKey s_key;
		StatRateValue s_value;
		s_key.source = i;
		s_value.binds = i + my_version;
		mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
				(char*) &s_value);
//		LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
	}

	mgr.reg_data(data_id);
	return 0;
}

int write_obj_complex() {
	//使用struct 测试
	StatRateKey s_key;
	s_key.source = 1;
	StatRateValue s_value;
	s_value.binds = 1;
	LOG_INFO("operate is write_obj2");

	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();

	KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
	for (int i = 0; i < 100; i++) {
		//第一种init
		mgr.init(zk_ip_list, server_ip, server_port);
		//第二种init
		mgr.init(zk_ip_list, server_port);
	}
	string key = "key";
	string value = "value";
	//多次擦除
	for (int i = 0; i < 100; i++) {
		//写入相同值
		for (int j = 0; j < 100; j++) {
			mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
					(char*) &s_value);
		}
		mgr.reset();
	}
	//写 null 值
	mgr.put(0, NULL, sizeof(s_value), (char*) &s_value);

	//写入 100000  个 key值
	for (int i = 0; i < 10000; i++) {
		StatRateKey s_key;
		StatRateValue s_value;
		s_key.source = i;
		s_value.binds = i;
		mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
				(char*) &s_value);
		//标准化 log
		LOG_INFO("[test_info][write]key = %d,value = %d", s_key.source,
				s_value.binds);
	}
	mgr.reg_data(data_id);
	return 0;
}
int write_null() {
	//使用struct 测试
	StatRateKey s_key;
	StatRateValue s_value;
	s_key.source = 2;
	s_value.binds = 1;

	LOG_INFO("operate is write_obj");

	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();

	KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
	//第一种init
	mgr.init(zk_ip_list, server_ip, server_port);

	mgr.put(sizeof(s_key), (char*) &s_key, 0, NULL);
	mgr.put(0, NULL, sizeof(s_value), (char*) &s_value);
	mgr.put(0, NULL, 0, NULL);

	s_value.binds = 2;
	LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
	mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value), (char*) &s_value);

	mgr.reg_data(data_id);
	return 0;
}
int write_miti() {
	//使用struct 测试

	LOG_INFO("operate is write_miti");

	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();

	for (int i = 0; i < 10; i++) {
		KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
		//第一种init
		mgr.init(zk_ip_list, server_ip, server_port);
		StatRateKey s_key;
		StatRateValue s_value;
		s_key.source = i;
		s_value.binds = i;
		mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value),
				(char*) &s_value);
		mgr.reg_data(data_id);
	}

	LOG_INFO("[write]key=%d,value = %d", 9, 9);
	return 0;
}

int write_obj() {
	//使用struct 测试
	StatRateKey s_key;
	StatRateValue s_value;
	s_key.source = 2;
	s_value.binds = 1;

	LOG_INFO("operate is write_obj");
	LOG_INFO("[write]key=%d,value = %d", s_key.source, s_value.binds);
	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();

	KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
	//第一种init
	mgr.init(zk_ip_list, server_ip, server_port);
	string key = "key";
	string value = "value";
	mgr.put(sizeof(s_key), (char*) &s_key, sizeof(s_value), (char*) &s_value);
	mgr.reg_data(data_id);
	return 0;
}
int read_big_obj() {
	LOG_INFO("operate is read_big_obj");
	int data_id = Config::get_mutable_instance().data_id();
	int big_obj_size = Config::get_mutable_instance().big_obj_size();
	KVApi *kv_api = new KVApi();
	for (int i = 0; i < 100; i++) {
		kv_api->init();
	}
	int my_version = kv_api->get_version(data_id);
	LOG_INFO("version is [%d]", my_version);
	int success=0,error=0;
	for (int i = -10; i < 1024 * big_obj_size + 10; i++) {
		StatRateKey s_key;
		s_key.source = i;
		char *c_val;
		int version = 0;
		size_t val_size;
		int state = kv_api->get(data_id, sizeof(s_key), (const char*) &s_key,
				val_size, c_val, &version);


		if (state == 0) {
			StatRateValue *s_val = (StatRateValue *) c_val;
			if (s_val->binds != i + version) {
				LOG_INFO("[test_info][test_error]key : %d ,value : %d expect : %d ", i, s_val->binds,
						i + version);
				error++;
			}else{
				success++;
			}
		} else {
			LOG_INFO("[NO FIND] key = %d", i);
		}

	}
	LOG_INFO("[test_info][test_result]success : %d ,error : %d ", success, error);
//	KVIter *iter = kv_api->get_iter(data_id);
//	if (iter != NULL) {
//		size_t key_size, val_size;
//		char *key, *value;
//		while (iter->next(key_size, key, val_size, value) == 0) {
//			StatRateKey *s_key = (StatRateKey *) key;
//			StatRateValue *s_val = (StatRateValue *) value;
//			if (s_val->binds != s_key->source * my_version) {
//				LOG_INFO("[ERROR] key : %d ,value : %d ",
//						s_key->source * my_version, s_val->binds);
//			}
//		}
//		delete iter;
//	}

	delete kv_api;
	return 0;
}
int read_performance() {
	int data_id = Config::get_mutable_instance().data_id();
	int big_obj_size = Config::get_mutable_instance().big_obj_size();
	int performance_test_size =
			Config::get_mutable_instance().performance_test_size();
	int performance_test_time =
			Config::get_mutable_instance().performance_test_time();

//	int my_version = kv_api->get_version(data_id);
	LOG_INFO("data_id is [%d]", data_id);
	for (int h = 0; h < performance_test_time; h++) {

		time_t start_time = time(0);
		long const_time;
		LOG_INFO("[const_time]time : %d /100", h);
		KVApi *kv_api = new KVApi();
		kv_api->init();
		int my_version = kv_api->get_version(data_id);
		LOG_INFO("[version] is [%d]", my_version);

		time_t end_time;
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]kv_api init const_time=%d", const_time);

//		for (int i = 0; i > -10000 * performance_test_size; i--) {
//			StatRateKey s_key;
//			s_key.source = i;
//			char *c_val;
//			int version = 0;
//			size_t val_size;
//			int state = kv_api->get(data_id, sizeof(s_key),
//					(const char*) &s_key, val_size, c_val, &version);
//			if (state == 0) {
//				StatRateValue *s_val = (StatRateValue *) c_val;
//			} else {
//			}
//
//		}
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]get 10000 * %d null const_time=%d",
				performance_test_size, const_time);

		for (int i = 0; i < performance_test_size * 10000; i++) {
			StatRateKey s_key;
			s_key.source = i;
			char *c_val;
			int version = 0;
			size_t val_size;
			int state = kv_api->get(data_id, sizeof(s_key),
					(const char*) &s_key, val_size, c_val, &version);
			if (state == 0) {
				StatRateValue *s_val = (StatRateValue *) c_val;
				if (s_val->binds != i) {
					LOG_INFO("[ERROR] key : %d ,value : %d ", i, s_val->binds);
				}
			} else {
				LOG_INFO("[NO FIND] key = %d", i);
			}

		}
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]get 10000 * %d value const_time=%d",
				performance_test_size, const_time);

		delete kv_api;
		end_time = time(0);
		const_time = end_time - start_time;
		LOG_INFO("[const_time]delete kv_api null const_time=%d", const_time);

	}
	return 0;
}
int read_nomal() {
	LOG_INFO("operate is read_nomal");
	int data_id = Config::get_mutable_instance().data_id();
	int big_obj_size = Config::get_mutable_instance().big_obj_size();
	for (int k = 0; k < 10000; k++) {
		KVApi *kv_api = new KVApi();
		for (int i = 0; i < 100; i++) {
			kv_api->init();
		}
		int my_version = kv_api->get_version(data_id);
		LOG_INFO("local version is [%d]", my_version);
		for (int i = -10; i < 1024 * big_obj_size + 10; i++) {
			//	for (int i = 0; i < 1024 * big_obj_size; i++) {
			StatRateKey s_key;
			s_key.source = i;
			char *c_val;
			int version = 0;
			size_t val_size;
			int state = kv_api->get(data_id, sizeof(s_key),
					(const char*) &s_key, val_size, c_val, &version);
			if (state == 0) {
				StatRateValue *s_val = (StatRateValue *) c_val;
				if (s_val->binds != (i + version)) {
					LOG_INFO(
							"[ERROR] key : %d ,version: %d , value : %d ,expect : %d ",
							i, version, s_val->binds, i + version);
				}
			} else {
				LOG_INFO("[NO FIND] key = %d", i);
			}

		}
		KVIter *iter = kv_api->get_iter(data_id);
		if (iter != NULL) {
			size_t key_size, val_size;
			char *key, *value;
			while (iter->next(key_size, key, val_size, value) == 0) {
				StatRateKey *s_key = (StatRateKey *) key;
				StatRateValue *s_val = (StatRateValue *) value;
//				if (s_val->binds != s_key->source * my_version) {
//					LOG_INFO("[ERROR] key : %d ,value : %d ",
//							s_key->source * my_version, s_val->binds);
//				}
			}
		}
		delete iter;
		iter = kv_api->get_iter(data_id);
		if (iter != NULL) {
			size_t key_size, val_size;
			char *key, *value;
			while (iter->next(key_size, key, val_size, value) == 0) {
				StatRateKey *s_key = (StatRateKey *) key;
				StatRateValue *s_val = (StatRateValue *) value;
//				if (s_val->binds != s_key->source * my_version) {
//					LOG_INFO("[ERROR] key : %d ,value : %d ",
//							s_key->source * my_version, s_val->binds);
//				}
			}
		}
		delete iter;
		delete kv_api;
		sleep(60);
	}
	return 0;
}

int read_obj_complex() {
	LOG_INFO("operate is read_null");
	int data_id = Config::get_mutable_instance().data_id();
	KVApi *kv_api = new KVApi();
	for (int i = 0; i < 100; i++) {
		kv_api->init();
	}
	for (int i = 0; i < 100; i++) {
		int version = kv_api->get_version(data_id);
		LOG_INFO("version is [%d]", version);
	}

	for (int i = -100; i < 10100; i++) {
		StatRateKey s_key;
		StatRateValue * s_val;
		size_t val_size, key_size = sizeof(s_key);

		s_key.source = i;
		char * c_val;
		int ret = kv_api->get(data_id, sizeof(s_key), (char*) &s_key, val_size,
				c_val);
		if (ret == 0) {
			s_val = (StatRateValue *) c_val;
			LOG_INFO("[test_info][read]key = %d,value = %d", s_key.source,
					s_val->binds);
		}
	}

	delete kv_api;
	return 0;
}

int read_obj() {
	LOG_INFO("operate is read_obj");
	int data_id = Config::get_mutable_instance().data_id();
	KVApi *kv_api = new KVApi();
	kv_api->init();
	int version = kv_api->get_version(data_id);
	LOG_INFO("version is [%d]", version);
	size_t key_size, value_size;
	char *key, *value;
	KVIter *iter = kv_api->get_iter(data_id);
//    	int total=0;
	if (iter != NULL) {
		while (iter->next(key_size, key, value_size, value) == 0) {
			//    		 total += ((int)ks + (int)vs);
			StatRateKey *s_key = (StatRateKey *) key;
			StatRateValue *s_val = (StatRateValue *) value;
//			LOG_INFO("key length = %d, key=%d, value length = %d ,value = %d ",
//					key_size, s_key->source, value_size, s_val->binds);
			LOG_INFO("[read]key=%d,value = %d", s_key->source, s_val->binds);
		}
		delete iter;
	}

	delete kv_api;
	return 0;
}

int read_str() {

	LOG_INFO("operate is read_str");
	int data_id = Config::get_mutable_instance().data_id();

	KVApi *kv_api = new KVApi();
	kv_api->init();
	int version = kv_api->get_version(data_id);
	LOG_INFO("version is [%d]", version);
	size_t key_size, value_size;
	char *key, *value;
	KVIter *iter = kv_api->get_iter(data_id);
//    	int total=0;
	if (iter != NULL) {
		while (iter->next(key_size, key, value_size, value) == 0) {
			//    		 total += ((int)ks + (int)vs);
			LOG_INFO(
					"key length = %zd, key=%.*s, value length = %zd ,value = %.*s ",
					key_size, (int )key_size, key, value_size, (int )value_size,
					value);
		}
		delete iter;
	}
	delete kv_api;
	return 0;
}

int write_str() {

	LOG_INFO("operate is write_str");
	string zk_ip_list = Config::get_mutable_instance().zk_ip_list();
	string server_ip = Config::get_mutable_instance().server_ip();
	int server_port = Config::get_mutable_instance().server_port();
	int data_id = Config::get_mutable_instance().data_id();

	KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
//第一种init
	mgr.init(zk_ip_list, server_ip, server_port);
	string key = "key";
	string value = "value";
	mgr.put(key, value);
	mgr.reg_data(data_id);
	return 0;
}
//bool GetStatRateValue(const StatRateKey& key, StatRateValue** value) {
//	size_t val_size = 0;
//    char* val = NULL;
//    if (0 == m_kv_api.get(m_config.stat_rate_config().data_id(),
//    		sizeof(StatRateKey), (const char*) &key, val_size, val)) {
//            *value = (StatRateValue*) val;
//            return true;
//    }
//    return false;
//}

