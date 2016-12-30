# @brief: test SharedMemory 

import sys
import os
import getopt
import traceback

def usage():
    print >> sys.stderr, "usage: python test_shm [options]"
    print >> sys.stderr, "  -k | --key : specify share memory's ipc key"
    print >> sys.stderr, "  -i | --dataid: specify dataid"
    print >> sys.stderr, "  -v | --version: specify version"
    print >> sys.stderr, "  -d | --data: to write to shared memory"
    print >> sys.stderr, "  -r | --repeat: write (repeat) copies of data"
    print >> sys.stderr
    os._exit(1)

def update_shm(shm_key, dataid, version, data, repeat=1):
    # modify the data_server_addr to test your own case
    
    # create shared memory
    import sysv_ipc
    shm = sysv_ipc.SharedMemory(shm_key,
            flags=sysv_ipc.IPC_CREAT, 
            mode=0600, 
            size=(len(data)*repeat))
    shm.attach()

    n = 0
    while n < repeat:
        shm.write(data, n * len(data))
        n += 1
    shm.detach()

    # create MD5
    import hashlib
    md5 = hashlib.md5()
    md5.update(data * repeat)
    check_sum = md5.hexdigest()

    # connect to zk and set values
    from kazoo.client import KazooClient
    from kazoo.exceptions import NoNodeError, NodeExistsError

    import ConfigParser
    cf = ConfigParser.ConfigParser()
    cf.read('test.conf')
    zklist = cf.get('ipc', 'zklist')
    host_port = '%s:%s' % (cf.get('ipc', 'host'), cf.getint('ipc', 'port'))
    zkcli = KazooClient(hosts=cf.get('ipc', 'zklist'))
    zkcli.start()

    try:
        zkcli.create('/mem_sync/%s/%s' % (dataid, version), '', makepath=True)
    except NodeExistsError:
        pass

    try:
        zkcli.create('/mem_sync/%s/%s/shm_key' % (dataid, version), str(shm_key))
    except NodeExistsError:
        zkcli.set('/mem_sync/%s/%s/shm_key' % (dataid, version), str(shm_key))

    try:
        zkcli.create('/mem_sync/%s/%s/shm_size' % (dataid, version), str(len(data) * repeat))
    except NodeExistsError:
        zkcli.set('/mem_sync/%s/%s/shm_size' % (dataid, version), str(len(data) * repeat))

    try:
        zkcli.create('/mem_sync/%s/%s/check_sum' % (dataid, version), check_sum)
    except NodeExistsError:
        zkcli.set('/mem_sync/%s/%s/check_sum' % (dataid, version), check_sum)

    try:
        zkcli.create('/mem_sync/%s/%s/data_server_addr' % (dataid, version), 
            host_port)
    except NodeExistsError:
        zkcli.set('/mem_sync/%s/%s/data_server_addr' % (dataid, version), 
            host_port)

    try:
        zkcli.create('/mem_sync/%s/config/version' % (dataid,), str(version), makepath=True)
    except NodeExistsError:
        ver, _ = zkcli.get('/mem_sync/%s/config/version' % (dataid,))
        ver = int(ver)
        if ver < version:
            zkcli.set('/mem_sync/%s/config/version' % (dataid,), str(version))
    zkcli.stop()
 
if __name__ == '__main__':
    shm_key = None
    data = None
    dataid = None
    version = None
    repeat = 1

    opts, args = getopt.getopt(sys.argv[1:], "i:v:d:k:r:", 
        ["dataid=", "version=", "data=", "key=", "repeat="])

    for k, v in opts:
        if k in ('-d', '--data'):
            data = v
        elif k in ('-i', '--dataid'):
            dataid = int(v)
        elif k in ('-v', '--version'):
            version = int(v)
        elif k in ('-k', '--key'):
            shm_key = int(v)
        elif k in ('-r', '--repeat'):
            repeat = int(v)

    print 'shm_key =', shm_key
    print 'dataid =', dataid        
    print 'version =', version
    print 'data =', data
    print 'repeat =', repeat
    print 

    if dataid is None or version is None:
        usage()

    if shm_key is None:
        shm_key = 10000 * dataid + version

    if data is None:
        data = 'dataid = %s, version = %s' % (dataid, version)

    if dataid < 0 or version < 0:
        print >> sys.stderr, 'dataid and version must be >= 0'
        os._exit(1)

    try:
        update_shm(shm_key, dataid, version, data, repeat) 
    except:
        print >> sys.stderr, traceback.format_exc()
