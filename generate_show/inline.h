/* -*- mode:c++ -*-
 *
 * inline.h - Short inline functions.
 *
 * History:
 *
 * Created on 2015-11-21 by Hengyang Yu.
 */
#ifndef INLINE_H_
#define INLINE_H_

#include "ac_defs.h"

inline u32 make_mix_word(u32 move_bit, u32 large_bit, u32 staleness, u32 last_access_time) {
  return
    ((move_bit & kMoveBitMask) << kMoveBitShiftBits) |
    ((large_bit & kLargeBitMask) << kLargeBitShiftBits) |
    ((staleness & kStalenessMask) << kStalenessShiftBits) |
    ((last_access_time & kLastAccessTimeMask) << kLastAccessTimeShiftBits);
}

inline u32 move_bit(u32 mixword) {
  return (mixword >> kMoveBitShiftBits) & kMoveBitMask;
}

inline u32 large_bit(u32 mixword) {
  return (mixword >> kLargeBitShiftBits) & kLargeBitMask;
}

inline bool not_moved(u32 mixword) {
  return !move_bit(mixword);
}

inline bool not_large(u32 mixword) {
  return !large_bit(mixword);
}

inline u32 staleness(u32 mixword) {
  return (mixword >> kStalenessShiftBits) & kStalenessMask;
}

inline u32 lat(u32 mixword) {
  return (mixword >> kLastAccessTimeShiftBits) & kLastAccessTimeMask;
}


#endif  /* INLINE_H_ */