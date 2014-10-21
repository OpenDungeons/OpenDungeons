/*!
 * \file   Helper.cpp
 * \date   10 September 2011
 * \author StefanP.MUC, hwoarangmy, Bertram
 * \brief  Provides helper functions, constants and defines
 *
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

#include "utils/Helper.h"

#include "boost/filesystem.hpp"

namespace Helper
{
    std::vector<std::string> split(const std::string& line, char delimiter)
    {
        //std::cout << line << std::endl;
        std::stringstream ss(line);
        std::vector<std::string> elems;
        std::string item;
        while (std::getline(ss, item, '\t'))
        {
            elems.push_back(item);
            //std::cout << item << std::endl;
        }
        return elems;
    }

    int toInt(const std::string& text)
    {
        std::stringstream ss(text);
        int number = 0;
        ss >> number;
        return number;
    }

    double toDouble(const std::string& text)
    {
        std::stringstream ss(text);
        double number = 0.0;
        ss >> number;
        return number;
    }

    int round(double d)
    {
        return static_cast<int>(d + 0.5);
    }

    bool fillFilesList(const std::string& path,
                       std::vector<std::string>& listFiles,
                       const std::string& fileExtension)
    {
        const boost::filesystem::path dir_path(path);
        if (!boost::filesystem::exists(dir_path))
            return false;
        boost::filesystem::directory_iterator end_itr;
        for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr)
        {
            if(!boost::filesystem::is_directory(itr->status()))
            {
                if(itr->path().filename().extension().string() == fileExtension)
                    listFiles.push_back(itr->path().string());
            }
        }
        return true;
    }

    bool fillFileStemsList(const std::string& path,
                           std::vector<std::string>& listFiles,
                           const std::string& fileExtension)
    {
        const boost::filesystem::path dir_path(path);
        if (!boost::filesystem::exists(dir_path))
            return false;
        boost::filesystem::directory_iterator end_itr;
        for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr )
        {
            if(!boost::filesystem::is_directory(itr->status()))
            {
                if(itr->path().filename().extension().string() == fileExtension)
                    listFiles.push_back(itr->path().filename().stem().string());
            }
        }
        return true;
    }
} // namespace Helper
