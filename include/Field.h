#ifndef FIELD_H
#define FIELD_H

#include <iostream>
#include <utility>
#include <map>
using namespace std;

typedef pair<int,int> LocationType;
typedef map<LocationType, double> FieldType;

class Field
{
	public:
		enum OperationType {opUnion, opIntersection, opSymmetricDifference};

		pair<double,bool> get(int x, int y);
		void set(int x, int y, double f);
		void setAll(double f);

		void addField(Field *f, double scale);
		void subtractField(Field *f, double scale);
		void clear();

	private:
		FieldType theField;
};

#endif

