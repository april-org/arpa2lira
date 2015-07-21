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

#define NGRAM_PROB_POS   0
#define BACKOFF_PROB_POS 1

namespace Arpa2Lira {

  template<unsigned int N, unsigned int M> // M may be 1 or 2
  struct Ngram {
    unsigned int word[N];
    float values[M]; // index 0 is transition log-probabilty, index 1 is backoff
    // for sorting purposes
    bool operator<(const Ngram<N,M> &other) {
      for (unsigned int i=0; i<N; ++i) {
        if (word[i] < other.word[i]) return true;
        else if (word[i] > other.word[i]) return false;
      }
      return false;
    }
  };

} // namespace Arpa2Lira

#endif // NGRAM_TEMPLATE_H
