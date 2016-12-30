#ifndef _ORS_OFFLINE_FTRL_UTIL_
#define _ORS_OFFLINE_FTRL_UTIL_
#include <vector>
#include <string>

//math
float sigmoid(float x);

float unsigmoid(float p);

float signum(float x);

//helper
void listFiles(std::string path, std::vector<std::string>& files);

#endif
