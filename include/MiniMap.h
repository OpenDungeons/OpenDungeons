/*!
 * \file   MiniMap.h
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  header for the minimap
 */

#ifndef MINIMAP_H_
#define MINIMAP_H_

#include <Ogre.h>

class MiniMap : public Ogre::Singleton<MiniMap>
{
    public:
        MiniMap();
        static MiniMap& getSingleton();
        static MiniMap* getSingletonPtr();

    private:
        MiniMap(const MiniMap&);
        ~MiniMap();

        Ogre::Camera* miniMapCamera;
        Ogre::TexturePtr miniMapOgreTexture;
        Ogre::RenderTexture* miniMapRenderer;
};

#endif /* MINIMAP_H_ */
