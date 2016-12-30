/*
 * echo_client.cpp
 * Created on: 2016-12-17
 */

#include "co2/client/tcp_client.h"
#include "co2/co2.h"

#include <iostream>

int main(int argc, char * argv[])
{
    co2::Init();

    using co2::client::TCPClient;
    TCPClient client;

    int res = client.Connect("127.0.0.1", 10000);
    if (res != 0) {
        fprintf(stderr, "connect failed, res = %d!\n", res);
        exit(0);
    }

    int i = 0;
    while (i++ < 10) {
        std::string s;
        res = client.WriteBytes("hello, world\n");
        if (res != 0) {
            fprintf(stderr, "write failed, res = %d\n", res);
            break;
        }

        res = client.ReadLine(s);
        if (res != 0) {
            fprintf(stderr, "readline failed, res = %d\n", res);
            break;
        }
        std::cout << s << std::endl;

        for (int j = 0; j < 10000; j++) {
            client.AppendBytes("Lua Python C++ ");
            client.AppendBytes("Java Ruby and C# ");
        }
        client.AppendBytes("\n");
        client.Flush();

        res = client.ReadLine(s);
        if (res != 0) {
            fprintf(stderr, "readline failed, res = %d\n", res);
            break;
        }
        std::cout << s << std::endl;
    }
    return 0;
}


