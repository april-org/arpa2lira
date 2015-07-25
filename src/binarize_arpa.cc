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
#include "april-ann.h"

// from Arpa2Lira
#include "binarize_arpa.h"
#include "config.h"

using namespace AprilUtils;

namespace Arpa2Lira {
  
  const float BinarizeArpa::logZero = -1e12f;
  const float BinarizeArpa::logOne  = 0.0f;
  const float BinarizeArpa::log10   = 2.302585092994046; // log(10.0);
  const int   BinarizeArpa::final_st = 0;
  const int   BinarizeArpa::zerogram_st = 1;
  const int   BinarizeArpa::no_backoff = -1;

  BinarizeArpa::BinarizeArpa(const char *vocabFilename,
                             const char *inputFilename,
                             const char *begin_ccue,
                             const char *end_ccue) : 
    voc(vocabFilename),
    ngramOrder(0),
    num_states(2), // 0 and 1 are zerogram_st and final_st
    num_transitions(0),
    begin_ccue(voc(begin_ccue)),
    end_ccue(voc(end_ccue)) {

    cod2state = 0;

    read_mmapped_buffer(input_arpa_file,inputFilename);
    workingInput = inputFile = constString(input_arpa_file.file_mmapped,
                                           input_arpa_file.file_size);
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

  void BinarizeArpa::read_mmapped_buffer(mmapped_file_data &filedata, const char *filename) {
    // open file
    filedata.file_descriptor = -1;
    assert((filedata.file_descriptor = open(filename, O_RDONLY)) >= 0);
    // find size of input file
    struct stat statbuf;
    assert(fstat(filedata.file_descriptor,&statbuf) >= 0);
    filedata.file_size = statbuf.st_size;
    // mmap the input file
    assert((filedata.file_mmapped = (char*)mmap(0, filedata.file_size,
                                                PROT_READ, MAP_SHARED,
                                                filedata.file_descriptor, 0))  !=(caddr_t)-1);
  }
  
  void BinarizeArpa::release_mmapped_buffer(mmapped_file_data &filedata) {
    if (munmap(filedata.file_mmapped, filedata.file_size) == -1) {
      ERROR_EXIT(1, "munmap error\n");
    }
    // da error close(filedata.file_descriptor);
  }

  void BinarizeArpa::create_output_vectors() {
    // determine an upper bound on the number of states and transitions
    max_num_states = 2;
    max_num_transitions = 0;
    for (int level=0; level<ngramOrder; ++level) {
      max_num_states += counts[level];
    }
    max_num_transitions += max_num_states;
    
    // actually create two vectors in a mmapped buffer
    create_mmapped_buffer(states_data, sizeof(StateData)*max_num_states);
    states = (StateData*) states_data.file_mmapped;
    create_mmapped_buffer(transitions_data, sizeof(TransitionData)*max_num_transitions);
    transitions = (TransitionData*) transitions_data.file_mmapped;

    // initialize zerogram_st and final_st
    initialize_state(final_st);
    initialize_state(zerogram_st);
  }

  bool BinarizeArpa::exists_state(int *v, int n, int &st) {
    if (n<1) {
      st = zerogram_st;
      return true;
    } else if (v[n-1] == end_ccue) {
      st = final_st;
      return true;
    }
    return ngram_dict.get((const char*)v,sizeof(int)*n,st);
  }

  void BinarizeArpa::initialize_state(int st) {
    states[st].fan_out = 0;
    states[st].backoff_dest = no_backoff;
    states[st].best_prob = logZero;
    states[st].backoff_weight = logZero;
  }

  int BinarizeArpa::get_state(int *v, int n) {
    int st =0;
    if (n<1)
      st = zerogram_st;
    else if (v[n-1] == end_ccue)
      st = final_st;
    else if (!ngram_dict.get((const char*)v,sizeof(int)*n,st)) {
      st = num_states++;
      assert(num_states <= max_num_states && " max num states exceeded\n");
      initialize_state(st);
      ngram_dict.set((const char*)v,sizeof(int)*n,st);
    }
    return st;
  }

  void BinarizeArpa::skip_ngram_header(int level) {
    char header[20];
    sprintf(header,"\\%d-grams:",level);
    constString cs;
    do {
      cs = workingInput.extract_line();
      fprintf(stderr,"%s reading %s\n", header,
              AprilUtils::UniquePtr<char []>(cs.newString()).get());
    } while (!cs.is_prefix(header));
  }

  void BinarizeArpa::extractNgramLevel(int level) {
    bool notLastLevel = level<ngramOrder;
    int from = 1; // this is true for the last ngram level:
    if (notLastLevel) {
      from = 0;
    }
    int dest_size = level-from;
    int backoff_search_start = from+1;
    int backoff_size = level-backoff_search_start;

    skip_ngram_header(level);
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
      assert(orig_state != final_st);
      dest_state = get_state(ngramvec+from,dest_size);

      if (dest_state != final_st &&
          states[dest_state].backoff_dest == no_backoff &&
          bo > logZero) {
        // look for backoff_dest_state
        backoff_dest_state = zerogram_st;
        int search_start = backoff_search_start;
        int search_size  = backoff_size;
        while (search_size>0 &&
               !exists_state(ngramvec+search_start,search_size,backoff_dest_state)) {
          search_start++;
          search_size--;
        }
        states[dest_state].backoff_dest = backoff_dest_state;
        states[dest_state].backoff_weight = bo;
      }

      if (states[orig_state].best_prob < trans)
        states[orig_state].best_prob = trans;
      
      assert(num_transitions < max_num_transitions && " max num transitions exceeded\n");
      states[orig_state].fan_out++;
      transitions[num_transitions].origin     = orig_state;
      transitions[num_transitions].dest       = dest_state;
      transitions[num_transitions].word       = ngramvec[level-1];
      transitions[num_transitions].trans_prob = trans;
      num_transitions++;
    }
    fprintf(stderr, "\r100.00%%\n");
  }

