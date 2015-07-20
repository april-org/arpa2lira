#include <cmath> // log function
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <map> // std dictionary
#include <string> // use in the dictionary
#include "constString.h"

#define _unused(x) ((void)x)

class VocabDictionary {
  static const int bufferSize = 10000;
  int vocabSize;
  std::map <std::string,int> vocabDictionary;
public:
  VocabDictionary(const char *vocabFilename) {
    vocabSize = 0;
    FILE *f = fopen(vocabFilename,"r");
    char word[bufferSize];
    while (fscanf(f,"%s",word)==1)
      vocabDictionary[std::string(word)] = ++vocabSize;
    fclose(f);
  }
  int operator()(const char *word) {
    return vocabDictionary[std::string(word)];
  }
  int operator()(constString cs) {
    return vocabDictionary[std::string((const char *)cs,0,cs.len())];
  }
};

template<int N, int M> // M may be 1 or 2
struct Ngram {
  int word[N];
  float values[M]; // index 0 is transition log-probabilty, index 1 is backoff
};

class BinarizeArpa {
  VocabDictionary dict;
  static const int maxNgramOrder=10;
  int counts[maxNgramOrder];
  constString inputFile,workingInput;
  int ngramOrder;

  static const float logZero = 1e-12;
  static const float logOne  = 0;
  static const float log10   = 2.302585092994046; // log(10.0);

  float arpa_prob(float x) {
    if (x <= -99) 
      return logZero;
    else
      return x*log10;
  }
  
  void processArpaHeader();
  template<int N, int M>
  void extractNgramLevel(const char *outputFilename);

public:
  BinarizeArpa(VocabDictionary dict,
	       const char *inputFilename);
  void processArpa(const char *outputPrefixFilename);
};

