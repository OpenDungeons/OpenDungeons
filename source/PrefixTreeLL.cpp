/**
 * \file    PrefixGraph.cpp
 * \author  Jakub Stepien <thumren@gmail.com>
 * \version 0.1
 * \date    Sep, 2011
 *
 * 
 * \section LICENSE
 *
 * Copyright 2011 Jakub Stepien. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of copyright holder.
 */


#include "PrefixTreeLL.h"
#include <stdlib.h>
#include <fstream>
#include <iomanip>

using namespace std;


char* loadFile(const char* filename, long& size) {
    size = 0;
    
    ifstream file(filename, ifstream::binary);

    // get length of file
    file.seekg(0, ios::end);
    size = file.tellg();

    if(size < 1)
    {
        std::cerr << filename << " not exist" << std::endl;
        return 0;
    }

    // allocate memory
    char* buffer = new char[size];
    
    // read data
    try {
        file.seekg(0, ios::beg);
        file.read(buffer, size);
    }
    catch(...) {
        size = 0;
        delete[] buffer;
        throw;
    }
    
    return buffer;
}

void PrefixTreeLL::clear() {
    free(letter);
    free(next);
    size = 0;
    letter = NULL;
    next = NULL;
}

void PrefixTreeLL::show(std::ostream &out) {
    out<<setw(10)<<left<<"index"<<right;
    for(unsigned int i=0; i<size; i++) out<<setw(5)<<i;
    out<<endl;
    
    out<<setw(10)<<left<<"letter"<<right;
    for(unsigned int i=0; i<size; i++) {
        out<<setw(5);
        if(letter[i] != '\n' ) out<<letter[i];
        else out<<'\\';
    }
    out<<endl;

    out<<setw(10)<<left<<"next"<<right;
    for(unsigned int i=0; i<size; i++) out<<setw(5)<<next[i];
    out<<endl;
}

bool PrefixTreeLL::has(const char* word) {
    unsigned int i = 0;
    cerr << "trying " << word << endl; 
    while( *word ) {
        if( *word == letter[i] ) {

                cerr<<"*word == letter[i] letter[i]:  "<< letter[i] <<  endl;
            i++;
            word++;
        }
        // this line speeds up checking but may cause errors if there are national characters and dictionary is not sorted by ASCII codes
        // else if( *str < letter[i] ) return false;
        else if( next[i] >= 0 ) { cerr << "next[i] >= 0  next[i]: " <<  next[i] << "letter[i]: " << letter[i] << " " << int(letter[i]) <<  endl ;
          i = next[i]; }
        else return false;
    }
    return letter[i] == '\n';
}

string PrefixTreeLL::complete(const char *const word, list<string>* ll ){

        cerr << word << endl;
    show(cerr);
    string infix = "";
        const char* ww = word;
    
    string postfix = "";
    int ii = 0;

    while( *ww ) {
        if( *ww == letter[ii] ) {
            ii++;
            ww++;
        }
        // this line speeds up checking but may cause errors if there are national characters and dictionary is not sorted by ASCII codes
        // else if( *str < letter[i] ) return false;
        else if( next[ii] >= 0 ) ii = next[ii];
        else return infix;
    }




    while(next[ii]< 0 && letter[ii] != '\n'){
      infix.push_back(letter[ii]);
        ii++;
    }
    
    complete_aux(ll, ii,postfix);
    return infix;

}

void PrefixTreeLL::complete_aux(  list<string>* ll, int ii, string postfix   ){


  int saved_ii = ii;



  if(next[ii]>=0){

     ii = next[ii];
   
     complete_aux (ll, ii,postfix);
   

  }
  ii = saved_ii;

  if(letter[ii] == '\n') {

    ll->push_back(postfix);    
    
  }
  else{
    postfix.push_back(letter[ii]);
    ii++;
    complete_aux (ll, ii,postfix);    
    
    
  }



  return ;

    
}




