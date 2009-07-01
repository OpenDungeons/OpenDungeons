#include "Field.h"

/*! \brief Returns the stored value at a position (or 0) and a boolean indicating whether the value was wactually found.
 *
 */
pair<double,bool> Field::get(int x, int y)
{
	LocationType location(x, y);
	FieldType::iterator itr;
	itr = theField.find(location);
	bool found = (itr != theField.end());
	
	return pair<double,bool>( found?(*itr).second:0.0, found );
}

/*! \brief Sets the field value at location (x,y) to the value f adding that place if necessary.
 *
 */
void Field::set(int x, int y, double f)
{
	LocationType location(x, y);
	theField[location] = f;
}

/*! \brief Loops over all the places currently set and sets each one to f.
 *
 */
void Field::setAll(double f)
{
	FieldType::iterator itr;
	for(itr = theField.begin(); itr != theField.end(); itr++)
	{
		(*itr).second = f;
	}
}

/*! \brief Adds the values in f to the values stored in theField.
 *
 * Nonexistant tiles in theField are treated as if their original value was
 * zero (i.e. a new entry is created and set the f(x, y)).
 */
void Field::addField(Field *f, double scale=1.0)
{
	FieldType::iterator itr, thisOne;
	for(itr = f->theField.begin(); itr != f->theField.end(); itr++)
	{
		double fValue = (*itr).second;
		int fX = (*itr).first.first;
		int fY = (*itr).first.second;
		thisOne = theField.find(LocationType(fX, fY));
		if(thisOne != theField.end())
		{
			(*thisOne).second += scale*fValue;
		}
		else
		{
			theField[(*itr).first] = scale*fValue;
		}
	}
}

void Field::subtractField(Field *f, double scale=1.0)
{
	addField(f, -1.0*scale);
}

