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

#include <exception>

#include <SFML/Window/Event.hpp>

#include "modes/AbstractApplicationMode.h"
#include "utils/LogManager.h"

#include "modes/SFMLToOISListener.h"

//! \brief Initialise the table that translates between CEGUI (and OIS) and SFML key codes
void initKeyTable(std::array<CEGUI::Key::Scan,sf::Keyboard::KeyCount>& keyMap)
{
    using cgk = CEGUI::Key::Scan;
    using sfk = sf::Keyboard::Key;
    keyMap[static_cast<size_t>(sfk::A)]			= cgk::A;
        keyMap[static_cast<size_t>(sfk::B)]			= cgk::B;
        keyMap[static_cast<size_t>(sfk::C)]			= cgk::C;
        keyMap[static_cast<size_t>(sfk::D)]			= cgk::D;
        keyMap[static_cast<size_t>(sfk::E)]			= cgk::E;
        keyMap[static_cast<size_t>(sfk::F)]			= cgk::F;
        keyMap[static_cast<size_t>(sfk::G)]			= cgk::G;
        keyMap[static_cast<size_t>(sfk::H)]			= cgk::H;
        keyMap[static_cast<size_t>(sfk::I)]			= cgk::I;
        keyMap[static_cast<size_t>(sfk::J)]			= cgk::J;
        keyMap[static_cast<size_t>(sfk::K)]			= cgk::K;
        keyMap[static_cast<size_t>(sfk::L)]			= cgk::L;
        keyMap[static_cast<size_t>(sfk::M)]			= cgk::M;
        keyMap[static_cast<size_t>(sfk::N)]			= cgk::N;
        keyMap[static_cast<size_t>(sfk::O)]			= cgk::O;
        keyMap[static_cast<size_t>(sfk::P)]			= cgk::P;
        keyMap[static_cast<size_t>(sfk::Q)]			= cgk::Q;
        keyMap[static_cast<size_t>(sfk::R)]			= cgk::R;
        keyMap[static_cast<size_t>(sfk::S)]			= cgk::S;
        keyMap[static_cast<size_t>(sfk::T)]			= cgk::T;
        keyMap[static_cast<size_t>(sfk::U)]			= cgk::U;
        keyMap[static_cast<size_t>(sfk::V)]			= cgk::V;
        keyMap[static_cast<size_t>(sfk::W)]			= cgk::W;
        keyMap[static_cast<size_t>(sfk::X)]			= cgk::X;
        keyMap[static_cast<size_t>(sfk::Y)]			= cgk::Y;
        keyMap[static_cast<size_t>(sfk::Z)]			= cgk::Z;
        keyMap[static_cast<size_t>(sfk::Num0)]		= cgk::Zero;
        keyMap[static_cast<size_t>(sfk::Num1)]		= cgk::One;
        keyMap[static_cast<size_t>(sfk::Num2)]		= cgk::Two;
        keyMap[static_cast<size_t>(sfk::Num3)]		= cgk::Three;
        keyMap[static_cast<size_t>(sfk::Num4)]		= cgk::Four;
        keyMap[static_cast<size_t>(sfk::Num5)]		= cgk::Five;
        keyMap[static_cast<size_t>(sfk::Num6)]		= cgk::Six;
        keyMap[static_cast<size_t>(sfk::Num7)]		= cgk::Seven;
        keyMap[static_cast<size_t>(sfk::Num8)]		= cgk::Eight;
        keyMap[static_cast<size_t>(sfk::Num9)]		= cgk::Nine;
        keyMap[static_cast<size_t>(sfk::Escape)]		= cgk::Escape;
        keyMap[static_cast<size_t>(sfk::LControl)]	= cgk::LeftControl;
        keyMap[static_cast<size_t>(sfk::LShift)]		= cgk::LeftShift;
        keyMap[static_cast<size_t>(sfk::LAlt)]		= cgk::LeftAlt;
        keyMap[static_cast<size_t>(sfk::LSystem)]		= cgk::LeftWindows;
        keyMap[static_cast<size_t>(sfk::RControl)]	= cgk::RightControl;
        keyMap[static_cast<size_t>(sfk::RShift)]		= cgk::RightShift;
        keyMap[static_cast<size_t>(sfk::RAlt)]		= cgk::RightAlt;
        keyMap[static_cast<size_t>(sfk::RSystem)]		= cgk::RightWindows;
        keyMap[static_cast<size_t>(sfk::Menu)]		= cgk::AppMenu;
        keyMap[static_cast<size_t>(sfk::LBracket)]	= cgk::LeftBracket;
        keyMap[static_cast<size_t>(sfk::RBracket)]	= cgk::RightBracket;
        keyMap[static_cast<size_t>(sfk::SemiColon)]	= cgk::Semicolon;
        keyMap[static_cast<size_t>(sfk::Comma)]		= cgk::Comma;
        keyMap[static_cast<size_t>(sfk::Period)]		= cgk::Period;
        keyMap[static_cast<size_t>(sfk::Quote)]		= cgk::Apostrophe;
        keyMap[static_cast<size_t>(sfk::Slash)]		= cgk::Slash;
        keyMap[static_cast<size_t>(sfk::BackSlash)]	= cgk::Backslash;
        keyMap[static_cast<size_t>(sfk::Tilde)]		= cgk::Grave;
        keyMap[static_cast<size_t>(sfk::Equal)]		= cgk::Equals;
        keyMap[static_cast<size_t>(sfk::Dash)]		= cgk::Minus;
        keyMap[static_cast<size_t>(sfk::Space)]		= cgk::Space;
        keyMap[static_cast<size_t>(sfk::Return)]		= cgk::Return;
        keyMap[static_cast<size_t>(sfk::BackSpace)]	= cgk::Backspace;
        keyMap[static_cast<size_t>(sfk::Tab)]			= cgk::Tab;
        keyMap[static_cast<size_t>(sfk::PageUp)]		= cgk::PageUp;
        keyMap[static_cast<size_t>(sfk::PageDown)]	= cgk::PageDown;
        keyMap[static_cast<size_t>(sfk::End)]			= cgk::End;
        keyMap[static_cast<size_t>(sfk::Home)]		= cgk::Home;
        keyMap[static_cast<size_t>(sfk::Insert)]		= cgk::Insert;
        keyMap[static_cast<size_t>(sfk::Delete)]		= cgk::Delete;
        keyMap[static_cast<size_t>(sfk::Add)]			= cgk::Add;
        keyMap[static_cast<size_t>(sfk::Subtract)]	= cgk::Subtract;
        keyMap[static_cast<size_t>(sfk::Multiply)]	= cgk::Multiply;
        keyMap[static_cast<size_t>(sfk::Divide)]		= cgk::Divide;
        keyMap[static_cast<size_t>(sfk::Left)]		= cgk::ArrowLeft;
        keyMap[static_cast<size_t>(sfk::Right)]		= cgk::ArrowRight;
        keyMap[static_cast<size_t>(sfk::Up)]			= cgk::ArrowUp;
        keyMap[static_cast<size_t>(sfk::Down)]		= cgk::ArrowDown;
        keyMap[static_cast<size_t>(sfk::Numpad0)] 	= cgk::Numpad0;
        keyMap[static_cast<size_t>(sfk::Numpad1)] 	= cgk::Numpad1;
        keyMap[static_cast<size_t>(sfk::Numpad2)] 	= cgk::Numpad2;
        keyMap[static_cast<size_t>(sfk::Numpad3)] 	= cgk::Numpad3;
        keyMap[static_cast<size_t>(sfk::Numpad4)] 	= cgk::Numpad4;
        keyMap[static_cast<size_t>(sfk::Numpad5)] 	= cgk::Numpad5;
        keyMap[static_cast<size_t>(sfk::Numpad6)] 	= cgk::Numpad6;
        keyMap[static_cast<size_t>(sfk::Numpad7)] 	= cgk::Numpad7;
        keyMap[static_cast<size_t>(sfk::Numpad8)] 	= cgk::Numpad8;
        keyMap[static_cast<size_t>(sfk::Numpad9)] 	= cgk::Numpad9;
        keyMap[static_cast<size_t>(sfk::F1)]			= cgk::F1;
        keyMap[static_cast<size_t>(sfk::F2)]			= cgk::F2;
        keyMap[static_cast<size_t>(sfk::F3)]			= cgk::F3;
        keyMap[static_cast<size_t>(sfk::F4)]			= cgk::F4;
        keyMap[static_cast<size_t>(sfk::F5)]			= cgk::F5;
        keyMap[static_cast<size_t>(sfk::F6)]			= cgk::F6;
        keyMap[static_cast<size_t>(sfk::F7)]			= cgk::F7;
        keyMap[static_cast<size_t>(sfk::F8)]			= cgk::F8;
        keyMap[static_cast<size_t>(sfk::F9)]			= cgk::F9;
        keyMap[static_cast<size_t>(sfk::F10)]			= cgk::F10;
        keyMap[static_cast<size_t>(sfk::F11)]			= cgk::F11;
        keyMap[static_cast<size_t>(sfk::F12)]			= cgk::F12;
        keyMap[static_cast<size_t>(sfk::F13)]			= cgk::F13;
        keyMap[static_cast<size_t>(sfk::F14)]			= cgk::F14;
        keyMap[static_cast<size_t>(sfk::F15)]			= cgk::F15;
        keyMap[static_cast<size_t>(sfk::Pause)]		= cgk::Pause;
/*
        keyMap[sf::Mouse::Left)]		= CEGUI::LeftButton;
        keyMap[sf::Mouse::Right)]		= CEGUI::RightButton;
        keyMap[sf::Mouse::Middle)]		= CEGUI::MiddleButton;
        keyMap[sf::Mouse::XButton1)]	= CEGUI::X1Button;
        keyMap[sf::Mouse::XButton2)]	= CEGUI::X2Button;*/
}

