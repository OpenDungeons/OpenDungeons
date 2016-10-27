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

#ifndef INPUTBRIDGE_H
#define INPUTBRIDGE_H


//#include <OIS/OISKeyboard.h>
#include <OIS/OISMouse.h>

#include <SFML/Window/Event.hpp>

namespace OIS {
    class MouseEvent;
    class KeyEvent;
}

using MouseMoveEvent = sf::Event::MouseMoveEvent;
using MouseWheelEvent = sf::Event::MouseWheelScrollEvent;


inline MouseMoveEvent toSFMLMouseMove(const OIS::MouseEvent& evt)
{
    return MouseMoveEvent{evt.state.X.abs, evt.state.Y.abs};
}

inline MouseWheelEvent toSFMLMouseWheel(const OIS::MouseEvent& evt)
{
    // TODO: Not sure if the sfml wheel event would correspond to relative or absolute state
    return MouseWheelEvent{sf::Mouse::VerticalWheel, static_cast<float>(evt.state.Z.rel), evt.state.X.abs, evt.state.Y.abs};
}

#endif // INPUTBRIDGE_H
