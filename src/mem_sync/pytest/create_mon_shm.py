import sys
import os
import sysv_ipc

if __name__ == '__main__':
    shm = sysv_ipc.SharedMemory(
            0x1234,
            flags=sysv_ipc.IPC_CREAT,
            mode=0666,
            size=4800004)
