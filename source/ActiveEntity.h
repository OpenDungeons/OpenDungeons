#ifndef ACTIVEENTITY_H
#define ACTIVEENTITY_H

class ActiveEntity
{
    public:
        virtual ~ActiveEntity() {}

        virtual bool doUpkeep();
};

#endif

