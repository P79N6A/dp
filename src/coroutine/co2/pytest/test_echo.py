import socket
import sys
import os

if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10000))
    
    data = '123456789' * int(sys.argv[1]) + '\r\n'
     
    print 'sending data...'
    repeat = int(sys.argv[2])
    i = 0
    while i < repeat: 
        i += 1
        s.send(data)
        res = s.recv(len(data))
        print i, res

