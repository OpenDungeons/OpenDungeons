#ifndef ATTACKABLEOBJECT_H
#define ATTACKABLEOBJECT_H

class AttackableObject
{
	public:
		enum AttackableObjectType {creature, room, trap, door};

		virtual vector<Tile*> getCoveredTiles() = 0;
		virtual double getHP(Tile *tile) = 0;
		virtual double getDefense() = 0;
		virtual void takeDamage(double damage, Tile *tileTakingDamage) = 0;
		virtual void recieveExp(double experience) = 0;
		virtual bool isMobile() = 0;
		virtual int getLevel() = 0;
		virtual int getColor() = 0;
		virtual string getName() = 0;
		virtual AttackableObjectType getAttackableObjectType() = 0;
};

#endif

