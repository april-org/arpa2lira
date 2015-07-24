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
    max_num_states = 3;
    max_num_transitions = 0;
    for (int level=0; level<ngramOrder; ++level) {
      max_num_states += counts[level];
    }
    max_num_transitions += max_num_states + counts[ngramOrder-1];

    create_mmapped_buffer(states_data, sizeof(StateData)*max_num_states);
    states = (StateData*) states_data.file_mmapped;
    create_mmapped_buffer(transitions_data, sizeof(TransitionData)*max_num_transitions);
    transitions = (TransitionData*) transitions_data.file_mmapped;
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

  int BinarizeArpa::get_state(int *v, int n) {
    int st =0;
    if (n<1)
      st = zerogram_st;
    else if (v[n-1] == end_ccue)
      st = final_st;
    else if (!ngram_dict.get((const char*)v,sizeof(int)*n,st)) {
      st = num_states++;
      assert(num_states < max_num_states && " max num states exceeded\n");
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
      fprintf(stderr,"%s reading %s\n", header,
              AprilUtils::UniquePtr<char []>(cs.newString()).get());
    } while (!cs.is_prefix(header));
  
    int numNgrams = counts[level-1];

    for (int i=0; i<numNgrams; ++i) {
      if (i>0 && i%1000==0) {
        fprintf(stderr,"\r%6.2f%%",i*100.0f/numNgrams);
      }
      constString cs = workingInput.extract_line();
      //constString cscopy = cs;
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

      // if (!notLastLevel) {
      //   fprintf(stderr,"%d states, line \"%s\"\n",num_states,
      //           AprilUtils::UniquePtr<char []>(cscopy.newString()).get());
      //   for (int x=0; x<level; ++x)
      //     fprintf(stderr,"%d ",ngramvec[x]);
      //   fprintf(stderr,"\ndest: ");
      //   for (int x=0; x<dest_size; ++x)
      //     fprintf(stderr,"%d ",ngramvec[from+x]);
      //   fprintf(stderr,"\n");
      //   fprintf(stderr,"  exist origin %d exist dest %d\n",
      //           (int)exists_state(ngramvec,level-1,orig_state),
      //           (int)exists_state(ngramvec+from,dest_size,dest_state));
      // }

      orig_state = get_state(ngramvec,level-1);
      dest_state = get_state(ngramvec+from,dest_size);

      if (dest_state == final_st) {
        bo = logZero;
      } else {
        // look for backoff_dest_state
        backoff_dest_state = zerogram_st;
        int search_start = backoff_search_start;
        int search_size  = backoff_size;
        while (search_size>0 &&
               !exists_state(ngramvec+search_start,search_size,backoff_dest_state)) {
          search_start++;
          search_size--;
        }
      }

      if (bo > logZero) {
        states[orig_state].backoff_dest = backoff_dest_state;
        states[orig_state].backoff_weight = bo; 
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

      // if (!notLastLevel) {
      //   fprintf(stderr,"%d %d %d %f\n",orig_state,dest_state,ngramvec[level-1],trans);
      // }

    }
    fprintf(stderr, "\r100.00%%\n");
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

  }

  void BinarizeArpa::sort_transitions() {
    AprilUtils::Sort(transitions, num_transitions);
  }

  void BinarizeArpa::compute_best_prob() {
    for (int st=num_states-1; st>final_st; --st) {
      int   downst = st;
      float bound  = states[downst].best_prob;
      float backoffsum = 0;
      while (st != zerogram_st &&
             states[downst].backoff_weight > logZero) {
        backoffsum += states[downst].backoff_weight;
        downst = states[downst].backoff_dest;
        float aux = backoffsum + states[downst].best_prob;
        if (aux > bound)
          bound = aux;
      }
      if (states[st].best_prob < bound)
        states[st].best_prob = bound;
    }
  }

  void BinarizeArpa::detect_useless_states() {
    int count=0;
    for (int st=final_st+1; st<num_states; ++st) {
      if (states[st].fan_out == 0)
        count++;
    }
    printf("there are %d useless states\n",count);
  }

  void BinarizeArpa::generate_lira() {

    // sorting transitions
    fprintf(stderr,"sorting transitions\n");
    sort_transitions();
    fprintf(stderr,"sorted!\n");

    
    // compute getBestProb
    fprintf(stderr,"computing best prob\n");
    compute_best_prob();
    fprintf(stderr,"computed!\n");


    // detect states with fanout zero which are not final, they are to
    // be removed
    fprintf(stderr,"detecting states to be removed\n");
    detect_useless_states();
    fprintf(stderr,"detected!\n");


    // esto en el arpa2lira lo que se hace es       a_eliminar[st] = true

    // check if an state descend (by means of backoff) to a state to be removed
    // local it = state2transitions:beginIt()
    // local end_it = state2transitions:endIt()
    // while it:notEqual(end_it) do
    //   local st = it:getState()
    //   while backoffs[st] and a_eliminar[backoffs[st][1]] do
    //     backoffs[st][2] = backoffs[st][2] + backoffs[backoffs[st][1]][2]
    //     backoffs[st][1] = backoffs[backoffs[st][1]][1]
    //   end
    //   it:next()
    // end
    
    // sort the vector of transitions by the word

    // -- eliminar estados cambiando las transiciones que llegan a ellos
    // --for st,trans in pairs(state2transitions) do
    // local it = state2transitions:beginIt()
    // local end_it = state2transitions:endIt()
    // while it:notEqual(end_it) do
    //   local st = it:getState()
    //   local trans = it:getTransitions()
    //   local maxprob
    //   --for i = 1,#trans,3 do
    //   for i=1,trans:size() do
    //     --local dest,prob = trans[i],trans[i+2]
    //     local dest,word,prob = trans:get(i)
    //     while a_eliminar[dest] do
    //       local info = backoffs[dest] -- info es destino,bow
    //       dest = info[1]
    //       prob = prob+info[2]
    //     end
    //     --trans[i],trans[i+2] = dest,prob
    //     trans:set(i,dest,word,prob)
    //   end
    //   -- ordenamos las transiciones por el id de la palabra
    //   --shellsort(trans)
    //   trans:sortByWordId()
    //   it:next()
    // end

    // -- eliminar efectivamente los "estados a eliminar"
    // for st,trans in pairs(a_eliminar) do
    //   backoffs[st]          = nil
    //   --state2transitions[st] = nil
    //   state2transitions:erase(st)
    // end

    // collectgarbage("collect")
    
    // -- ordenar los estados por numero de transiciones que salen de ellos
    // local fan_out_list    = {} -- numero de transiciones que aparecen
    // local num_transitions = 0
    // local num_states      = 0
    // --for st,trans in pairs(state2transitions) do
    // local it = state2transitions:beginIt()
    // local end_it = state2transitions:endIt()
    // while it:notEqual(end_it) do
    //   local st = it:getState()
    //   local trans = it:getTransitions()
    //   --local fanout = #trans/3 -- fan out del estado st
    //   local fanout = trans:size()
    //   num_states = num_states + 1 -- contamos estados
    //   num_transitions = num_transitions + fanout -- contamos todas las transiciones
    //   if fanout2states[fanout] == nil then -- lista de estados con un fan out dado
    //     fanout2states[fanout] = {}
    //     table.insert(fan_out_list,fanout) -- lista de fan outs aparecidos
    //   end
    //   table.insert(fanout2states[fanout],st)
    //   it:next()
    // end
    
    // ----------------------------------------------------------------------
    // -- empezamos a escribir el formato lira
    // ----------------------------------------------------------------------
    
    // local tabla_vocabulario = vocabulary:getWordVocabulary()
    // fprintf(output_file,"# number of words and words\n%d\n%s\n",#tabla_vocabulario,
    //         table.concat(tabla_vocabulario,"\n"))
    // fprintf(output_file,
    //         "# max order of n-gram\n%d\n# number of states\n%d\n# number of transitions\n%d\n"..
    //           "# bound max trans prob\n%f\n",
    //         n,num_states,num_transitions,bestProb)
    
    // ----------------------------------------------------------------------
    // -- darle un numero a cada estado
    // ----------------------------------------------------------------------
    // local state2cod = {} -- state_name   -> state_number
    // local cod2state = {} -- state_number -> state_name
    // local num_state = -1
    // fprintf(output_file,"# how many different number of transitions\n%d\n" ..
    //         "# \"x y\" means x states have y transitions\n",
    //       #fan_out_list)
    // -- recorremos los estados ORDENADOS por numero de transiciones de
    // -- salida
    // table.sort(fan_out_list)
    // for i,fan_out in ipairs(fan_out_list) do
    //   -- codificamos todos los estados con fan_out transiciones
    //   fprintf(output_file,"%d %d\n",
    //           #fanout2states[fan_out],fan_out)
    //   for k,st in ipairs(fanout2states[fan_out]) do
    //     num_state            = num_state+1
    //     state2cod[st]        = num_state
    //     cod2state[num_state] = st
    //   end
    // end
    
    // fprintf(output_file,"# initial state, final state and lowest state\n%d %d %d\n",
    //         state2cod[initial_state],state2cod[final_state],state2cod[lowest_state])
    
    // -- para cada estado imprimimos su estado backoff destino, la
    // -- probabilidad de bajar y su cota superior de transitar
    // fprintf(output_file,"# state backoff_st 'weight(state->backoff_st)' [max_transition_prob]\n")
    // fprintf(output_file,"# backoff_st == -1 means there is no backoff\n")
    // -- stcod es el codigo definitivo que sale en lira, statename es el
    // -- codigo que le asigno el trie
    // for stcod=0,num_state do
    //   local statename = cod2state[stcod]
    //   local s         = string.format("%d",stcod)
    //   local info      = backoffs[statename]
    //   if info then
    //     if state2cod[info[1]] == nil then
    //       error(string.format("se intenta bajar por backoff al estado %s que no existe",
    //       		    info[1]))
    //     end
    //     s = s .. string.format(" %d %f",state2cod[info[1]],info[2])
    //   else
    //     s = s .. " -1 -1" -- valores especiales para indicar que no hay backoff
    //   end
    //   if upperBoundBestProb[statename] then
    //     s = s .. string.format(" %f", upperBoundBestProb[statename])
    //   end
    //   fprintf(output_file,"%s\n",s)
    //   if math.modf(stcod,100000) == 0 then collectgarbage("collect") end
    // end
    
    // -- las transiciones
    // fprintf(output_file,"# transitions\n# orig dest word prob\n")
    // for stcod=0,num_state do
    //   local statename = cod2state[stcod]
    //   --local trans = state2transitions[statename]
    //   local trans = state2transitions:getTransitions(statename)
    //   --for i = 1,#trans,3 do
    //   for i=1,trans:size() do
    //     local dest,word,prob = trans:get(i) --trans[i],trans[i+1],trans[i+2]
    //     if verbosity > 1 then
    //       fprintf(output_file,"# %s -> %s %s %g\n",statename,dest,word,prob)
    //     end
    //     fprintf(output_file,"%d %d %d %g\n",
    //             stcod,state2cod[dest],word,prob)
    //   end
    //   if math.modf(stcod,10000) == 0 then collectgarbage("collect") end
    // end
    
    // output_file:close()
  }    



} // namespace Arpa2Lira
