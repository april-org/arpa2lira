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
#ifndef NGRAM_TEMPLATE_H
#define NGRAM_TEMPLATE_H

#include "murmur_hash.h"
#include "hash_table.h"

namespace Arpa2Lira {

  struct NgramForHashing {
    uint64_t hashvalue;
    int *p; // pointer to ngram
    int sz; // ngram size
    NgramForHashing() {}
    NgramForHashing(const NgramForHashing &other) :
      hashvalue(other.hashvalue),
      p(other.p),
      sz(other.sz) {
    }

    NgramForHashing(int *p, int sz) :
      hashvalue(MurmurHash64((const void *)p, (std::size_t) sz)),
      p(p),
      sz(sz) {
    }
  };

  struct NFH_hash_fcn {
    uint64_t operator()(const NgramForHashing &x) const {
      return x.hashvalue;
    }
  };

  typedef AprilUtils::hash<NgramForHashing,
                           int,
                           NFH_hash_fcn,
                           default_equality_comparison_function> ngram_hash;

} // namespace Arpa2Lira

#endif // NGRAM_TEMPLATE_H
