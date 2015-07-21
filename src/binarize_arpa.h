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
#include <string> // use in the dictionary
#include <unordered_map>

#include "constString.h"
#include "error_print.h"
#include "smart_ptr.h"
#include "vector.h"

#define NGRAM_PROB_POS   0
#define BACKOFF_PROB_POS 1

namespace Arpa2Lira {

  class VocabDictionary {
    static const unsigned int MAX_WORD_SIZE = 10000u;
    unsigned int vocabSize;
    std::unordered_map<std::string,unsigned int> vocabDictionary;
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
    unsigned int operator()(const char *word) const {
      return vocabDictionary.find(std::string(word))->second;
    }
    unsigned int operator()(AprilUtils::constString cs) const {
      return vocabDictionary.find(std::string((const char *)cs, cs.len()))->second;
    }
  };

  template<unsigned int N, unsigned int M> // M may be 1 or 2
  struct Ngram {
    unsigned int word[N];
    float values[M]; // index 0 is transition log-probabilty, index 1 is backoff
  };

  class BinarizeArpa {

    /**
     * @brief unrolls a loop using templates and processes all ngram levels
     * calling to extractNgramLevel
     */
    template<unsigned int COUNT>
    void extractNgramLevelUnroller() {
      extractNgramLevelUnroller<COUNT-1>();
      if (COUNT <= ngramOrder) {
        if (COUNT == ngramOrder) {
          extractNgramLevel<COUNT,1u>();
        }
        else {
          extractNgramLevel<COUNT,2u>();
        }
      }
    }
    
    VocabDictionary dict;
    static const unsigned int MAX_NGRAM_ORDER=20;
    unsigned int counts[MAX_NGRAM_ORDER];
    AprilUtils::constString inputFile,workingInput;
    unsigned int ngramOrder;
    AprilUtils::UniquePtr<char> outputFilenames[MAX_NGRAM_ORDER];

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
  
    void processArpaHeader();
    template<unsigned int N, unsigned int M>
    void extractNgramLevel();
    
  public:
    BinarizeArpa(VocabDictionary dict,
                 const char *inputFilename);
    void processArpa();
  };

} // namespace Arpa2Lira

#endif // BINARIZE_ARPA_H
