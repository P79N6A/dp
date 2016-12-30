#include "data_api/kv_reg_mgr.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    using namespace std;
    using namespace poseidon::mem_sync;

    int r = 0, n = 10, data_id = 101, max_key_size = 32, max_val_size = 256;

    KVRegMgr &mgr = KVRegMgr::get_mutable_instance();
    mgr.init("localhost:2181", "100.84.73.56", 25600);

    for (int i = 0; i < n; i++) {
        srand(time(NULL) + i);
        int key_size = rand() % max_key_size;

        srand(time(NULL) + i * 2);
        int val_size = rand() % max_val_size;

        char key[key_size];
        char val[val_size];

        memcpy(key, &i , key_size < 4 ? key_size : 4);

        mgr.put(key_size, key, val_size, val);
    }

    fprintf(stdout, "Press to reg_data\r\n");
    getchar();
    mgr.reg_data(data_id);

    fprintf(stdout, "Press to exit\r\n");
    getchar();

    return r;
}

