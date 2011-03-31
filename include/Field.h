#ifndef FIELD_H
#define FIELD_H

#include <map>

typedef std::pair<int, int> LocationType;
typedef std::map<LocationType, double> FieldType;

class Field
{
    public:
        enum OperationType
        {
            opUnion, opIntersection, opSymmetricDifference
        };

        Field(const std::string& nName);

        std::string name;

        std::pair<double, bool> get(int x, int y);
        FieldType::iterator begin();
        FieldType::iterator end();
        void set(int x, int y, double f);
        void setAll(double f);

        void addField(Field *f, double scale);
        void subtractField(Field *f, double scale);
        void clear();
        std::pair<LocationType, double> min();
        std::pair<LocationType, double> max();

        void refreshMeshes(double offset);
        void createMeshes(double offset);
        void destroyMeshes();

    private:
        FieldType theField;
        bool hasMeshes;
};

#endif

