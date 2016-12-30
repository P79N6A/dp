#include "util.h"
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <algorithm>
#include <cmath>

//math
float sigmoid(float x) {
    if (x < -30) {
        return 0.0;
    } else if (x > 30) {
        return 1.0;
    }
    return 1.0 / (1.0 + exp(-x));
}

float unsigmoid(float p) {
    if (p < 0 || p > 1) {
        return 0.0;
    } else if (p == 0) {
        return -30.0;
    } else if (p == 1) {
        return 30.0;
    }
    return -log(1.0 / p - 1.0);
}

float signum(float x) {
    return (x > 0) - (x < 0);
}

//helper
void listFiles(std::string path, std::vector<std::string>& files) {
    using namespace std;
    struct dirent* ent = NULL;
    DIR *p_dir;
    p_dir = opendir(path.c_str());
    if (p_dir == NULL) {
        if (errno == ENOTDIR) {
            files.push_back(path);
        }
        return;
    }
    while (NULL != (ent = readdir(p_dir))) {
        string sub_name(ent->d_name);
        if (ent->d_type == 8) {  //file
            files.push_back(path + "/" + sub_name);
        } else {  //directory
            if (sub_name == "." || sub_name == "..") {
                continue;
            }
            listFiles(path + "/" + sub_name, files);
        }
    }
    closedir(p_dir);
    sort(files.begin(), files.end());
}
