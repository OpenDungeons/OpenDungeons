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

#ifndef MASTERSERVER_H
#define MASTERSERVER_H

#include <cstdint>
#include <string>
#include <vector>

//! \brief class used to retrieve instances of the game currently launched on the master server
class MasterServerGame
{
public:
    MasterServerGame(const std::string& uuid, const std::string& creator, const std::string& ip,
             int32_t port, const std::string& label, const std::string& descr):
        mUuid(uuid),
        mCreator(creator),
        mIp(ip),
        mPort(port),
        mLabel(label),
        mDescr(descr)
    {}

    const std::string mUuid;
    const std::string mCreator;
    const std::string mIp;
    const int32_t mPort;
    const std::string mLabel;
    const std::string mDescr;
};

//! \brief MasterServer namespace contains globals use to communicate with the master server
namespace MasterServer
{
    //! \brief Connects to the master server and fills the given list will currently pending games
    //! Returns true if connection is successful and masterServerGames will be filled
    bool fillMasterServerGames(const std::string& odVersion, std::vector<MasterServerGame>& masterServerGames);

    //! \brief Registers a game to the master server. Returns true if the connexion is successful and
    //! uuid is set to the uuid returned by the server
    bool registerGame(const std::string& odVersion, const std::string& creator, int32_t port, const std::string& label, const std::string& descr, std::string& uuid);

    //! \brief Registers a game to the master server. Returns true if the connexion is successful
    bool updateGame(const std::string& uuid, int32_t status);

    //! \brief Formats the string so that it can be read by the master server
    //! returns the formatted string
    std::string formatStringForMasterServer(const std::string& str);
    std::string formatStringFromMasterServer(const std::string& str);
}

#endif // MASTERSERVER_H
