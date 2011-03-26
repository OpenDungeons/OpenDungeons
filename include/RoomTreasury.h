#ifndef ROOMTREASURY_H
#define ROOMTREASURY_H

#include "Room.h"
#include <string>

class RoomTreasury: public Room
{
    public:
        RoomTreasury();

        // Functions overriding virtual functions in the Room base class.
        void absorbRoom(Room *r);
        bool doUpkeep(Room *r);
        void addCoveredTile(Tile* t, double nHP = Room::defaultTileHP);
        void removeCoveredTile(Tile* t);
        void clearCoveredTiles();

        // Functions specific to this class.
        int getTotalGold();
        int emptyStorageSpace();
        int depositGold(int gold, Tile *tile);
        int withdrawGold(int gold);

    private:
        enum TreasuryTileFullness
        {
            empty, bag, chest, overfull
        };
        const TreasuryTileFullness& getTreasuryTileFullness(const int& gold) const;
        std::string getMeshNameForTreasuryTileFullness(TreasuryTileFullness fullness);

        void updateMeshesForTile(Tile *t);
        void createMeshesForTile(Tile *t);
        void destroyMeshesForTile(Tile *t);
        void createGoldMeshes();
        void destroyGoldMeshes();

        static const int maxGoldWhichCanBeStoredInABag = 3000;
        static const int maxGoldWhichCanBeStoredInAChest = 5000;

        std::map<Tile*, int> goldInTile;
        std::map<Tile*, TreasuryTileFullness> fullnessOfTile;
};

#endif

