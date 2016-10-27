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

#ifndef SFMLTOOISLISTENER_H
#define SFMLTOOISLISTENER_H

#include <array>
#include <SFML/Window/Keyboard.hpp>
#include <CEGUI/InputEvent.h>
#include <OIS/OISMouse.h>

namespace sf {
    class Event;
}

class AbstractApplicationMode;

//! \brief A class that receives SFML events and translates them to OIS events which are then distributed to
//! the supplied AbstractApplicationMode instance
class SFMLToOISListener
{
public:
    SFMLToOISListener(AbstractApplicationMode& receiver, int windowWidth, int windowHeigth);
    void setReceiver(AbstractApplicationMode& receiver);
    //! \brief Convert an SFML event into the appropriate action and send it to the supplied listener.
    bool handleEvent(const sf::Event& event);
private:
    AbstractApplicationMode* mReceiver;
    // TODO: Make this const somewhere
    //! An array that maps between CEGUI and SFML key codes
    std::array<CEGUI::Key::Scan, sf::Keyboard::KeyCount> mKeyMap;
    //! A manually updated mouseState for the mouse events
    OIS::MouseState mMouseState;
};

#endif // SFMLTOOISLISTENER_H
