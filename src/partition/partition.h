/*
 * partition.h
 *
 *  Created on: Nov 18, 2019
 *      Author: teng
 */

#ifndef PARTITION_H_
#define PARTITION_H_
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "../PPMC/mymesh.h"
#include "../geometry/aab.h"
#include "../index/index.h"
#include "../spatial/spatial.h"

using namespace std;
namespace hispeed {

// partition the space
void get_voxels(std::vector<std::string>& input_folders,
                std::vector<weighted_aab*>& voxels,
                const int num_threads,
                const int sample_rate);

void persist_tile(std::vector<aab>& tiles, const char* space_path);
void load_space(std::vector<aab>& tiles, const char* space_path);
void partition_data(std::vector<aab>& tiles, const char* output_folder);

}  // namespace hispeed
#endif /* PARTITION_H_ */
