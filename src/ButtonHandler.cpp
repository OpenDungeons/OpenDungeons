#include "Globals.h"
#include "ButtonHandler.h"
#include "ExampleFrameListener.h"

bool quitButtonPressed(const CEGUI::EventArgs &e)
{
	cout << "\n\nQuit button pressed.\n\n";
	//app.mFrameListener->quit(e);

	return true;
}

