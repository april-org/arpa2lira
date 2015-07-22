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
  const int   BinarizeArpa::zerogram_st = 0;
  const int   BinarizeArpa::final_st = 1;

  BinarizeArpa::BinarizeArpa(VocabDictionary dict,
                             const char *inputFilename
                             int begin_ccue,
                             int end_ccue) : dict(dict),
                                           begin_ccue(begin_ccue),
                                           end_ccue(end_ccue) {
    ngramOrder = 0;
    num_states = 2; // 0 and 1 are zerogram_st and final_st
    num_transitions = 0;

    // open file
    int file_descriptor = -1;
    assert((file_descriptor = open(inputFilename, O_RDONLY)) >= 0);
    // find size of input file
    struct stat statbuf;
    assert(fstat(file_descriptor,&statbuf) >= 0);
    inputFilesize = statbuf.st_size;
    // mmap the input file
    assert((mmappedInput = (char*)mmap(0, inputFilesize,
                                       PROT_READ, MAP_SHARED,
                                       file_descriptor, 0))  !=(caddr_t)-1);
    workingInput = inputFile = constString(mmappedInput, inputFilesize);
    close(file_descriptor);
  }
  
  BinarizeArpa::~BinarizeArpa() {
  }

  void BinarizeArpa::processArpaHeader() {
    unsigned int level;
    constString cs,previous;
    do {
      cs = workingInput.extract_line();
    } while (cs != "\\data\\");
    previous = workingInput;
    cs = workingInput.extract_line();
    while (cs.skip("ngram")) { // example: ngram 1=103459
      assert(cs.extract_unsigned_int(&level));
      assert(level == ++ngramOrder);
      cs.skip("=");
      assert(cs.extract_unsigned_int(&counts[level-1]));
      previous = workingInput;
      cs = workingInput.extract_line();
    }
    workingInput = previous;
    if (ngramOrder > MAX_NGRAM_ORDER) {
      ERROR_EXIT(1, "Maximum ngram order overflow\n");
    }
  }
  
  template<unsigned int N, unsigned int M>
  bool BinarizeArpa::sortThreadCall(char *filemapped, size_t filesize,
                                    Ngram<N,M> *p, unsigned int numNgrams) {
    AprilUtils::Sort(p, numNgrams);
    // work done, free the resources ;)
    if (munmap(filemapped, filesize) == -1) {
      ERROR_EXIT(1, "munmap error\n");
    }
    return true;
  }

  const char* create_mmaped_file(const char *filename, size_t file_size) {

  }

  void BinarizeArpa::create_vectors() {
    int max_num_states = 3;
    int max_num_transitions = 0;
    for (int level=0; level<ngramOrder-1; ++level) {
      max_num_states += counts[level];
    }
    max_num_transitions += max_num_states + counts[ngramOrder-1];

    

    char  *filemapped;
    // trying to open file in write mode
    int f_descr;
    AprilUtils::UniquePtr<char []> outputFilename;
    if ((f_descr = Config::openTemporaryFile(O_RDWR | O_CREAT | O_TRUNC, outputFilename)) < 0) {
      ERROR_EXIT2(1, "Error creating file %s: %s\n",
                  outputFilename.get(), strerror(errno));
    }
    size_t filesize = sizeof(Ngram<N,M>) * numNgrams;

    //fprintf(stderr,"creating %s filename of size %d for level %d\n",
    //outputFilename.get(),(int)filesize,N);

    // keep the filename in a vector
    outputFilenames[N-1] = outputFilename;
    
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

    //fprintf(stderr,"created filename\n");

    // aqui crear un vector de talla 
    Ngram<N,M> *p = (Ngram<N,M>*)filemapped;


  }

  bool BinarizeArpa::exists_state(int *v, int n, int &st) {
    return ngram_dict.get((const char*)v,sizeof(int)*n,st);
  }

  int BinarizeArpa::get_state(int *v, int n) {
    int st;
    if (n<1)
      st = zerogram_st;
    else if (!ngram_dict.get((const char*)v,sizeof(int)*n,st)) {
      st = num_states++;
      states[st].fan_out = 0;
      states[st].backoff_dest = zerogram_st;
      states[st].best_prob = logZero;
      states[st].backoff_weight = logZero;
      ngram_dict.set((const char*)v,sizeof(int)*n,st);
    }
    return st;
  }
  
  void BinarizeArpa::extractNgramLevel(int level) {
    bool notLastLevel = level<ngramOrder;
    // this is true for the last ngram level:
    int from = 1;
    if (notLastLevel) {
      from = 0;
    }
    int dest_size = level-from;
    int backoff_search_start = from+1;
    int backoff_size = level-backoff_search_start;
    // skip header
    char header[20];
    sprintf(header,"\\%d-grams:",level);
    constString cs;
    do {
      cs = workingInput.extract_line();
      // fprintf(stderr,"reading %s\n",
      //         AprilUtils::UniquePtr<char []>(cs.newString()).get());
    } while (!cs.is_prefix(header));
  
    unsigned int numNgrams = counts[level-1];

    for (unsigned int i=0; i<numNgrams; ++i) {
      if (i>0 && i%10000==0) {
        fprintf(stderr,"\r%6.2f%%",i*100.0f/numNgrams);
      }
      constString cs = workingInput.extract_line();
      float trans,bo;
      cs.extract_float(&trans);
      trans = arpa_prob(trans);
      cs.skip(1);
      for (unsigned int j=0; j<N; ++j) {
        constString word = cs.extract_token("\r\t\n ");
        cs.skip(1);
        int wordId = dict(word);
        ngramv[j] = wordId;
      }
      if (notLastLevel) {
        if (cs.extract_float(&bo)) {
          bo = arpa_prob(bo);
        } else {
          bo = logOne;
        }
      }
      // process current ngram
      int orig_state,dest_state,backoff_dest_state;
      orig_state = get_state(ngramv,level-1);
      if (ngramv[N-1] == end_ccue)
        dest_state = final_st;
      else {
        dest_state = get_state(ngramv+from,dest_size);
      }

      // look for backoff_dest_state
      backoff_dest_state = zerogram_st;
      int search_start = backoff_search_start;
      int search_size  = backoff_size;
      while (search_size>0 && !exists_state(ngram+search_start,search_size,backoff_dest_state)) {
        search_start++;
        search_size--;
      }

      if (bo > logZero) {
        states[orig_state].backoff_dest = backoff_dest_state;
        states[orig_state].backoff_weight = bo; 
      }

      if (states[orig_state].best_prob < trans)
        states[orig_state].best_prob = trans;
      
      states[orig_state].fan_out++;
      transitions[num_transitions].origin     = orig_state;
      transitions[num_transitions].dest       = dest_state;
      transitions[num_transitions].word       = ngramv[level-1];
      transitions[num_transitions].trans_prob = trans;
      num_transitions++;


    }
    fprintf(stderr, "\r100.00%%\n");
    close(f_descr);

  }

  void BinarizeArpa::processArpa() {
    processArpaHeader();

    if (ngramOrder>1) {
      ngramvec[0] = begin_ccue;
      initial_st = get_state(ngramvec,1);
    } else {
      initial_st = zerogram_st;
    }

    // unrolls a loop using templates and processes all ngram levels calling to
    // extractNgramLevel
    extractNgramLevelUnroller<MAX_NGRAM_ORDER>();
    joinThreads();
    // work done, free the resources ;)
    if (munmap(mmappedInput, inputFilesize) == -1) {
      ERROR_EXIT(1, "munmap error\n");
    }    
  }

  void BinarizeArpa::joinThreads() {
    for (auto && result : sort_thread_results) {
      result.get();
    }
    sort_thread_results.clear();
  }
  
} // namespace Arpa2Lira
