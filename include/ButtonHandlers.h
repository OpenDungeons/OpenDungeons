#ifndef BUTTONHANDLERS_H
#define BUTTONHANDLERS_H

#include <CEGUI.h>

#include "Globals.h"
#include "Room.h"
#include "Defines.h"
#include "TextRenderer.h"

bool quitButtonPressed(const CEGUI::EventArgs &e);
bool quartersButtonPressed(const CEGUI::EventArgs &e);
bool treasuryButtonPressed(const CEGUI::EventArgs &e);
bool forgeButtonPressed(const CEGUI::EventArgs &e);
bool dojoButtonPressed(const CEGUI::EventArgs &e);
bool cannonButtonPressed(const CEGUI::EventArgs &e);
bool serverButtonPressed(const CEGUI::EventArgs &e);

#endif

