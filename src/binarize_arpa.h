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
#ifndef BINARIZE_ARPA_H
#define BINARIZE_ARPA_H

#include <cmath> // log function
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <future>
#include <string> // use in the dictionary
#include <thread>
#include <unordered_map>
#include <map>
#include <vector>

#include "april-ann.h"

#include "hat_trie_dict.h"

namespace Arpa2Lira {

  class VocabDictionary {
    static const unsigned int MAX_WORD_SIZE = 10000u;
    unsigned int vocabSize;
    typedef std::unordered_map<std::string,unsigned int> dictType;
    dictType vocabDictionary;
  public:
    VocabDictionary(const char *vocabFilename) : vocabSize(0u) {
      FILE *f = fopen(vocabFilename,"r");
      char word[MAX_WORD_SIZE];
      while (fscanf(f,"%s",word)==1) {
        vocabDictionary.emplace(std::string(word), ++vocabSize);
      }
      if (vocabDictionary.size() != vocabSize) {
        ERROR_EXIT(1, "Improper dictionary, duplicated words found\n");
      }
      fclose(f);
    }
    unsigned int get_vocab_size() const {
      return vocabSize;
    }
    unsigned int operator()(const char *word) const {
      return vocabDictionary.find(std::string(word))->second;
    }
    unsigned int operator()(AprilUtils::constString cs) const {
      return vocabDictionary.find(std::string((const char *)cs, cs.len()))->second;
    }
    void writeDictionary(AprilIO::StreamInterface *f) const {
      std::vector<const char*> vec;
      vec.resize(vocabSize,"ERROR");
      for (dictType::const_iterator it = vocabDictionary.begin();
           it != vocabDictionary.end();
           ++it)
        vec[it->second-1] = it->first.c_str();
      for (unsigned int i=0; i<vocabSize; ++i)
        f->printf("%s\n",vec[i]);
    }
  };

  struct StateData {
    int fan_out;
    int backoff_dest;
    int cod; // the final code in lira format
    float best_prob;
    float backoff_weight;
  };

  struct TransitionData {
    int origin;
    int dest;
    int word;
    float trans_prob;
    
    // for sorting purposes
    bool operator<(const TransitionData &other) const {
      if (origin < other.origin) return true;
      if (origin > other.origin) return false;
      return word < other.word;
    }
  };

  struct mmapped_file_data {
    // NOT USED AprilUtils::UniquePtr<char []> filename;
    int file_descriptor;
    size_t file_size;
    char *file_mmapped;
  };

  class BinarizeArpa {

    VocabDictionary voc;
    static const int MAX_NGRAM_ORDER=20;
    int counts[MAX_NGRAM_ORDER];
    int ngramvec[MAX_NGRAM_ORDER];
    AprilUtils::constString inputFile,workingInput;
    int ngramOrder;

    static const float logZero;
    static const float logOne;
    static const float log10;

    float arpa_prob(float x) {
      if (x <= -99) {
        return logZero;
      }
      else {
        return x*log10;
      }
    }
  
    HAT_TRIE_DICT ngram_dict;

    static const int final_st;
    static const int zerogram_st;
    static const int no_backoff;
    int initial_st;
    int max_num_states;
    int num_states;
    int num_useful_states;
    int max_num_transitions;
    int num_transitions;
    int num_useful_transitions;
    float max_bound;

    int *cod2state; // vector of size num_useful_states

    mmapped_file_data input_arpa_file;
    mmapped_file_data states_data;
    mmapped_file_data transitions_data;
    
    StateData *states;
    TransitionData *transitions;

    int begin_ccue;
    int end_ccue;

    typedef std::map<int,int> int2int_dict_type;
    int2int_dict_type fan_out_dict; // ordered map for managing fan outs
    void add_fan_out(int f);

    bool exists_state(int *v, int n, int &st);
    void initialize_state(int st);
    int get_state(int *v, int sz);

    void read_mmapped_buffer(mmapped_file_data &filedata, const char *filename);
    void create_mmapped_buffer(mmapped_file_data &filedata, size_t filesize);
    void release_mmapped_buffer(mmapped_file_data &filedata);
    void create_output_vectors();

    void processArpaHeader();

    void skip_ngram_header(int level);
    void extractNgramLevel(int level);

    void compute_best_prob();
    
    bool is_useless_state(int st) { // inline
      return states[st].fan_out==0 && st!=final_st;
    }

    int renamed_state(int st) {
      return states[st].cod;
    }
    void bypass_backoff_useless_states_and_compute_fanout();
    void bypass_destination_useless_states();
    void rename_states();
    void rename_transitions();
    void sort_transitions();

    void write_lira_states(AprilIO::StreamInterface *f);
    void write_lira_transitions(AprilIO::StreamInterface *f);

  public:
    BinarizeArpa(const char *vocabFilename,
                 const char *inputFilename,
                 const char* begin_ccue,
                 const char* end_ccue);
    ~BinarizeArpa();
    void processArpa();
    void generate_lira(const char *liraFilename);
  };

} // namespace Arpa2Lira

#endif // BINARIZE_ARPA_H
