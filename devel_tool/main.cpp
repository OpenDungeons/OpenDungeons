#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <boost/algorithm/string/replace.hpp>






using std::cout ; using std::cin; using std::endl; using std::cerr; using std::string; using std::vector;


int main(int argc, char** argv){





    if(argc < 5  ){
	cout << "Usage " << argv[0] << " xmlPatternFile  rawDataFile outputXMLPrefix substitutionAlias "  << endl;
	
	return 1 ; 
    	}

    std::ifstream xmlPatternFile(argv[1]);    
    std::ifstream rawDataFile(argv[2]);
    std::vector<string> tokens;
    
    std::stringstream outputXMLPrefix(argv[3]);
    std::stringstream substitutionAlias(argv[4]);

    // cerr << outputXMLPrefix.str()<<endl;
    // cerr << substitutionAlias.str()<<endl;

    std::stringstream   ss2 ;
    ss2 << xmlPatternFile.rdbuf();
    std::stringstream   ss3;
    std::stringstream   ss4;
    std::stringstream searchedAlias;
 
    int fileLineNumber = 0 ; 
    
    cerr << rawDataFile;
    std::string oneLine;
    while( std::getline (rawDataFile , oneLine  )  ){
	
	ss3 << outputXMLPrefix.str() << "_" << fileLineNumber << ".xml" ;
	std::ofstream outputXML(ss3.str().c_str());
	std::istringstream iss(oneLine);
	cerr << oneLine << endl ;
	
	copy(std::istream_iterator<string>(iss),
	    std::istream_iterator<string>(),
	    std::back_inserter<vector<string> >(tokens));
	
	cerr << tokens[0] << endl;

	std::string fileString(ss2.str());
	int rowNumber =9;
	for(std::vector<string>::iterator ii = tokens.begin()  ;  ii != tokens.end()  ; ii++, rowNumber++){

	searchedAlias.str(std::string());
        searchedAlias << substitutionAlias.str() << rowNumber;
	// cerr << rowNumber;
	boost::replace_all( fileString, searchedAlias.str(), *ii);
       
	// cerr << fileString << endl;

	    }


	outputXML << fileString;
	outputXML.close();
	ss4.str(std::string());
	ss3.str(std::string());

	tokens.clear();
	fileLineNumber++;
	}


    // while(){
	
    // 	outputXML << ss1;


    // 	}



    // std::stringstream ss1;
    // int nn = 0; 
    // ss1.clear();
    // std::stringstream ss2;
    // ss2.clear();

    // for(nn=0 ; nn < 10 ; nn++){
    // ss2 << "definition_"; 
    // ss2 << nn;
    // ss2   << ".xml";
    

    // std::ofstream mInputFile(ss2.str().c_str());	




    // ss2.str(std::string());
           
   return 0 ; 


}
