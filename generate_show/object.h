/* -*- mode:c++ -*-
 *
 * object.h - Oject meta information.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef OBJECT_H_
#define OBJECT_H_

#include "ac_defs.h"

struct Object {
  u32 addr;
  u32 klass;
  u32 alloc_site;
  u32 size;

  inline ObjectKind Kind() const { return static_cast<ObjectKind>(TAG(klass)); }
  inline bool IsLarge() const { return Kind() == kLargeObject; }
  inline bool IsArray() const { return Kind() == kArrayObject; }
  inline u32 GetClass() const { return klass & kObjAddrMask; }
};

#endif  /* OBJECT_H_ */

