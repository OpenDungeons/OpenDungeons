/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "game/Seat.h"

#include "game/Research.h"
#include "game/ResearchManager.h"
#include "entities/CreatureDefinition.h"
#include "entities/ResearchEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "goals/Goal.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "spawnconditions/SpawnCondition.h"
#include "spells/Spell.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"
