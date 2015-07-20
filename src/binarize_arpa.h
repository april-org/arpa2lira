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

#include "constString.h"
#include "hash_table.h"
#include "vector.h"

namespace Arpa2Lira {

  class VocabDictionary {
    static const int MAX_WORD_SIZE = 10000;
    AprilUtils::vector<AprilUtils::string> vocab;
    AprilUtils::hash<const char *,unsigned int> vocabDictionary;
  public:
    VocabDictionary(const char *vocabFilename) {
      FILE *f = fopen(vocabFilename,"r");
      char word[MAX_WORD_SIZE];
      while (fscanf(f,"%s",word)==1) {
        vocab.push_back(AprilUtils::string(word));
        vocabDictionary[vocab.back().c_str()] = vocab.size();
      }
      fclose(f);
    }
    int operator()(const char *word) {
      return vocabDictionary[word];
    }
    int operator()(AprilUtils::constString cs) {
      return vocabDictionary[(const char *)cs];
    }
  };

  template<int N, int M> // M may be 1 or 2
  struct Ngram {
    int word[N];
    float values[M]; // index 0 is transition log-probabilty, index 1 is backoff
  };

  class BinarizeArpa {

    /**
     * @brief unrolls a loop using templates and processes all ngram levels
     * calling to extractNgramLevel
     */
    template<int COUNT>
    struct extractNgramLevelUnroller {
      extractNgramLevelUnroller(BinarizeArpa &binarizer,
                                const char *outputPrefixFilename) {
        extractNgramLevelUnroller<COUNT-1>(binarizer, outputPrefixFilename);
        if (COUNT <= binarizer.ngramOrder) {
          if (COUNT == binarizer.ngramOrder) {
            binarizer.extractNgramLevel<COUNT,1>(outputPrefixFilename);
          }
          else {
            binarizer.extractNgramLevel<COUNT,2>(outputPrefixFilename);
          }
        }
      }
    };
    
    VocabDictionary dict;
    static const int MAX_NGRAM_ORDER=30;
    int counts[MAX_NGRAM_ORDER];
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
  
    void processArpaHeader();
    template<int N, int M>
    void extractNgramLevel(const char *outputFilename);
    
  public:
    BinarizeArpa(VocabDictionary dict,
                 const char *inputFilename);
    void processArpa(const char *outputPrefixFilename);
  };

} // namespace Arpa2Lira

#endif // BINARIZE_ARPA_H
