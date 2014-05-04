#ifndef DUMMYARRAYCLASS_H_
#define DUMMYARRAYCLASS_H_


#include <array>
#include <vector>
#include "Vector3i.h"

using std::vector;


struct DummyArrayClass{
    std::array<Vector3i,4>  myArray;
public:

    bool clockwise(int ii , int jj);
    bool anticlockwise(int ii , int jj);
    Vector3i* getClockwiseNext();
    Vector3i* getAntiClockwiseNext();
    Vector3i& operator[](int ii) ;
    void sort();
    void sortByPhi(vector<Vector3i> &ll, Vector3i& cc);
    
	};
#endif /* DUMMYARRAYCLASS_H_ */
