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
extern "C" {
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>           // mmap() is defined in this header
#include <unistd.h>
}

#include <cerrno>
#include <cstring>

// from APRIL
#include "error_print.h"
#include "qsort.h"
#include "smart_ptr.h"
#include "unused_variable.h"

// from Arpa2Lira
#include "binarize_arpa.h"
#include "config.h"

using namespace AprilUtils;

namespace Arpa2Lira {
  
  const float BinarizeArpa::logZero = 1e-12f;
  const float BinarizeArpa::logOne  = 0.0f;
  const float BinarizeArpa::log10   = 2.302585092994046; // log(10.0);

  BinarizeArpa::BinarizeArpa(VocabDictionary dict,
                             const char *inputFilename) : dict(dict) {
    ngramOrder = 0;
    // open file
    int file_descriptor = -1;
    assert((file_descriptor = open(inputFilename, O_RDONLY)) >= 0);
    // find size of input file
    struct stat statbuf;
    assert(fstat(file_descriptor,&statbuf) >= 0);
    int filesize = statbuf.st_size;
    // mmap the input file
    char *filemapped;
    assert((filemapped = (char*)mmap(0, filesize,
                                     PROT_READ, MAP_SHARED,
                                     file_descriptor, 0))  !=(caddr_t)-1);
    workingInput = inputFile = constString(filemapped, filesize);
  }

  void BinarizeArpa::processArpaHeader() {
    unsigned int level;
    constString cs,previous;
    do {
      cs = workingInput.extract_line();
    } while (cs != "\\data\\");
    previous = workingInput;
    cs = workingInput.extract_line();
    while (cs.skip("ngram")) { //    ngram 1=103459
      assert(cs.extract_unsigned_int(&level));
      assert(level == ++ngramOrder);
      cs.skip("=");
      assert(cs.extract_unsigned_int(&counts[level-1]));
      // fprintf(stderr,"%d -> %d\n",level,counts[level-1]);
      previous = workingInput;
      cs = workingInput.extract_line();
      // fprintf(stderr,"Reading '%s'\n",cs.newString());
    }
    workingInput = previous;
    if (ngramOrder > MAX_NGRAM_ORDER) {
      ERROR_EXIT(1, "Maximum ngram order overflow\n");
    }
  }
  
  template<unsigned int N, unsigned int M>
  void BinarizeArpa::extractNgramLevel() {
    // fprintf(stderr,"extractNgramLevel N=%d M=%d\n",N,M);
    // skip header
    char header[20];
    sprintf(header,"\\%d-grams:",N);
    constString cs;
    do {
      cs = workingInput.extract_line();
      fprintf(stderr,"reading '%s'\n",
              AprilUtils::UniquePtr<char []>(cs.newString()).get());
    } while (!cs.is_prefix(header));
  
    unsigned int numNgrams = counts[N-1];
    char  *filemapped;

    // trying to open file in write mode
    int f_descr;
    AprilUtils::UniquePtr<char []> outputFilename;
    if ((f_descr = Config::openTemporaryFile(O_RDWR | O_CREAT | O_TRUNC, outputFilename)) < 0) {
      ERROR_EXIT2(1, "Error creating file %s: %s\n",
                  outputFilename.get(), strerror(errno));
    }
    size_t filesize = sizeof(Ngram<N,M>) * numNgrams;

    fprintf(stderr,"creating %s filename of size %d for level %d\n",
            outputFilename.get(),(int)filesize,N);

    // make file of desired size:
  
    // go to the last byte position
    if (lseek(f_descr,filesize-1, SEEK_SET) == -1) {
      ERROR_EXIT1(1, "lseek error, position %u was tried\n",
                  (unsigned int)(filesize-1));
    }
    // write dummy byte at the last location
    if (write(f_descr,"",1) != 1) {
      ERROR_EXIT(1, "write error\n");
    }
    // mmap the output file
    if ((filemapped = (char*)mmap(0, filesize,
                                  PROT_READ|PROT_WRITE, MAP_SHARED,
                                  f_descr, 0)) == (caddr_t)-1) {
      ERROR_EXIT(1, "mmap error\n");
    }

    fprintf(stderr,"created filename\n");

    // aqui crear un vector de talla 
    Ngram<N,M> *p = (Ngram<N,M>*)filemapped;

    for (unsigned int i=0; i<numNgrams; ++i) {
      if (i>0 && i%100==0) {
        fprintf(stderr,"\r%6.2f%%",i*100.0f/numNgrams);
      }
      constString cs = workingInput.extract_line();
      //fprintf(stderr,"%s\n",cs.newString());
      float trans,bo;
      cs.extract_float(&trans);
      p[i].values[NGRAM_PROB_POS] = arpa_prob(trans);
      cs.skip(1);
      for (unsigned int j=0; j<N; ++j) {
        constString word = cs.extract_token("\r\t\n ");
        cs.skip(1);
        int wordId = dict(word);
        //fprintf(stderr,"%d\n",wordId);
        p[i].word[j] = wordId;
      }
      if (M>1) {
        if (!cs.extract_float(&bo)) {
          bo = logOne;
        }
        p[i].values[BACKOFF_PROB_POS] = arpa_prob(bo);
      }
    }
    fprintf(stderr, "\r100.00%%\n");
    fprintf(stderr, "Sorting ngrams\n");
    AprilUtils::Sort(p, numNgrams);
    fprintf(stderr, "Ok\n");
    // work done, free the resources ;)
    if (munmap(filemapped, filesize) == -1) {
      ERROR_EXIT(1, "munmap error\n");
    }
    close(f_descr);
    // keep the filename in a vector
    outputFilenames[N-1] = outputFilename;
  }

  // base case for unroller template, does nothing
  template<> void BinarizeArpa::extractNgramLevelUnroller<0u>() {
  }
  
  void BinarizeArpa::processArpa() {
    processArpaHeader();
    // unrolls a loop using templates and processes all ngram levels calling to
    // extractNgramLevel
    extractNgramLevelUnroller<MAX_NGRAM_ORDER>();
  }
  
} // namespace Arpa2Lira
