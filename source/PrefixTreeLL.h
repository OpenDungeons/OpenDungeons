/**
 * \file    PrefixTreeLL.h
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


#ifndef __PrefixTreeLL__
#define __PrefixTreeLL__


#include <iostream>

/**
 * \brief Load data from file.
 * 
 * It throws an exception if something wrong.
 * \param [out] size number of bytes read.
 * \return pointer on data buffer.
 */
char* loadFile(const char* filename, long& size);

/**
 * \brief A linked list prefix tree.
 * 
 * It's linked list implementation of prefix tree structure.
 * 
 * \note
 * Probably you should use PrefixGraph class in your program.
 * PrefixGraph is both faster and less memory consuming with the same funcionality.
 * PrefixTreeLL is provided only to help building PrefixGraph. 
 */
class PrefixTreeLL {
	//template<class>
	friend class PrefixGraphBuilder;
	
protected:
	/// size of internal arrays
	unsigned int size;
	
	/// letters array
	char *letter;

	/**
	 * \brief positions of alternative letters
	 * 
\verbatim
For dictionary:
 ma
 mama
 tama

Notice that words "ma" and "mama" have the same prefixe.
The trie structure compresses all redundant prefixes into one:
 ma
   ma
 tama

Contents of arrays:
 letters = m  a \n  m  a \n  t  a  m  a \n
 next    = 6 -1  3 -1 -1 -1 -1 -1 -1 -1 -1

 next[0] = 6 because letter[6] = 't' is the next alternative letter (both 'm' and 't' are on the same position in words "ma" and "tama")
 next[2] = 3 because letter[3] = 'm' is the next alternative letter ( letter[2] = '\\n', this is the end of word character )
 If there is no next letter then value -1 is in the array.
\endverbatim
	 */
	int *next;
	
public:

	PrefixTreeLL() {
		size = 0;
		letter = NULL;
		next = NULL;
	}
	
	~PrefixTreeLL() {
		clear();
	}

	/**
	 * \brief Deallocate memory.
	 */
	void clear();
	
	/**
	 * \brief Print internal arrays.
	 * 
	 * Use only for small dictionaries.
	 */
	void show(std::ostream &out);
	
	/**
	 * \brief Check if trie contains a word.
	 * 
	 * \param word ends with \\0.
	 */
	bool has(const char* word);
	
	/**
	 * \brief Build a trie from words listed in file.
	 * 
	 * Words have to be sorted alphabetically.
	 * Every word ends with \\n, not with \\0.
	 * 
	 * \return 0 if succeed.
	 */
	int build(const char *filename);
	
	/**
	 * \brief Write trie into file.
	 */
	void save(const char* filename);
	
	/**
	 * \brief Load trie from file.
	 * 
	 * File should be created by save(filename).
	 * It's faster than building it from a list of words.
	 * It throws an exception if something wrong.
	 */
	void load(const char* filename);
};

#endif
