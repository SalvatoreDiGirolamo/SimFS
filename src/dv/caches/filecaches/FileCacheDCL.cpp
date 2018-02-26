//
// Created by Pirmin Schmid on 18.04.17.
//

#include "FileCacheDCL.h"

namespace dv {

constexpr char FileCacheDCL::kCacheName[];

FileCacheDCL::FileCacheDCL(DV *dv_ptr, ID_type capacity, ID_type protected_mrus)
    : FileCacheBCL(dv_ptr, capacity, protected_mrus),
      etd_(capacity - 1), etd_capacity_(capacity - 1) {
    cache_name_ = kCacheName;
}

/**
 * basically identical to the BCL version, except cost is not
 * paid immediately but deferred to the ETD map
 */
void FileCacheDCL::actualPut(const std::string &key, std::unique_ptr<FileDescriptor> value) {
    if (cache_.size() < capacity_) {
        cache_.add(key, std::move(value));
    } else {
        id_cost_pair_type r = evict();
        if (r.first == kNone) {
            // no eviction could be made, error message was already printed
            // expand capacity to keep going
            cache_.add(key, std::move(value));
        } else {
            cache_.replace(r.first, key, std::move(value));

            if (0 < r.second) {
                // cost adjustment is deferred
                etdPut(cache_.get(r.first)->get()->getName(), 2 * r.second);
            } else if (r.second < 0) {
                // case kCostForNone -> no eviction was possible, error message was already printed -> reset Acost
                // case kCostForLRU  -> was LRU -> reset Acost
                resetAcost();
            }
        }
    }
}

/**
 * similar to LRU/BCL version, with adjustment to cost handling
 */
FileDescriptor *FileCacheDCL::actualGet(const std::string &key) {
    FileDescriptor *d = FileCacheBCL::actualGet(key);

    ID_type e = etd_.find(key);
    if (e != kNone) {
        depreciateAcost(*etd_.get(e));
        if (debug_messages_) {
            std::cout << "*** depreciating Acost due to lookup of a recently evicted non-LRU entry" << std::endl;
        }
    } else {
        if (key == lru_key_) {
            if (debug_messages_) {
                std::cout << "*** clearing of ETD due to earlier access of LRU than evicted "
                          << etd_.size() << " non-LRU entries" << std::endl;
            }
            etd_.clear();
        }
    }

    return d;
}

FileCacheDCL::id_cost_pair_type FileCacheDCL::findVictim() {
    return FileCacheBCL::findVictim();
}

bool FileCacheDCL::isCostAware() const {
    return kCostAware;
}

bool FileCacheDCL::isPartitionAware() const {
    return kPartitionAware;
}

void FileCacheDCL::etdPut(std::string evicted_key, dv::cost_type cost) {
    etd_ID_type id = etd_.find(evicted_key);
    if (id != kEtdNone) {
        etd_.replace(id, evicted_key, cost);
        return;
    }

    if (etd_.size() < etd_capacity_) {
        etd_.add(evicted_key, cost);
    } else {
        etd_.replace(etd_.getLruId(), evicted_key, cost);
    }
}

}
