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

#include <OgreVector3.h>
#include <SFML/Network.hpp>

#include <string>
#include <cstdint>

/*! \brief This class is an utility class to transfer data through ODSocketClient.
 * It should also override operators << and >> for each standard types.
 * ODPacket should preserve integrity. That means that if an ODSocketClient
 * sends an ODPacket, the server should receive exactly 1 similar ODPacket (same data,
 * nothing less, nothing more)
 * IMPORTANT : ODPacket can handle different types of data the same way. For that, when we receive
 * data, we have to know its type (when an int is sent, we know an int will be received). The typical
 * use of ODPacket is :
 * Emission : int myInt;
 *            ODPacket packet;
 *            packet << myInt;
 *            send(packet);
 * Reception : ODPacket packet;
 *             receive(packet);
 *             int myInt;
 *             packet >> myInt;
 * As we can see in this example, we have to know the data type within the packet before reading it.
 * But depending on architecture, default datatype can be different between the server and the client.
 * For this reason, the data returned by a function should never be used directly as ODPacket data. Instead,
 * a temporary variable should be used and sent to ODPacket (sized variables such as int32_t, int8_t, ...
 * should be as used as possible for the same reason).
 * For example : You should NOT do :
 * packet << getName();
 * You should do :
 * std::string name = getName();
 * packet << name;
 * This way, if the function signature changes or if the data type is different (int vs long for example) on
 * different platforms, there will be an error when compiling instead of a probable buggy behaviour at runtime.
 * When dealing with class/structure, the best is to use directly the variables themselves, no getter/setter.
 * For exemple, You should NOT do :
 * Emission : packet << creature->getHp();
 * Reception : double hp;
 *             packet >> hp;
 *             creature->setHp(hp);
 * You should do :
 * Emission : packet << creature->mHp;
 * Reception : packet >> creature->mHp;
 * This way, if mHp changes (from float to double for example), it will still work.
 */
class ODPacket
{
    friend class ODSocketClient;

    public:
        ODPacket()
        {}
        ~ODPacket()
        {}

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
        ODPacket& operator >>(Ogre::Vector3& data);

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
        ODPacket& operator <<(const Ogre::Vector3&   data);

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

        /*! \brief Writes the packet content to the given ofstream.
         */
        void writePacket(int32_t timestamp, std::ofstream& os);

        /*! \brief Reads the packet content from the given ifstream.
         *         Returns the timestamp at which the packet has been sent.
         *         If EOF has been reached, returns -1
         */
        int32_t readPacket(std::ifstream& is);

        /*! \brief Template function to put arguments in a packet, used for in-place construction.
         */
        template<typename FirstArg, typename ...Args>
        static void putInPacket(ODPacket& packet, const FirstArg& arg, const Args&... args)
        {
            packet << arg;
            putInPacket(packet, args...);
        }

        template<typename Arg>
        static void putInPacket(ODPacket& packet, const Arg& arg)
        {
            packet << arg;
        }

    private:
        sf::Packet mPacket;

};

#endif // ODPACKET_H

