/*-*- mode:c++ -*-
 *
 * address_chain.h - Data structures for implementating address chain.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef ADDRESS_CHAIN_H_
#define ADDRESS_CHAIN_H_

class AllocSite;
class Class;

/*
 * A Chain node is a pair: <Timestamp, Address>
 */
struct Chain {
  Chain(u32 addr, u32 stamp):addr_(addr), stamp_(stamp), next_(NULL) {}

  u32 addr_;
  u32 stamp_;
  Chain *next_;
};


/*
 * Each new object has a ACHead (Allocation Chain Head) in hashtable, which stores information
 * such as staleness, vtable address, whether is from NOS, etc. Also ACHead records an object's
 * move trace (a Chain list) during its life time.
 */
struct ACHead {
  ACHead(AllocSite *asp, Class *klass, Chain *cp, u32 size, u32 born_time, u32 mixword):
    asp_(asp),
    klass_(klass),
    obj_size_(size),
    born_time_(born_time),
    mixword_(mixword),
    cp_(cp),
    next_(NULL) {}

  AllocSite *asp_;              /* allocation site pointer */
  Class  *klass_;               /* type information */
  u32 obj_size_; /* object size, for array, this includes its elements size */
  u32 born_time_;

  /*
   * 1. bit 31 move bit
   * 2. bit 30 large object bit
   * 3. bits 29-15 staleness
   * 4. bits 14-0 last access time
   */
  u32 mixword_;

  Chain *cp_;                   /* address chain pointer */
  ACHead *next_;                /* for hash confilicts */
};


#endif  /* ADDRESS_CHAIN_H_ */