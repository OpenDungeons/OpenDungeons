/*!
* \file   MainMenu.h
* \author StefanP.MUC
* \date   24 March 2011
* \brief  Header for MainMenu.cpp
*/

#include <CEGUI.h>

bool mMNewGameButtonPressed(const CEGUI::EventArgs &e);
bool mMLoadButtonPressed(const CEGUI::EventArgs &e);
bool mMOptionsButtonPressed(const CEGUI::EventArgs &e);
bool mMQuitButtonPressed(const CEGUI::EventArgs &e);

void loadMainMenu();
