#include <algorithm>

#include "TileCoordinateMap.h"

TileCoordinateMap::TileCoordinateMap(int nRadius)
{
	radius = nRadius;
	precomputeMap(nRadius);
}

void TileCoordinateMap::precomputeMap(int sightRadius)
{
	data.clear();

	//TODO: This loop can be made to list the visible region in a spiral pattern so that all of the tiles appear in the tileQueue already sorted.
	int sightRadiusSquared = sightRadius*sightRadius;
	for(int i = -1*sightRadius; i <= sightRadius; i++)
	{
		for(int j = -1*sightRadius; j <= sightRadius; j++)
		{
			int rSquared = i*i + j*j;
			if(rSquared > sightRadiusSquared)
				continue;

			data.push_back(TileCoordinateData(RadialVector2(0, 0, i, j), rSquared, std::pair<int,int>(i, j)));
		}
	}

	// Sort the tile queue so that if we start at any point in the tile queue and iterate forward from that point, every successive tile will be as far away from, or farther away from the target tile point.
	sort(data.begin(), data.end(), TileCoordinateMap::dataSortComparitor);
}

std::pair<int,int> TileCoordinateMap::getCoordinate(int i)
{
	checkIndex(i);
	return data[i].coord;
}

double TileCoordinateMap::getCentralTheta(int i)
{
	checkIndex(i);
	return data[i].vec.theta;
}

int TileCoordinateMap::getRadiusSquared(int i)
{
	checkIndex(i);
	return data[i].radiusSquared;
}

bool TileCoordinateMap::dataSortComparitor(TileCoordinateData t1, TileCoordinateData t2)
{
	return t1.radiusSquared < t2.radiusSquared;
}

void TileCoordinateMap::checkIndex(int i)
{
	if(i >= data.size())
	{
		int newRadius = radius*2;
		precomputeMap(newRadius);
		radius = newRadius;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

TileCoordinateData::TileCoordinateData(RadialVector2 nvec, int nradiusSquared, std::pair<int,int> ncoord)
{
	vec = nvec;
	radiusSquared = nradiusSquared;
	coord = ncoord;
}

