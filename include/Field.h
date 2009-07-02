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

		Field(string nName);

		string name;

		pair<double,bool> get(int x, int y);
		FieldType::iterator begin();
		FieldType::iterator end();
		void set(int x, int y, double f);
		void setAll(double f);

		void addField(Field *f, double scale);
		void subtractField(Field *f, double scale);
		void clear();
		pair<LocationType, double> min();

		void refreshMeshes(double offset);
		void createMeshes(double offset);
		void destroyMeshes();

	private:
		FieldType theField;
		bool hasMeshes;
};

#endif

