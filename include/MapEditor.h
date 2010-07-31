#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <string>
#include <sstream>
#include <fstream>

#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"

#include <CEGUI/CEGUI.h>
#include <OIS/OIS.h>
//#include <CEGUIRenderer.h>   // use this line if using a CEGUI version before 0.7
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>   // use this line if using a CEGUI version after 0.7

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
		CEGUI::Renderer *mRenderer;
};

#endif

