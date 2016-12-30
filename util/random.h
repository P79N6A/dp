/**
**/

#ifndef _UTIL_RAMDOM_H_
#define _UTIL_RAMDOM_H_
//include STD C/C++ head files
#include <ext/algorithm>

//include third_party_lib head files


namespace poseidon
{
namespace util
{
// http://en.wikipedia.org/wiki/Fisher-Yates_shuffle
template <class RandomAccessIterator>
void RandomShuffle(RandomAccessIterator first, RandomAccessIterator last,
                   RandomAccessIterator ofirst, RandomAccessIterator olast)
{
    (void)last;

    int n = olast - ofirst;
    ofirst[0] = first[0];
    for (int i = 1; i < n; ++i)
    {
        int j = rand() % (i + 1);
        ofirst[i] = ofirst[j];
        ofirst[j] = first[i];
    }
}

// http://en.wikipedia.org/wiki/Reservoir_sampling
template <class RandomAccessIterator>
void RandomSample(RandomAccessIterator first, RandomAccessIterator last,
                  RandomAccessIterator ofirst, RandomAccessIterator olast)
{
    int k = olast - ofirst;
    int n = last - first;

    if (k == n)
    {
        RandomShuffle(first, last, ofirst, olast);
    }
    else
    {
        __gnu_cxx::random_sample(first, last, ofirst, olast);
    }
}

} // namespace util
} // namespace poseidon

#endif // _UTIL_RAMDOM_H_

