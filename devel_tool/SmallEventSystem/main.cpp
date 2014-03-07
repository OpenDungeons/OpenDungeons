#include <iostream>
#include <cctype>

#include "main.h"


using std::cin; using std::cout; using std::


int main (char argv** , int argc){





}


bool checkBracesValidity(char* expressionChar){
    int monitor = 0;
    int ii = 0 ;

    while(expressionChar[ii]!='\0' ){
	if(expressionChar[ii]=='{')
	    monitor++;
	else if(expressionChar[ii]=='}')
	    monitor--;

	if(monitor < 0 )
	    return false;
	ii++;
   
    }
    return true;
}
