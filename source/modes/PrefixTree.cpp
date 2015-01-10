/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "PrefixTree.h"

#include <fstream>
#include <algorithm>

PrefixTree::PrefixTree():
    mParent(nullptr),
    mValidWord(false)
{}

void PrefixTree::addNewString(const std::string& ss)
{
    addNewStringAux(ss.begin(), ss.begin(), ss.end());
}

void PrefixTree::addNewStringAux(std::string::const_iterator it,
                                 std::string::const_iterator ssBegin,
                                 std::string::const_iterator ssEnd)
{
    if(it == ssEnd)
    {
        mValidWord = true;
        return;
    }

    std::vector<std::pair<char, PrefixTree*> >::iterator result;
    result = std::find_if(mSiblingsList.begin(), mSiblingsList.end(),
                            [it](std::pair<char,PrefixTree*> pp)
                            {
                                return (*it == pp.first);
                            });

    if(result != mSiblingsList.end())
    {
        ++it;
        result->second->addNewStringAux(it, ssBegin, ssEnd);

    }
    else
    {
        PrefixTree* prefixTree = new PrefixTree();
        mSiblingsList.push_back(std::make_pair(*it, prefixTree));
        ++it;
        prefixTree->addNewStringAux(it, ssBegin, ssEnd);
    }

}

bool PrefixTree::complete(const char* word, std::vector<std::string>& ll)
{
    std::string wordString(word);
    //std::cout << wordString << std::endl;

    PrefixTree* ff = findPrefix(wordString);
    if(ff != nullptr)
        return ff->completePlusPrefix(std::string(), ll);
    else
        return false;
}

bool PrefixTree::completePlusPrefix(const std::string& ss, std::vector<std::string>& ll)
{
    if(mValidWord)
        ll.push_back(ss);

    std::vector<std::pair<char, PrefixTree*> >::iterator it;
    for(it = mSiblingsList.begin(); it != mSiblingsList.end(); ++it)
    {
        std::string tt = ss;
        tt += it->first;
        it->second->completePlusPrefix(tt, ll);
    }
    return true;
}

PrefixTree* PrefixTree::findPrefix(const std::string& ss)
{
    return findPrefixAux(ss.begin(), ss.end());
}

PrefixTree* PrefixTree::findPrefixAux(std::string::const_iterator it, std::string::const_iterator end_it)
{
    if(it == end_it)
        return this;

    std::vector<std::pair<char, PrefixTree*> >::iterator result;
    result = std::find_if(mSiblingsList.begin(), mSiblingsList.end(),
                            [it](std::pair<char, PrefixTree*> pp)
                            {
                                return (*it == pp.first);
                            } );

    if(result == mSiblingsList.end())
        return nullptr;

    return result->second->findPrefixAux(++it, end_it);
}
