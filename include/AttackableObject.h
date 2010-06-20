#ifndef ATTACKABLEOBJECT_H
#define ATTACKABLEOBJECT_H

class AttackableObject
{
	public:
		virtual vector<Tile*> getCoveredTiles() = 0;
		virtual double getHP() = 0;
		virtual void takeDamage(double damage) = 0;
		virtual void recieveExp(double experience) = 0;
		virtual bool isMobile() = 0;
		virtual int getLevel() = 0;
		virtual int getColor() = 0;
};

#endif

