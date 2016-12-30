# @brief: test server 

import sys
import os
import getopt

def usage():
    print >> sys.stderr, "usage: python test_server [options]"
    print >> sys.stderr, "  -i | --dataid: specify dataid"
    print >> sys.stderr, "  -v | --version: specify version"
    print >> sys.stderr
    os._exit(1)

def get_data(dataid, version):
    import socket
    import struct
    import ConfigParser

    cf = ConfigParser.ConfigParser()
    cf.read('test.conf')

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # sock.connect(('192.168.24.130', 25600))
    host = cf.get('ipc', 'host') 
    port = cf.getint('ipc', 'port')
    sock.connect((host, port))

    # dataid, version, offset, size
    data = struct.pack('!3s2i2qs', '(MS', dataid, version, 0, 0, ')')
    print 'sending data %s to server ... ' % data
    
    sock.send(data)
    data = sock.recv(24)
    print len(data)
    res = struct.unpack('!3s3iqs', data)
    print res

    # res = ( '(MS', result, dataid, version, size, ')' )
    (_, result, dataid, version, size, _) = res

    if result == -1:
        print 'no data'
    else:
        read = 0
        while size > 0:
            data = sock.recv(size)
            data_len = len(data)
            if data_len == 0:
                break

            read += data_len
            size -= data_len
            print 'recv %s bytes from server, %s bytes left' % (read, size)


if __name__ == '__main__':
    dataid = 1
    version = 1

    opts, args = getopt.getopt(sys.argv[1:], "i:v:h", 
        ["dataid=", "version=", "help"])

    for k, v in opts:
        if k in ('-i', '--dataid'):
            dataid = int(v)
        elif k in ('-v', '--version'):
            version = int(v)
        else:
            usage()
    get_data(dataid, version)
