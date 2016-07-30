/*!
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

#include "gamemap/MiniMap.h"

#include "gamemap/MiniMapCamera.h"
#include "gamemap/MiniMapDrawn.h"
#include "gamemap/MiniMapDrawnFull.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

namespace MiniMapTypes
{
static const std::string MINIMAP_CAMERA = "MiniMapCamera";
static const std::string MINIMAP_DRAWN = "MiniMapDrawn";
static const std::string MINIMAP_DRAWN_FULL = "MiniMapDrawnFull";

static std::vector<std::string> buildMiniMapTypes()
{
    std::vector<std::string> mapTypes;
    mapTypes.push_back(MINIMAP_CAMERA);
    mapTypes.push_back(MINIMAP_DRAWN);
    mapTypes.push_back(MINIMAP_DRAWN_FULL);
    return mapTypes;
}

};

const std::string& MiniMap::DEFAULT_MINIMAP = MiniMapTypes::MINIMAP_CAMERA;

MiniMap* MiniMap::createMiniMap(CEGUI::Window* miniMapWindow)
{
    ConfigManager& config = ConfigManager::getSingleton();
    std::string minimapType = config.getGameValue(Config::MINIMAP_TYPE, DEFAULT_MINIMAP, false);
    if(minimapType == MiniMapTypes::MINIMAP_CAMERA)
        return new MiniMapCamera(miniMapWindow);
    if(minimapType == MiniMapTypes::MINIMAP_DRAWN)
        return new MiniMapDrawn(miniMapWindow);
    if(minimapType == MiniMapTypes::MINIMAP_DRAWN_FULL)
        return new MiniMapDrawnFull(miniMapWindow);

    OD_LOG_ERR("Couldn't find requested minimap=" + minimapType);
    // Per default, we return the default minimap
    return new MiniMapCamera(miniMapWindow);
}

const std::vector<std::string>& MiniMap::getMiniMapTypes()
{
    static std::vector<std::string> minimapTypes = MiniMapTypes::buildMiniMapTypes();
    return minimapTypes;
}