  void BinarizeArpa::compute_best_prob() {
    max_bound = logZero; // global
    for (int st=num_states-1; st>final_st; --st) {
      if (!is_useless_state(st)) {
        int   downst = st;
        float bound  = states[downst].best_prob;
        float backoffsum = 0;
        while (downst != zerogram_st &&
               states[downst].backoff_dest != no_backoff) {
          backoffsum += states[downst].backoff_weight;
          downst = states[downst].backoff_dest;
          float aux = backoffsum + states[downst].best_prob;
          if (aux > bound)
            bound = aux;
        }
        if (states[st].best_prob < bound)
          states[st].best_prob = bound;
        if (bound > max_bound)
          max_bound = bound;
      }
    }
  }

  void BinarizeArpa::add_fan_out(int f) {
    int2int_dict_type::iterator it = fan_out_dict.find(f);
    if (it == fan_out_dict.end())
      fan_out_dict[f]=1;
    else
      it->second++;
  }
  
  void BinarizeArpa::bypass_backoff_useless_states_and_compute_fanout() {
    add_fan_out(states[final_st].fan_out);
    num_useful_states = num_states;
    for (int st=final_st+1; st<num_states; ++st) {
      if (is_useless_state(st)) {
        num_useful_states--;
      } else {
        add_fan_out(states[st].fan_out); // put here to avoid a novel traversal :S
        while (states[st].backoff_dest != no_backoff &&
               is_useless_state(states[st].backoff_dest)) {
          int back = states[st].backoff_dest;
          states[st].backoff_weight += states[back].backoff_weight;
          states[st].backoff_dest = states[back].backoff_dest;
        }
      }
    }
  }
  
  void BinarizeArpa::bypass_destination_useless_states() {
    num_useful_transitions = 0;
    for (int trans=0; trans<num_transitions; ++trans) {
      if (!is_useless_state(transitions[trans].origin)) {
        num_useful_transitions++;
        while (is_useless_state(transitions[trans].dest)) {
          transitions[trans].trans_prob += states[transitions[trans].dest].backoff_weight;
          transitions[trans].dest = states[transitions[trans].dest].backoff_dest;
        }
      }
    }
  }
  
  void BinarizeArpa::rename_states() {
    cod2state = new int[num_useful_states];
    int aux_cont_states = 0;

    int2int_dict_type first_state_dict; 
    // traverse fanouts in a sorted way (ordered map)
    for (int2int_dict_type::iterator it = fan_out_dict.begin();
         it != fan_out_dict.end();
         ++it) {
      first_state_dict[it->first] = aux_cont_states;
      aux_cont_states += it->second;
    }
    assert(aux_cont_states == num_useful_states);

    // unloop final_st as special case
    int fanout = states[final_st].fan_out;
    int cod = first_state_dict[fanout]++;
    states[final_st].cod = cod;
    cod2state[cod] = final_st;
    // rest of states
    for (int st=final_st+1; st<num_states; ++st) {
      fanout = states[st].fan_out;
      if (fanout>0) { // useful state
        cod = first_state_dict[fanout]++;
        states[st].cod = cod;
        cod2state[cod] = st;
      } else { // useless state
        states[st].cod = num_states+1;
      }
    }
    for (int st=0; st<num_states; ++st) {
      if (!is_useless_state(st) &&
          states[st].backoff_dest != no_backoff)
        states[st].backoff_dest = renamed_state(states[st].backoff_dest);
    }
  }

