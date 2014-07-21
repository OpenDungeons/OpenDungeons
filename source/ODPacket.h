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

#ifndef ODPACKET_H
#define ODPACKET_H

#include <SFML/Network.hpp>

#include <string>
#include <cstdint>

/*! \brief This class is an utility class to transfer data through ODSocketClient.
 * It should also override operators << and >> for each standard types.
 * ODPacket should preserve integrity. That means that if an ODSocketClient
 * sends an ODPacket, the server should receive exactly 1 similar ODPacket (same data,
 * nothing less, nothing more)
 */
class ODPacket
{
    friend class ODSocketClient;

    public:
        ODPacket();
        ~ODPacket();

        /*! \brief Export data operators.
         * The behaviour is the same as standard C++ streams
         */
        ODPacket& operator >>(bool&         data);
        ODPacket& operator >>(int8_t&       data);
        ODPacket& operator >>(uint8_t&      data);
        ODPacket& operator >>(int16_t&      data);
        ODPacket& operator >>(uint16_t&     data);
        ODPacket& operator >>(int32_t&      data);
        ODPacket& operator >>(uint32_t&     data);
        ODPacket& operator >>(int64_t&      data);
        ODPacket& operator >>(uint64_t&     data);
        ODPacket& operator >>(float&        data);
        ODPacket& operator >>(double&       data);
        ODPacket& operator >>(char*         data);
        ODPacket& operator >>(std::string&  data);
        ODPacket& operator >>(wchar_t*      data);
        ODPacket& operator >>(std::wstring& data);

        /*! \brief Import data operators
         * The behaviour is the same as standard C++ streams
         */
        ODPacket& operator <<(bool                  data);
        ODPacket& operator <<(int8_t                data);
        ODPacket& operator <<(uint8_t               data);
        ODPacket& operator <<(int16_t               data);
        ODPacket& operator <<(uint16_t              data);
        ODPacket& operator <<(int32_t               data);
        ODPacket& operator <<(uint32_t              data);
        ODPacket& operator <<(int64_t               data);
        ODPacket& operator <<(uint64_t              data);
        ODPacket& operator <<(float                 data);
        ODPacket& operator <<(double                data);
        ODPacket& operator <<(const char*           data);
        ODPacket& operator <<(const std::string&    data);
        ODPacket& operator <<(const wchar_t*        data);
        ODPacket& operator <<(const std::wstring&   data);

        /*! \brief Return true if there were no error exporting data (operator >>).
         * This behaviour is the same as standard C++ streams :
         * If we try to export data while the packet is empty or from incompatible types,
         * the data will be invalid and the function will return false
         */
        operator bool() const;

        /*! \brief Clears the packet. After calling Clear, the packet should
         * be empty.
         */
        void clear();

    private:
        sf::Packet packet;

};

#endif // ODPACKET_H