SFMLToOISListener::SFMLToOISListener(AbstractApplicationMode* receiver, int windowWidth, int windowHeigth)
    : mReceiver(receiver)
{
    mMouseState.width = windowWidth;
    mMouseState.height = windowHeigth;
    initKeyTable(mKeyMap);
}

void SFMLToOISListener::setReceiver(AbstractApplicationMode* receiver)
{
    mReceiver = receiver;
}

bool SFMLToOISListener::handleEvent(const sf::Event& event)
{
    if(mReceiver == nullptr)
    {
        return true;
    }
    using et = sf::Event::EventType;
    switch(event.type)
    {
        case et::KeyPressed:
            if (static_cast<size_t>(event.key.code) >= mKeyMap.size())
            {
                return true;
            }
            return mReceiver->keyPressed(OIS::KeyEvent(nullptr, static_cast<OIS::KeyCode>(mKeyMap.at(event.key.code)), 0));
        case et::KeyReleased:
            if (static_cast<size_t>(event.key.code) >= mKeyMap.size())
            {
                return true;
            }
            return mReceiver->keyReleased(OIS::KeyEvent(nullptr, static_cast<OIS::KeyCode>(mKeyMap.at(event.key.code)), 0));
        case et::TextEntered:
            return mReceiver->keyPressed(OIS::KeyEvent(nullptr, OIS::KeyCode::KC_UNASSIGNED, event.text.unicode));
        case et::MouseMoved:
            mMouseState.X.abs = event.mouseMove.x;
            mMouseState.Y.abs = event.mouseMove.y;
            // Reset the mouse wheel position here as otherwise mouseMoved will think the scrollwheel has changed each
            // time the mouse moves.
            mMouseState.Z.rel = 0;
            return mReceiver->mouseMoved(OIS::MouseEvent(nullptr, mMouseState));
        case et::MouseButtonPressed:
            mMouseState.buttons |= 1 << static_cast<int>(event.mouseButton.button);
            return mReceiver->mousePressed(OIS::MouseEvent(nullptr, mMouseState), static_cast<OIS::MouseButtonID>(event.mouseButton.button));
        case et::MouseButtonReleased:
            mMouseState.buttons &= ~(1 << static_cast<int>(event.mouseButton.button));
            return mReceiver->mouseReleased(OIS::MouseEvent(nullptr, mMouseState), static_cast<OIS::MouseButtonID>(event.mouseButton.button));
#if SFML_VERSION_MINOR > 2
        case et::MouseWheelScrolled:
            mMouseState.Z.rel = static_cast<int>(event.mouseWheelScroll.delta);
            mMouseState.X.abs = event.mouseWheelScroll.x;
            mMouseState.Y.abs = event.mouseWheelScroll.y;
            return mReceiver->mouseMoved(OIS::MouseEvent(nullptr, mMouseState));
#else /* SFML_VERSION_MINOR > 2 */
        case et::MouseWheelMoved:
            mMouseState.Z.rel = event.mouseWheel.delta;
            mMouseState.X.abs = event.mouseWheel.x;
            mMouseState.Y.abs = event.mouseWheel.y;
            return mReceiver->mouseMoved(OIS::MouseEvent(nullptr, mMouseState));
#endif /* SFML_VERSION_MINOR > 2 */
        case et::Resized:
            mMouseState.width = event.size.width;
            mMouseState.height = event.size.height;
            break;
        default:
            OD_LOG_DBG("Unhandled event!");
    }
    return true;
}
