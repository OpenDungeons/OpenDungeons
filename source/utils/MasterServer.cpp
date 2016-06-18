/*
*  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "utils/MasterServer.h"

#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <SFML/Network.hpp>

// We will replace all special meaning chars to make sure they are correctly read
// by the master server. For example, since ';' is the separation for CSV,
// we don't want to use some in the texts as it may break our reading.
static const char replacementChar = '_';
static const std::vector<std::pair<char,char>> specialChars = {
    std::pair<char,char>(replacementChar, '0'),
    std::pair<char,char>('\n', '1'),
    std::pair<char,char>(';', '2'),
    std::pair<char,char>('&', '3')
};

namespace MasterServer
{
    bool fillMasterServerGames(const std::string& odVersion, std::vector<MasterServerGame>& masterServerGames)
    {
        sf::Http::Request request("/get_csv.php", sf::Http::Request::Get);
        sf::Http http(ConfigManager::getSingleton().getMasterServerUrl());
        sf::Http::Response response = http.sendRequest(request);

        if (response.getStatus() != sf::Http::Response::Ok)
            return false;

        // Read pending games
        std::stringstream ss(response.getBody());
        std::string line;
        while(!ss.eof())
        {
            if(!Helper::readNextLineNotEmpty(ss, line))
                break;

            std::vector<std::string> elems = Helper::split(line, ';');
            if(elems.size() != 7)
                continue;

            // Whatever the OD version is, the first column should be the version. We process only lines
            // where the version matches
            if(odVersion != formatStringFromMasterServer(elems[0]))
                continue;

            int32_t port = Helper::toInt(elems[4]);
            masterServerGames.emplace_back(
                formatStringFromMasterServer(elems[1]),
                formatStringFromMasterServer(elems[2]),
                formatStringFromMasterServer(elems[3]),
                port,
                formatStringFromMasterServer(elems[5]),
                formatStringFromMasterServer(elems[6]));
        }
        return true;
    }

    bool registerGame(const std::string& odVersion, const std::string& creator, int32_t port, const std::string& label, const std::string& descr, std::string& uuid)
    {
        // Before sending the level informations, we format the strings to avoid special meaning chars
        // like '\n' or ';'
        sf::Http::Request request("/announce.php", sf::Http::Request::Post);
        std::string body = "odVersion=" + formatStringForMasterServer(odVersion)
            + "&creator=" + formatStringForMasterServer(creator)
            + "&port=" + Helper::toString(port)
            + "&label=" + formatStringForMasterServer(label)
            + "&descr=" + formatStringForMasterServer(descr);
        request.setBody(body);

        sf::Http http(ConfigManager::getSingleton().getMasterServerUrl());
        sf::Http::Response response = http.sendRequest(request);
        if (response.getStatus() != sf::Http::Response::Ok)
            return false;

        std::string line = response.getBody();
        Helper::trim(line);
        static const std::string prefix = "uuid=";
        if(prefix.size() >= line.size())
            return false;
        if(!std::equal(prefix.begin(), prefix.end(), line.begin()))
            return false;

        uuid = line.substr(prefix.size());
        return true;
    }

    bool updateGame(const std::string& uuid, int32_t status)
    {
        sf::Http::Request request("/update.php", sf::Http::Request::Post);
        std::string body = "uuid=" + uuid
            + "&status=" + Helper::toString(status);
        request.setBody(body);

        sf::Http http(ConfigManager::getSingleton().getMasterServerUrl());
        sf::Http::Response response = http.sendRequest(request);
        if (response.getStatus() != sf::Http::Response::Ok)
            return false;

        return true;
    }

    std::string formatStringForMasterServer(const std::string& str)
    {
        // We replace special meaning chars
        std::string formatted;
        for(auto it = str.begin(); it != str.end(); ++it)
        {
            char c = *it;
            bool isSecialChar = false;
            for(const std::pair<char,char>& specialChar : specialChars)
            {
                if(c == specialChar.first)
                {
                    formatted += replacementChar;
                    formatted += specialChar.second;
                    isSecialChar = true;
                    break;
                }
            }
            if(isSecialChar)
                continue;

            formatted += c;
        }

        return formatted;
    }

    std::string formatStringFromMasterServer(const std::string& str)
    {
        // We replace special meaning chars
        std::string formatted;
        for(auto it = str.begin(); it != str.end(); ++it)
        {
            char c = *it;
            if(c != replacementChar)
            {
                formatted += c;
                continue;
            }

            // If we have a replacementChar, we expect a char telling us
            // what the original char is
            ++it;
            if(it == str.end())
            {
                // It is not normal to end a string with replacementChar!
                OD_LOG_ERR("Error while processing str=" + str + std::string(", c=") + c);
                break;
            }
            c = *it;
            bool isSpecialChar = false;
            for(const std::pair<char,char>& specialChar : specialChars)
            {
                if(c == specialChar.second)
                {
                    formatted += specialChar.first;
                    isSpecialChar = true;
                    break;
                }
            }
            if(!isSpecialChar)
            {
                OD_LOG_ERR("Unexpected special char found c=" + c + std::string(" in str=") + str);
                break;
            }
        }
        return formatted;
    }
}
