char* actionTree;



bool readActionTree(char* const expressionChar){
std::ifstream t;
int length;
t.open("ActionTree.txt");     // open input file
t.seekg(0, std::ios::end);    // go to the end
length = t.tellg();           // report location (this is the length)
t.seekg(0, std::ios::beg);    // go back to the beginning
expressionChar = new char[length];    // allocate memory for a buffer of appropriate dimension
t.read(buffer, length);       // read the whole file into the buffer
t.close();  
}

bool checkBracesValidity(char* const expressionChar){
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
