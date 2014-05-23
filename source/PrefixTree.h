#include <iostream>
#include <list>
#include <string>
#include <utility>


using std::cout ; using std::cin; using std::endl;
using std::string; using std::list; using std::pair;


class PrefixTree {
private:
    list<pair<char,PrefixTree*> > siblingsList ;
    char NodeChar;
    PrefixTree* parent;
    bool validWord;
    bool addNewStringAux(string::const_iterator, string::const_iterator, string::const_iterator ) ;
    PrefixTree* findPrefixAux(string::const_iterator , string::const_iterator );
    bool completePlusPrefix(string,  list<string>* ll);

public: 
    PrefixTree();
    bool addNewString(string);
    bool readStringsFromFile(string);
    bool printAll();
    PrefixTree* findPrefix(string );
    int build(const char *filename);
    bool complete(const char *const word, list<string>* ll );



};