/// \todo Check malloc/realloc return values.
int PrefixTreeLL::build(const char *filename) {
    clear();
    
    // load word list
    long inputBufferSize;
    char *inputBuffer = loadFile(filename, inputBufferSize);
    
    // check if last word ends with '\n'
    if( !(inputBufferSize>1 && inputBuffer[inputBufferSize-1] == '\n' && inputBuffer[inputBufferSize-2] != '\n') ) {
        cerr<<"error in PrefixTreeLL::build:\n";
        cerr<<" check input file\n";
        cerr<<" words have to be ended by '\\n' character\n";
        cerr<<" there has to be only one '\\n' on the end of the file\n";
        delete[] inputBuffer;
        return 2;
    }
    
    // i like puzzles so it's implemented on arrays for fun and better performance :P
    
    // number of words in the input file
    unsigned int wordsNum = 0;
    
    // index of currently added letter in the input buffer
    long index = 0;
    // index of currently added letter in the word
    unsigned int indexInWord = 0;
    
    // letters of last added word are in the 'letter' array
    // every element of array 'lastWord' is an index
    // example:
    //  if you add word 'foo' to dictionary this word will be saved in 'letter' buffer
    //  letter[ lastWord[0] ] = 'f'
    //  letter[ lastWord[1] ] = 'o'
    // 'lastWord' serves to calculate values of the 'next' array
    unsigned int *lastWord = NULL;
    // size of the last added word ( content after lastWordLen elements is not valid )
    unsigned int lastWordLen = 0;
    
    // allocate memory
    unsigned int lastWordAllocated = 50;
    lastWord = (unsigned int*)malloc(lastWordAllocated*sizeof(unsigned int));
    unsigned int allocated = 5000;
    letter = (char*) malloc(allocated);
    next = (int*) malloc(allocated*sizeof(int));
    
    // iterate through all characters in the buffer
    while( index < inputBufferSize ) {
        // we add the data to the output only when
        // current word is longer than last added word or
        // current character is different from a character in previously added word at the same position
        if( indexInWord >= lastWordLen || ( inputBuffer[index] != letter[ lastWord[indexInWord] ] )) {
            // check if we have enough memory allocated
            if( indexInWord >= lastWordAllocated ) {
                // let's allocate two times more memory, why not
                lastWordAllocated *= 2;
                lastWord = (unsigned int*)realloc(lastWord, lastWordAllocated*sizeof(unsigned int));
            }
            if( size >= allocated ) {
                // practically this executes only once
                
                // allocate more memory than from estimation: inputBufferSize*size/index
                // (for the case when the end of inputBuffer is not so easy to pack as the begining)
                // arbitrary choosen these extra bytes equal to previously allocated size of memory
                allocated += float(inputBufferSize)*size/index;
                letter = (char*) realloc(letter, allocated);
                next = (int*) realloc(next, allocated*sizeof(int));
            }
            
            // if current character is different from a character in previously added word at the same position
            // then this is the next alternative letter
            if( indexInWord < lastWordLen ) next[ lastWord[indexInWord] ] = size;
            
            lastWord[indexInWord] = size;
            lastWordLen = indexInWord+1;

            letter[size] = inputBuffer[index];
            next[size] = -1;
            size++;
        }
        
        if( inputBuffer[index] == '\n') {
            indexInWord = 0;
            // counts words correctly only when there are no duplicates
            wordsNum++;
        }
        else indexInWord++;
        index++;
    }
        
    free(lastWord);
    lastWord = NULL;
    delete[] inputBuffer;
    inputBuffer = NULL;
    
    letter = (char*) realloc(letter, size);
    next = (int*) realloc(next, size*sizeof(int));

    clog<<"PrefixTreeLL::build: dictionary of "<<wordsNum<<" words written in "<<size<<" nodes\n";
    
    return 0;
}

void PrefixTreeLL::save(const char* filename) {
    ofstream file(filename, ofstream::binary);
    file.write(letter, size);
    file.write((char*)next, long(size)*sizeof(*next));
}   

void PrefixTreeLL::load(const char* filename) {
    clear();
    
    ifstream file;
    file.open(filename, ifstream::binary);
    
    // get length of file
    file.seekg(0, ios::end);
    long sizeInBytes = file.tellg();
    
    // calculate arrays size
    const int nodeSize = sizeof(*letter)+ sizeof(*next);
    if( sizeInBytes%nodeSize != 0 ) {
        throw "error in PrefixTreeLL::load: wrong file format\n";
    }
    size = sizeInBytes/nodeSize;
    
    // allocate memory
    letter = (char*) malloc(size);
    next = (int*) malloc(size*sizeof(int));
    
    // read data
    try {   
        file.seekg(0, ios::beg);
        file.read(letter, size*sizeof(*letter));
        file.read((char*)next, size*sizeof(*next));
    }
    catch(...) {
        clear();
        throw;
    }
}
