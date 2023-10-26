/*
 * index.h
 *
 *  Created on: Nov 18, 2019
 *      Author: teng
 */

#ifndef HISPEED_INDEX_H_
#define HISPEED_INDEX_H_
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
#include "../spatial/spatial.h"
#include "RTree.h"

#define FillFactor 0.9
#define IndexCapacity 10
#define LeafCapacity 50

using namespace std;
namespace hispeed {

/*
 * OCTree
 * */
class OctreeNode {
    weighted_aab node_voxel;

  public:
    long tile_size;
    int level;
    bool isLeaf;
    bool canBeSplit;
    OctreeNode* children[8];
    vector<weighted_aab*> objectList;
    bool isroot() {
        return level == 0;
    }
    OctreeNode(aab b, int level, long tsize);
    ~OctreeNode();
    bool addObject(weighted_aab* object);
    bool intersects(weighted_aab* object);
    void genTiles(vector<aab>& tiles);
    void query_nn(weighted_aab* box, vector<pair<int, range>>& results, float& min_farthest);
    void query_within(weighted_aab* box, vector<pair<int, range>>& results, const float min_farthest);
    void query_intersect(weighted_aab* box, vector<int>& results);
};
OctreeNode* build_octree(std::vector<weighted_aab*>& mbbs, int num_tiles);

// sorting tree
class SPNode {
    weighted_aab node_voxel;
    std::vector<SPNode*> children;
    SPNode* parent;

  public:
    SPNode() {
        parent = NULL;
    }
    ~SPNode() {
        for (SPNode* s : children) {
            delete s;
        }
        children.clear();
    }
    void add_child(SPNode* child) {
        node_voxel.box.update(child->node_voxel.box);
        node_voxel.size += child->node_voxel.size;
        children.push_back(child);
        child->parent = this;
    }
    void add_object(weighted_aab* obj) {
        node_voxel.size += obj->size;
        node_voxel.box.update(obj->box);
    }
    bool load(const char* path);
    bool persist(const char* path);
    void genTiles(vector<aab>& tiles);
};

SPNode* build_sort_partition(std::vector<weighted_aab*>& mbbs, int num_tiles);

}  // namespace hispeed
#endif /* HISPEED_INDEX_H_ */
