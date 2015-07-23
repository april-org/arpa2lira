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

  BinarizeArpa::BinarizeArpa(VocabDictionary voc,
                             const char *inputFilename,
                             const char *begin_ccue,
                             const char *end_ccue) : 
    voc(voc),
    ngramOrder(0),
    num_states(2), // 0 and 1 are zerogram_st and final_st
    num_transitions(0),
    begin_ccue(voc(begin_ccue)),
    end_ccue(voc(end_ccue)) {

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
    //close(file_descriptor);
  }
  
  BinarizeArpa::~BinarizeArpa() {
  }

  void BinarizeArpa::processArpaHeader() {
    int level;
    constString cs,previous;
    do {
      cs = workingInput.extract_line();
    } while (cs != "\\data\\");
    previous = workingInput;
    cs = workingInput.extract_line();
    while (cs.skip("ngram")) { // example: ngram 1=103459
      assert(cs.extract_int(&level));
      assert(level == ++ngramOrder);
      cs.skip("=");
      assert(cs.extract_int(&counts[level-1]));
      previous = workingInput;
      cs = workingInput.extract_line();
    }
    workingInput = previous;
    if (ngramOrder > MAX_NGRAM_ORDER) {
      ERROR_EXIT(1, "Maximum ngram order overflow\n");
    }
  }
  
  void BinarizeArpa::create_mmapped_buffer(mmapped_file_data &filedata, size_t filesize) {
    filedata.file_size = filesize;
    if ((filedata.file_descriptor = 
         Config::openTemporaryFile(O_RDWR | O_CREAT | O_TRUNC, filedata.filename)) < 0) {
      ERROR_EXIT2(1, "Error creating file %s: %s\n",
                  filedata.filename.get(), strerror(errno));
    }
    
    // make file of desired size:
    
    // go to the last byte position
    if (lseek(filedata.file_descriptor,filedata.file_size-1, SEEK_SET) == -1) {
      ERROR_EXIT1(1, "lseek error, position %u was tried\n",
                  (unsigned int)(filedata.file_size-1));
    }
    // write dummy byte at the last location
    if (write(filedata.file_descriptor,"",1) != 1) {
      ERROR_EXIT(1, "write error\n");
    }
    // mmap the output file
    if ((filedata.file_mmapped = (char*)mmap(0, filedata.file_size,
                                          PROT_READ|PROT_WRITE, MAP_SHARED,
                                          filedata.file_descriptor, 0)) == (caddr_t)-1) {
      ERROR_EXIT(1, "mmap error\n");
    }
  }
  
  void BinarizeArpa::release_mmapped_buffer(mmapped_file_data &filedata) {
    if (munmap(filedata.file_mmapped, filedata.file_size) == -1) {
      ERROR_EXIT(1, "munmap error\n");
    }
  }

  void BinarizeArpa::create_output_vectors() {
    int max_num_states = 3;
    int max_num_transitions = 0;
    for (int level=0; level<ngramOrder-1; ++level) {
      max_num_states += counts[level];
    }
    max_num_transitions += max_num_states + counts[ngramOrder-1];

    create_mmapped_buffer(states_data, sizeof(StateData)*max_num_states);
    states = (StateData*) states_data.file_mmapped;
    create_mmapped_buffer(transitions_data, sizeof(TransitionData)*max_num_transitions);
    transitions = (TransitionData*) transitions_data.file_mmapped;
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
  
    int numNgrams = counts[level-1];

    for (int i=0; i<numNgrams; ++i) {
      if (i>0 && i%10000==0) {
        fprintf(stderr,"\r%6.2f%%",i*100.0f/numNgrams);
      }
      constString cs = workingInput.extract_line();
      float trans,bo;
      cs.extract_float(&trans);
      trans = arpa_prob(trans);
      cs.skip(1);
      for (int j=0; j<level; ++j) {
        constString word = cs.extract_token("\t ");
        cs.skip(1);
        int wordId = voc(word);
        ngramvec[j] = wordId;
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
      orig_state = get_state(ngramvec,level-1);
      if (ngramvec[level-1] == end_ccue)
        dest_state = final_st;
      else {
        dest_state = get_state(ngramvec+from,dest_size);
      }

      // look for backoff_dest_state
      backoff_dest_state = zerogram_st;
      int search_start = backoff_search_start;
      int search_size  = backoff_size;
      while (search_size>0 &&
             !exists_state(ngramvec+search_start,search_size,backoff_dest_state)) {
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
      transitions[num_transitions].word       = ngramvec[level-1];
      transitions[num_transitions].trans_prob = trans;
      num_transitions++;


    }
    fprintf(stderr, "\r100.00%%\n");
  }

  void BinarizeArpa::processArpa() {
    processArpaHeader();

    if (ngramOrder>1) {
      ngramvec[0] = begin_ccue;
      initial_st = get_state(ngramvec,1);
    } else {
      initial_st = zerogram_st;
    }


  }

} // namespace Arpa2Lira
