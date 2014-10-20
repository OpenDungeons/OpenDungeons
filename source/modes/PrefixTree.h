/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PREFIX_TREE_H_
#define PREFIX_TREE_H_

#include <iostream>
#include <list>
#include <string>
#include <utility>

class PrefixTree
{
public:
    PrefixTree();

    void addNewString(const std::string&);
    bool readStringsFromFile(const std::string&);
    bool printAll();
    PrefixTree* findPrefix(const std::string&);
    int build(const char *filename);
    bool complete(const char *const word, std::list<std::string>* ll);

private:
    std::list<std::pair<char, PrefixTree*> > mSiblingsList;
    char mNodeChar;
    PrefixTree* mParent;
    bool mValidWord;

    void addNewStringAux(std::string::const_iterator, std::string::const_iterator, std::string::const_iterator);
    PrefixTree* findPrefixAux(std::string::const_iterator, std::string::const_iterator);
    bool completePlusPrefix(const std::string&, std::list<std::string>* ll);
};

#endif // PREFIX_TREE_H_
