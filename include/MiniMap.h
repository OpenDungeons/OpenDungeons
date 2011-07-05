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
        ~MiniMap();
        
    private:
        MiniMap(const MiniMap&);

        Ogre::TexturePtr miniMapOgreTexture;
        int width, height;
};

#endif /* MINIMAP_H_ */
