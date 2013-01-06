/**
 * \file    check.cpp
 * 
 * \brief Demo Check PrefixGraph.
 * 
 * Probably you want to run script test.sh which do all the tests.
 * 
 * Program checks if PrefixGraph contains a list of words.
 * 
 * Run this program with the name of PrefixGraph file as the first argument and the name of file with a list of words as the second.\n
 * First you should create PrefixGraph file by using program convert from this directory
 */


#include "string.h"
#include <iostream>
#include "PrefixTreeLL.h"


using namespace std;

template <class Dict>
int check(Dict *dt, const char* filename) {
  int errors = 0;
	
  long size ;
	
  char *buffer = loadFile(filename, size);

  char *word = buffer;
	
  for(long i=0; i<size; i++) if(word[i] == '\n' ) word[i] = '\0';
	
  long words_num=0;
  while( word < buffer+size ) {
    words_num++;
    if( !dt->has(word) ) {
      errors++;
    }
    word += strlen(word)+1;
  }
	
  cout<<words_num<<" words checked\n";
	

  return errors;
}


// int main(int argc, char** argv)
// {
// 	if(argc==3) {
// 		try {
// 			PrefixTreeLL* dt = new PrefixTreeLL;
// 			dt->build(argv[1]);
// 			cout<<"checking..\n";
// 			int errors = check(dt, argv[2]);
			
// 			if(errors) {
// 				cout<<"error: "<<errors<<" words not found\n";
// 				return 1;
// 			}
// 			else cout<<"ok"<<endl;
		
// 			delete dt;
// 		}
// 		catch (exception &e) {
// 			cout<<"exception: "<<e.what()<<endl;
// 			cout<<"           maybe file doesn't exist\n";
// 			return 1;
// 		}
// 		catch(...) {
// 			cout<<"exception has ocurred\n";
// 			return 1;
// 		}
// 	}
// 	else {
// 		cout<<"program check if words from file are in Dictionary\n";
// 		cout<<" give a name of dictionary file in PrefixGraph format as an argument\n";
// 		cout<<" give a name of file with a list of words as a second argument\n";
// 		cout<<"  every word has to be ended by \\n\n";
// 	}
	
// 	return 0;
// }




int main(int argc, char** argv)
{
  list<string> ll ; 
  string prefixString;
  try {
    PrefixTreeLL* dt = new PrefixTreeLL;
    dt->build(argv[1]);
    cout<<"ready...\n";
    cin >> prefixString;
    while(prefixString!="exit"){
      ll.clear();
      dt->complete( prefixString.c_str() , &ll);
      for(list<string> :: iterator it = ll.begin(); it != ll.end() ; it++ ){
	cout << *it   << endl;

      }
      cout<<"Those prefix were found " << endl;
      cin >> prefixString;

			
    }
    delete dt;			
  }

		

catch (exception &e) {
  cout<<"exception: "<<e.what()<<endl;
  cout<<"           maybe file doesn't exist\n";
  return 1;
 }
 catch(...) {
   cout<<"exception has ocurred\n";
   return 1;
 }

return 0;
}



         
