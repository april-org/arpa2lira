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
#ifndef BINARPA_2_LIRA
#define BINARPA_2_LIRA

#include "ngram_hash.h"

extern "C" {
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>           // mmap() is defined in this header
#include <unistd.h>
}

namespace Arpa2Lira {

  struct StateInfo {
    int fan_out;
    float best_prob;
    float backoff_weight;
    int backoff_st;
  };

  struct TransitionInfo {
    int   origin;
    int   dest;
    int   word;
    float prob;
  };

  class BinArpa2Lira {
    ngram_hash ngram2state;
    static const int maxNgramOrder = 20;
    static const int zero_gram = 0;
    static const int final_state = 1;
    int num_states;
    int ngramOrder;
    int begin_cc;
    int end_cc;
    int counts[maxNgramOrder];
    int mmaped_file_descriptor[maxNgramOrder];
    char *mmaped_file[maxNgramOrder];
    size_t mmaped_file_size[maxNgramOrder];

    StateInfo      *state_vector;
    TransitionInfo *transition_vector;

    void allocate_mmap_level(const char *mmapped_filename, int level);
    void release_mmap_level(int level);
    void process_mmaped_buffer(const char *buffer, int level);
    
  public:
    BinArpa2Lira(int ngramOrder, int ngramcounts[],
                 int begin_cc, int end_cc);
    ~BinArpa2Lira();
    void process_level(const char *mmapped_filename, int level);
    
  };

} // namespace Arpa2Lira

#endif // BINARPA_2_LIRA

