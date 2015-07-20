/*
 * This file is part of Arpa2Lira for APRIL toolkit (A Pattern Recognizer In
 * Lua).
 *
 * Copyright 2015, Salvador España-Boquera, Francisco Zamora-Martinez
 *
 * Arpa2Lira is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef MURMUR_HASH_H
#define MURMUR_HASH_H
#include <cstddef>
#include <stdint.h>

#include "architecture.h"

namespace Arpa2Lira {
#ifdef ARCH64
  // 64-bit machine version
  uint64_t MurmurHash64(const void * key, std::size_t len, uint64_t seed = 0);
#else
  // 32-bit machine version (not the same function as above)
  uint64_t MurmurHash64(const void * key, std::size_t len, uint64_t seed = 0);
#endif
} // namespace Arpa2Lira

#endif // MURMUR_HASH_H
