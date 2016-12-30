#include "data_api/kv_api.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    using namespace std;
    using namespace poseidon::mem_sync;

    char bf[4096];
    int r = 0, n = 10, data_id = 101;

    KVApi *ka = new KVApi();
    ka->init();

    fprintf(stdout, "Press to get version\r\n");
    getchar();
    int ver = ka->get_version(data_id);
    fprintf(stderr, "version=%d\r\n", ver);

    fprintf(stdout, "Press to do get\r\n");
    getchar();

    string val;
    for (int i = 0; i < n; i++) {
        sprintf(bf, "%d:kkkkkkkkkkkkkkk", i);
        string key(bf);

        r = ka->get(data_id, key, val);
        if (r != 0) {
            fprintf(stderr, "error: r=%d ks=%d, key=%.*s\r\n", 
                    r, (int)key.size(), (int)key.size(), key.data());
        }
    }

    fprintf(stdout, "Press to do iteration\r\n");
    getchar();
    size_t ks, vs;
    char *k, *v;
    KVIter *iter = ka->get_iter(data_id);
    while ((r = iter->next(ks, k, vs, v)) == 0) {
        fprintf(stderr, "ks=%zd, k=%.*s, vs=%zd, val=%.*s\r\n", ks, (int)ks, k, vs, (int)vs, v);
        sleep(1);
    }

    fprintf(stdout, "Press to release ka\r\n");
    getchar();
    delete ka;

    fprintf(stdout, "Press to exit\r\n");
    getchar();

    return r;
}

 
