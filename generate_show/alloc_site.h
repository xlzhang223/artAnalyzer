/* -*- mode:c++ -*-
 *
 * alloc_site.h - Allocation site information collector.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */

#ifndef ALLOC_SITE_H_
#define ALLOC_SITE_H_

#include <set>
#include "base.h"

class Class;

class AllocSite final: public Base {
public:
  AllocSite(u32 site, const std::string& name):
    Base(site, name) {
  }
  ~AllocSite() {}

  void AddClass(Class *klass) {
    if (klass)
      cross_ref_.insert(reinterpret_cast<Base*>(klass));
  }
  u32 GetSite() const { return GetKey(); }
};


#endif  /* ALLOC_SITE_H_ */