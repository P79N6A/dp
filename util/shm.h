/**
**/

#ifndef _UTIL_SHM_H_
#define _UTIL_SHM_H_
//include STD C/C++ head files
#include <stdint.h>
#include <stddef.h>

//include third_party_lib head files


namespace poseidon
{
namespace util
{
namespace shm
{

void* ShmCreate(uint32_t key, size_t size, int shmflag=0600);
void* ShmAttach(uint32_t key, size_t size, int shmflag=0600);
int ShmDetach(void* addr);
int ShmDelete(uint32_t key, size_t size);
bool ShmGetSize(uint32_t key, size_t *size);


} // namespace shm
} // namespace util
} // namespace poseidon

#endif // _UTIL_SHM_H_

