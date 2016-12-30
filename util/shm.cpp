/**
**/

//include STD C/C++ head files
#include <sys/shm.h>
#include <sys/errno.h>

//include third_party_lib head files
#include "util/shm.h"
#include "util/log.h"

namespace poseidon
{
namespace util 
{

namespace shm
{

void* ShmAttach(uint32_t key, size_t size, int shmflag)
{
    int shmid = shmget(key, size, shmflag);
    if (shmid == -1)
    {
        LOG_ERROR("shmget key=%d, size=%d failed!", key, size);
        return NULL;
    }

    void* addr = shmat(shmid, 0, 0);
    if (addr == (void*)-1)
    {
        LOG_ERROR("shmget key=%d, size=%d failed!", key, size);
        return NULL;
    }

    return addr;
}

void* ShmCreate(uint32_t key, size_t size, int shmflag)
{
    int shmid = shmget(key, size, shmflag|IPC_CREAT);
    if (shmid == -1)
    {
        LOG_ERROR("shmget key=%d, size=%d failed!", key, size);
        if (errno == EINVAL)
        {
            shmid = shmget(key, 0, SHM_R|SHM_W);
        }
        if (shmid == -1)
        {
            LOG_ERROR("shmget key=%d, size=%d failed!", key, 0);
            return NULL;
        }

        int ret = shmctl(shmid, IPC_RMID, NULL);
        if (ret == 0)
        {
            shmid = shmget(key, size, SHM_R|SHM_W|IPC_CREAT);
        }
        else 
        {
            LOG_ERROR("shmctl remove key=%d, size=%d failed!", key, size);
            return NULL;
        }
        if (shmid == -1)
        {
            LOG_ERROR("shmget key=%d, size=%d failed!", key, size);
            return NULL;
        }
    }

    void* addr = shmat(shmid, 0, 0);
    if (addr == (void*)-1)
    {
        LOG_ERROR("shmat key=%d, size=%d failed!", key, size);
        return NULL;
    }

    return addr;
}

int ShmDelete(uint32_t key, size_t size)
{
    int shmid = shmget(key, size, SHM_R|SHM_W);
    if (shmid == -1)
    {
        LOG_DEBUG("shmget key=%d, size=%d failed!", key, size);
        return -1;
    }

    int ret = shmctl(shmid, IPC_RMID, NULL);
    if (ret != 0)
    {
        LOG_DEBUG("shmctl remove key=%d, size=%d failed!", key, size);
    }
    return ret;
}

int ShmDetach(void* addr)
{
    int ret = shmdt(addr);
    if (ret != 0)
    {
        LOG_DEBUG("shmdt addr=%d failed!", addr);
    }
    return ret;
}

bool ShmGetSize(uint32_t key, size_t *size)
{
    int shmid = shmget(key, 0, SHM_R|SHM_W);
    if (shmid == -1)
    {
        LOG_ERROR("shmget key=%d, size=%d failed!", key, 0); 
        return false;
    }

    struct shmid_ds shm_stat;
    if (shmctl(shmid, IPC_STAT, &shm_stat) == 0)
    { 
        *size = shm_stat.shm_segsz;
        return true;
    }

    LOG_ERROR("shmctl IPC_STAT key=%d failed!", key);
    return false;
}



} // namespace shm
} // namespace ors
} // namespace poseidon

