#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <string>
#include <sstream>
#include <fstream>
using namespace std;

#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"

#include <CEGUI/CEGUI.h>
#include <OIS/OIS.h>
#include <OgreCEGUIRenderer.h>

class MapEditor : public ExampleApplication
{
	public:
		MapEditor();
		~MapEditor();
		void createCamera(void);
		void createScene(void);
		void createFrameListener(void);

	protected:
		void chooseSceneManager(void);
		CEGUI::System *mSystem;
		CEGUI::OgreCEGUIRenderer *mRenderer;
};

#endif

