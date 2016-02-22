/* -*- mode:c++ -*-
 *
 * base.h - Base class for allocation site and object type.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef BASE_H_
#define BASE_H_

#include <cmath>
#include <string>
#include "ac_defs.h"
#include "inline.h"
#include "address_chain.h"
#include <cassert>

/*
 * Basic information for objects classification.
 */
class Base {
public:
  Base(u32 key, const std::string& name):
    key_(key),
    nborn_(0),
    ndead_(0),
    nstale_(0),
    stale_mem_size_(0),
    sum_cur_normalized_staleness_(0.0f),
    sum_normalized_weighted_staleness_(0.0f),
    sum_prev_normalized_weighted_staleness_(0.0f),
    leak_confidence_(0.0f),
    name_(name) {
  }

  virtual ~Base() {}

  u32 key_;		// unique identifier of Base object, i.e, allocation site or vtable
  u32 nborn_;
  u32 ndead_;
  u32 nstale_;		// number of stale objects after gc
  u32 stale_mem_size_;
  double sum_cur_normalized_staleness_;
  //
  // Normalization formula: (Tgc - Tlast_access_time)/(Tgc - Tborn), which must < 1.0
  // Weighted by object's size
  //
  double sum_normalized_weighted_staleness_;
  double sum_prev_normalized_weighted_staleness_;
  double leak_confidence_;
  std::string name_;
  std::set<Base*> cross_ref_;   // for Class, Base is AllocSite; for AllocSite, Base is Class

  const std::string& GetName() const  { return name_;  }
  u32 GetKey() const { return key_; }
  u32 GetNumBorn() const { return nborn_; }
  u32 GetNumStale() const { return nstale_; }
  u32 GetStaleMemSize() const { return stale_mem_size_; }

  void AddStaleObj(const ACHead *ap)    {
    extern u32 g_ngc;
    u32 stal = staleness(ap->mixword_);

    if (stal > 1) {
      if (ap->obj_size_ < 4) {
        printf("obj size %uz\n", ap->obj_size_);
        return ;
      }
      assert(ap->obj_size_ >= 4);

      double normalized_staleness = (double)(stal - 1)/(g_ngc - ap->born_time_);
      double normalized_size = log2(ap->obj_size_)/32.0f;

      // printf("staleness: %u, g_ngc: %u, born time: %u\n", stal, g_ngc, ap->born_time_);
      assert(normalized_staleness < 1.0f);

      ++nstale_;
      stale_mem_size_ += ap->obj_size_;
      sum_cur_normalized_staleness_ += normalized_staleness;

      /*
       * Predictor 1 - Consider only staleness.
       */
      // _sum_normalized_weighted_staleness = _sum_cur_normalized_staleness;


      /*
       * Predictor 2 - Consider only size.
       */
      // _sum_normalized_weighted_staleness += normalized_size;


      /*
       * Predictor 3 - Consider size and staleness.
       */
      // _sum_normalized_weighted_staleness += normalized_size*normalized_staleness;


      /*
       * Predictor 4 - Consider size and staleness, plus adjustment of the exponent.
       */
      double coff = 1.0f;
      double e = std::min(2.0, 1.0/normalized_staleness);

      if (e < 2.0) {
        coff = pow(normalized_staleness, e);
        // coff = normalized_staleness; // <=> e = 1
      } else
        coff = normalized_staleness*normalized_staleness;

      sum_normalized_weighted_staleness_ += coff*normalized_size;
    }
  }

  void BeforeGc()    {
    nstale_ = 0;
    stale_mem_size_ = 0;
    sum_cur_normalized_staleness_ = 0.0f;
    sum_normalized_weighted_staleness_ = 0.0f;
  }

  void AfterGc()    {
    if (sum_normalized_weighted_staleness_ > 1.0f &&
        sum_prev_normalized_weighted_staleness_ < sum_normalized_weighted_staleness_) {

      sum_prev_normalized_weighted_staleness_ = sum_normalized_weighted_staleness_;
      leak_confidence_ = sum_normalized_weighted_staleness_;
    }
  }
};

#endif  /* BASE_H_ */