  void BinarizeArpa::rename_transitions() {
    for (int trans=0; trans<num_transitions; ++trans) {
      transitions[trans].origin = renamed_state(transitions[trans].origin);
      transitions[trans].dest   = renamed_state(transitions[trans].dest);
    }
  }
    
  void BinarizeArpa::sort_transitions() {
    AprilUtils::Sort(transitions, num_transitions);
  }

  void BinarizeArpa::write_lira_states(FILE *f) {
    fprintf(f,"# initial state, final state and lowest state\n%d %d %d\n",
            states[initial_st].cod,
            states[final_st].cod,
            states[zerogram_st].cod);
    fprintf(f,"# state backoff_st 'weight(state->backoff_st)' [max_transition_prob]\n"
            "# backoff_st == -1 means there is no backoff\n");

    for (int cod=0; cod<num_useful_states; ++cod) {
      int st = cod2state[cod];
      if (st < num_states)
        fprintf(f,"%d %d %g %g\n",
                cod,
                states[st].backoff_dest,
                states[st].backoff_weight,
                states[st].best_prob);
    }
  }

  void BinarizeArpa::write_lira_transitions(FILE *f) {
    fprintf(f,"# transitions\n# orig dest word prob\n");
    for (int trans=0; trans<num_useful_transitions; ++trans) {
      fprintf(f,"%d %d %d %g\n",
              transitions[trans].origin,
              transitions[trans].dest,
              transitions[trans].word,
              transitions[trans].trans_prob);
    }
  }

  void BinarizeArpa::generate_lira(const char *liraFilename) {
    // compute getBestProb
    fprintf(stderr,"computing best prob\n");
    compute_best_prob();

    // detect states with fanout zero which are not final, they are to
    // be removed
    fprintf(stderr,"bypassing states to be removed for backoff purposes\n");
    bypass_backoff_useless_states_and_compute_fanout();

    fprintf(stderr,"bypassing useless destination states\n");
    bypass_destination_useless_states();
    
    fprintf(stderr,"renaming states\n");
    rename_states();

    fprintf(stderr,"renaming transitions\n");
    rename_transitions();

    // sort the vector of transitions first by renamed origin state
    // and second by word
    fprintf(stderr,"sorting transitions\n");
    sort_transitions();

    fprintf(stderr,"opening file \"%s\"\n",liraFilename);
    FILE *f = fopen(liraFilename,"w");

    fprintf(f,"# number of words and words\n%d\n",voc.get_vocab_size());
    voc.writeDictionary(f);
    fprintf(f,"# max order of n-gram\n%d\n",ngramOrder);
    fprintf(f,"# number of states\n%d\n",num_useful_states);
    fprintf(f,"# number of transitions\n%d\n",num_useful_transitions);
    fprintf(f,"# bound max trans prob\n%f\n",max_bound);
    
    int different_fan_outs = fan_out_dict.size();
    fprintf(f,"# how many different number of transitions\n%d\n"
            "# \"x y\" means x states have y transitions\n",
            different_fan_outs);
    for (int2int_dict_type::iterator it = fan_out_dict.begin();
         it != fan_out_dict.end();
         ++it) {
      fprintf(f,"%d %d\n",it->second,it->first);
    }

    write_lira_states(f);
    write_lira_transitions(f);

    fprintf(stderr,"closing file \"%s\"\n",liraFilename);
    fclose(f);
  }    

  void BinarizeArpa::processArpa() {
    processArpaHeader();
    fprintf(stderr,"arpa header processed\n");

    fprintf(stderr,"creating output vectors\n");
    create_output_vectors();
    fprintf(stderr,"output vectors created\n");

    if (ngramOrder>1) {
      ngramvec[0] = begin_ccue;
      initial_st = get_state(ngramvec,1);
    } else {
      initial_st = zerogram_st;
    }
    for (int level=1; level<=ngramOrder; ++level) {
      fprintf(stderr,"extractNgramLevel(%d)\n",level);
      extractNgramLevel(level);
    }
    fprintf(stderr,"%d states, %d transitions\n",num_states,num_transitions);

    fprintf(stderr,"releasing arpa file\n");
    release_mmapped_buffer(input_arpa_file);
    fprintf(stderr,"released\n");
  }

} // namespace Arpa2Lira
