#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <string>
#include <sstream>
#include <fstream>

#include <OIS.h>

#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"
#include "MusicPlayer.h"
#include "SoundEffectsHelper.h"

class MapEditor: public ExampleApplication
{
    public:
        MapEditor();
        virtual ~MapEditor();
        void createCamera(void);
        void createScene(void);
        void createFrameListener(void);

    protected:
        void chooseSceneManager(void);
};

#endif

