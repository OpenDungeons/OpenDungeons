#include "TrapCannon.h"

TrapCannon::TrapCannon()
	: Trap()
{
}

double TrapCannon::getHP(Tile *tile)
{
	return 0;
}

double TrapCannon::getDefense()
{
	return 0;
}

void TrapCannon::takeDamage(double damage, Tile *tileTakingDamage)
{
}

void TrapCannon::recieveExp(double experience)
{
}

bool TrapCannon::isMobile()
{
	return false;
}

int TrapCannon::getLevel()
{
	return 1;
}

int TrapCannon::getColor()
{
	if(controllingSeat != NULL)
		return controllingSeat->color;
	else
		return 0;
}

string TrapCannon::getName()
{
	return name;
}

AttackableObject::AttackableObjectType TrapCannon::getAttackableObjectType()
{
	return trap;
}

