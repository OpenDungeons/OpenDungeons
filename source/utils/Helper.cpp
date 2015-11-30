/*!
 * \file   Helper.cpp
 * \date   10 September 2011
 * \author StefanP.MUC, hwoarangmy, Bertram
 * \brief  Provides helper functions, constants and defines
 *
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

#include "utils/Helper.h"

#include "utils/LogManager.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace Helper
{
    std::vector<std::string> split(const std::string& line, char delimiter, bool removeEmpty)
    {
        //std::cout << line << std::endl;
        std::stringstream ss(line);
        std::vector<std::string> elems;
        std::string item;
        while (std::getline(ss, item, delimiter))
        {
            if(removeEmpty && item.empty())
                continue;

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

    uint32_t toUInt32(const std::string& text)
    {
        std::stringstream ss(text);
        uint32_t number = 0;
        ss >> number;
        return number;
    }

    float toFloat(const std::string& text)
    {
        std::stringstream ss(text);
        float number = 0.0;
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

    int round(float f)
    {
        return static_cast<int>(f + 0.5f);
    }

    void trim(std::string& str)
    {
        boost::algorithm::trim(str);
    }

    bool fillDirList(const std::string& path,
                       std::vector<std::string>& listDir,
                       bool absoluteDir)
    {
        const boost::filesystem::path dir_path(path);
        if (!boost::filesystem::exists(dir_path))
            return false;
        boost::filesystem::directory_iterator end_itr;
        for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr)
        {
            if(!boost::filesystem::is_directory(itr->status()))
                continue;

            if(absoluteDir)
                listDir.push_back(boost::filesystem::canonical(itr->path()).string());
            else
                listDir.push_back(itr->path().filename().string());
        }
        return true;
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
                {
                    boost::filesystem::path p = boost::filesystem::canonical(itr->path());
                    listFiles.push_back(p.string());
                }
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

    bool readFileWithoutComments(const std::string& fileName, std::stringstream& stream)
    {
        // Try to open the input file for reading and throw an error if we can't.
        std::ifstream baseLevelFile(fileName.c_str(), std::ifstream::in);
        if (!baseLevelFile.good())
        {
            OD_LOG_WRN("File not found=" + fileName);
            return false;
        }

        // Read in the whole baseLevelFile, strip it of comments and feed it into
        // the stream.
        std::string nextParam;
        while (baseLevelFile.good())
        {
            std::getline(baseLevelFile, nextParam);
            /* Find the first occurrence of the comment symbol on the
             * line and return everything before that character.
             */
            stream << nextParam.substr(0, nextParam.find('#')) << "\n";
        }

        baseLevelFile.close();

        return true;
    }

    std::string toString(void* p)
    {
        std::stringstream stream;
        stream << p;
        return stream.str();
    }

    std::string intTo2Hex(int i)
    {
        std::stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::hex << i;
        return stream.str();
    }

    std::string getCEGUIColorFromOgreColourValue(const Ogre::ColourValue& color)
    {
        std::string colourStr = Helper::intTo2Hex(static_cast<int>(color.a * 255.0f))
                                + Helper::intTo2Hex(static_cast<int>(color.r * 255.0f))
                                + Helper::intTo2Hex(static_cast<int>(color.g * 255.0f))
                                + Helper::intTo2Hex(static_cast<int>(color.b * 255.0f));
        return colourStr;
    }

    std::string getImageColoursStringFromColourValue(const Ogre::ColourValue& color)
    {
        std::string colourStr = Helper::getCEGUIColorFromOgreColourValue(color);
        std::string imageColours = "tl:" + colourStr + " tr:" + colourStr + " bl:" + colourStr + " br:" + colourStr;
        return imageColours;
    }

    std::string toString(float f, unsigned short precision)
    {
        std::ostringstream oss;
        oss << std::setprecision(precision) << f;
        return oss.str();
    }
    std::string toString(double d, unsigned short precision)
    {
        std::ostringstream oss;
        oss << std::setprecision(precision) << d;
        return oss.str();
    }
    std::string toString(int8_t d)
    {
        return TTostring(d);
    }
    std::string toString(uint8_t d)
    {
        return TTostring(d);
    }
    std::string toString(int16_t d)
    {
        return TTostring(d);
    }
    std::string toString(uint16_t d)
    {
        return TTostring(d);
    }
    std::string toString(int32_t d)
    {
        return TTostring(d);
    }
    std::string toString(uint32_t d)
    {
        return TTostring(d);
    }
    std::string toString(int64_t d)
    {
        return TTostring(d);
    }
    std::string toString(uint64_t d)
    {
        return TTostring(d);
    }
    std::string toString(const Ogre::Vector3& v)
    {
        return "(" + TTostring(v.x)
            + "," + TTostring(v.y)
            + "," + TTostring(v.z) + ")";
    }
    std::string toStringWithoutZ(const Ogre::Vector3& v)
    {
        return "(" + TTostring(v.x)
            + "," + TTostring(v.y) + ")";
    }
    std::string toString(const Ogre::ColourValue& c)
    {
        return "[" + TTostring(c.r)
            + "," + TTostring(c.g)
            + "," + TTostring(c.b)
            + "," + TTostring(c.a) + "]";
    }
} // namespace Helper
