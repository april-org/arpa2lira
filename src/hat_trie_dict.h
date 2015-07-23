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
#ifndef HAT_TRIE_DICT_H
#define HAT_TRIE_DICT_H

#include "hat_trie.h"

class HAT_TRIE_DICT {
  hattrie_t* hat;
public:
  HAT_TRIE_DICT() {
    hat = hattrie_create();
  }
  ~HAT_TRIE_DICT() {
    hattrie_free(hat);
  }
  bool get(const char* k, size_t sz, int &value) {
    int *ptr = (int*)hattrie_tryget(hat, k, sz);
    if (ptr) {
      value = *ptr;
      return true;
    }
    return false;
  }
  bool exists(const char* k, size_t sz) {
    return hattrie_tryget(hat, k, sz);
  }
  void set(const char* k, size_t sz, int value) {
    int* ptr = (int*)hattrie_get(hat, k, sz);
    ptr[0] = value;
  }
};

#endif // HAT_TRIE_DICT_H

