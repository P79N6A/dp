/**
 **/
#include "api/exp_api.h"
#include <stdio.h>
#include "gflags/gflags.h"
using namespace std;

DEFINE_int32(shm_key, 13579, "共享内存key");
DEFINE_int32(shm_size, 20 * 1024 * 1024, "共享内存key");

DEFINE_int32(module_id, 10001, "module_id");
DEFINE_int32(source_id, 1, "source_id");
DEFINE_int32(view_type, 103, "view_type");
DEFINE_bool(test_shm, false, "此值设置为ture时，直接测试共享内存");

int main(int argc, char * argv[]) {
	::google::ParseCommandLineFlags(&argc, &argv, true);
	int rt = 0;
	do {
		if (FLAGS_test_shm) {
			cout << "exp init,shm_key : " << FLAGS_shm_key << " shm_size : "
					<< FLAGS_shm_size << endl;
			if (!EXP_API().init(FLAGS_shm_key,FLAGS_shm_size))
			{
				std::cout<<"EXP_API init error"<<std::endl;
				rt=-1;
				break;
			}
		}
		else
		{
			if(!EXP_API().init())
			{
				std::cout<<"EXP_API init error"<<std::endl;
				rt=-1;
				break;
			}
		}

		cout<<"search module : "<<FLAGS_module_id<<" source : "<<FLAGS_source_id<<" view_type  : "<<FLAGS_view_type<<endl;

		int i;
		for (i = 0; i < 1000; i++) {
			std::vector<int> exp_id;
			std::vector<poseidon::exp_sys::ExpParam> exp_param;
			rt=EXP_API().get_exp_param(FLAGS_module_id, FLAGS_source_id, FLAGS_view_type, exp_id, exp_param);
			if(rt != 0)
			{
				fprintf(stderr, "api.get_exp_param error[%d]\n", rt);
				continue;
			}
			std::vector<int>::iterator it;
			for(it = exp_id.begin(); it!=exp_id.end(); it++ )
			{
				std::cout<<"exp_id:"<<*it<<"|";
			}
			std::cout<<std::endl;

			std::vector<poseidon::exp_sys::ExpParam>::iterator itp;
			for(itp = exp_param.begin(); itp!=exp_param.end(); itp++ )
			{
				std::cout<<"exp_param:"<<itp->param_id<<":"<<itp->param_type<<":";
				if(itp->param_type==poseidon::exp_sys::PT_INT)
				{
					std::cout<<itp->param_vlaue.int_v<<std::endl;
				}
				if(itp->param_type==poseidon::exp_sys::PT_FLOAT)
				{
					std::cout<<itp->param_vlaue.float_v<<std::endl;
				}
			}
		}
	}while(0);
	return rt;
}

