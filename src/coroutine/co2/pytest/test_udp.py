#coding: utf8
import os
import sys
import time
import socket

if __name__ == '__main__':
    process = int(sys.argv[1])
    loop = int(sys.argv[2])

    i = 0
    datas = [
        '1234567890' * 100 + '\n',
        'abcdefghij' * 100 + '\n', 
        'zphzphzph' * 100 + '\n', 
        '你好你好你好' * 100 + '\n'
    ]
    try:
        while i < process:
            i += 1
            pid = os.fork()
            if pid > 0:
                continue
            client = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
            j = 0
            while j < loop:
                j += 1
                client.sendto(datas[i % 4], "/tmp/coroutine")
                client.recvfrom(len(datas[i % 4]))
            os._exit(0)
    except Exception, e:
        print e.__class__.__name__, e

    while True:
        try:
            os.wait()
        except:
            break
