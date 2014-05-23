#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <algorithm>
#include "PrefixTree.h"

using std::cout ; using std::cin; using std::endl; using std::make_pair;

PrefixTree::PrefixTree():validWord(false){}
bool PrefixTree::addNewString(string ss){
    return addNewStringAux(ss.begin(), ss.begin(), ss.end());

}

bool PrefixTree::addNewStringAux(string::const_iterator ii, string::const_iterator ssBegin, string::const_iterator ssEnd ){

    if( ii != ssEnd){

        PrefixTree* tmpPtr;
	auto result = std::find_if(siblingsList.begin(), siblingsList.end(), [ii](pair<char,PrefixTree*> pp){  return (*ii == pp.first );} );

	if(result != siblingsList.end()){
	    ii++;
	    result->second->addNewStringAux(ii, ssBegin, ssEnd);

	}
	else{
	    tmpPtr = new PrefixTree();
	    siblingsList.push_back(make_pair(*ii, tmpPtr));
	    ii++;
	    tmpPtr->addNewStringAux(ii, ssBegin, ssEnd);
	}	
    }
    else{
	validWord = true;

    }
}

bool PrefixTree::readStringsFromFile(string fileName){

    std::string str;
    std::fstream fileStream (fileName);
    cout << fileStream.good() << endl; 
    while (std::getline(fileStream, str)) {
	addNewString(str);
    }
}

bool PrefixTree::complete(const char * word, list<string>* ll ){

    string wordString(word );
    cout << wordString << endl;
    auto ff = findPrefix(wordString);
    if( ff !=nullptr)
	return ff->completePlusPrefix(string(""), ll );
    else 
	return false;
}

bool PrefixTree::completePlusPrefix(string ss, list<string>* ll){
    if(validWord)
	ll->push_back(ss);

    for(auto ii = siblingsList.begin(); ii != siblingsList.end(); ii++ ){
	std::string tt = ss;
	tt += ii->first;
	ii->second->completePlusPrefix(tt, ll);
    }
}


PrefixTree* PrefixTree::findPrefix(string ss){


    return findPrefixAux(ss.begin(), ss.end());

}


PrefixTree* PrefixTree::findPrefixAux(string::const_iterator ii, string::const_iterator end_ii){

    if(ii == end_ii)
	return this;
    else{
	auto result = std::find_if(siblingsList.begin(), siblingsList.end(), [ii](pair<char,PrefixTree*> pp){  return (*ii == pp.first );} );	
	int nn = 2 + 2;
	auto tmpDebug = siblingsList.end();
	if( result == siblingsList.end())
	    return nullptr;

	else
	    result->second->findPrefixAux(++ii,end_ii) ;
    }
}








