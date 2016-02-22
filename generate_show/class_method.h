/* -*- mode: c++ -*-
 * Copyright (C) 2015 Yu Hengyang.
 *
 * class_method.h - Type definitions for class and method.
 *
 * History:
 *
 * Created on 2015-11-20 by Hengyang Yu.
 */
#ifndef AC_CLASS_METHOD_H_
#define AC_CLASS_METHOD_H_

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "base.h"
#include "gen_html.h"

class Class final: public Base {
public:
  Class(uint32_t vtable, size_t instance_size, const std::string& name)
    :Base(vtable, name),
    instance_size_(instance_size) {
    }


  size_t GetInstanceSize() const { return instance_size_; }
  u32 GetClass() const { return GetKey(); }
  void AddAllocSite(AllocSite *asp) {
    if (asp)
      cross_ref_.insert(reinterpret_cast<Base*>(asp));
  }

  size_t instance_size_;
};


class Method final {
public:
  Method(uint32_t code_begin, uint32_t code_end, const std::string& name)
    :code_begin_(code_begin),
    code_end_(code_end),
    name_(name) {}

  uint32_t code_begin_;
  uint32_t code_end_;
  std::string name_;
};

typedef std::vector<Method*> MethodTable;
class ClassManager;

// return true on success, false on failure.
bool ReadClassAndMethod(const char *path,
                        MethodTable& method_table,
                        ClassManager& class_mgr,
			Gen* gen_html);

#endif  /* AC_CLASS_METHOD_H_ */

