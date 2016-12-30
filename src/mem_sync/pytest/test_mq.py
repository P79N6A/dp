# @brief: test MessageQueue

import sys
import os
import getopt
import traceback

def usage():
    print >> sys.stderr, "usage: python test_mq [options]"
    print >> sys.stderr, "  -k | --key : specify MQ's ipc key"
    print >> sys.stderr, "  -t | --type : specify MQ's data type"
    print >> sys.stderr, "  -i | --dataid: specify dataid"
    print >> sys.stderr
    os._exit(1)

def mq_push(mq_key, mq_type, dataid):
    import struct
    import sysv_ipc

    print 'sending dataid %s to MQ:' % (dataid, )
    mq = sysv_ipc.MessageQueue(mq_key, flags=sysv_ipc.IPC_CREAT, mode=0666)
    mq.send(struct.pack('<i', dataid), block=True, type=mq_type)
    
if __name__ == '__main__':
    mq_key = None
    mq_type = None
    data_id = None
    
    opts, args = getopt.getopt(sys.argv[1:], "i:k:h", ["dataid=", "key=", "help"])

    for k, v in opts:
        if k in ('-i', '--dataid'):
            data_id = int(v)
        elif k in ('-k', '--key'):
            mq_key = int(v)
        elif k in ('-t', '--type'):
            mq_type = int(v)
        
    print 'mq_key =', mq_key
    print 'dataid = ', data_id

    if data_id is None:
        usage()

    if mq_key is None:
        mq_key = 565
    if mq_type is None:
        mq_type = 15

    if data_id < -1:
        print >> sys.stderr, 'dataid must be > -1'
        os._exit(1)

    mq_push(mq_key, mq_type, data_id)



