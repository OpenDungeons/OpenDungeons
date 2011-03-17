#ifndef PROTECTEDOBJECT_H
#define PROTECTEDOBJECT_H

#include <semaphore.h>

template<class ObjectType>
class ProtectedObject
{
    public:
        void initialize();
        ProtectedObject();
        ProtectedObject(ObjectType init);

        void set(ObjectType o);
        ObjectType get();
        void lock();
        void unlock();
        void rawSet(ObjectType o);
        ObjectType rawGet();

    private:
        ObjectType object;
        sem_t semaphore;
};

template<class ObjectType>
void ProtectedObject<ObjectType>::initialize()
{
    // Valgrind's helgrind tool complains during the lock function about a wait happening before
    // a post, this code (rather than just initializing the semaphore to 1) should prevent that.
    sem_init(&semaphore, 0, 0);
    sem_post(&semaphore);
}

template<class ObjectType>
ProtectedObject<ObjectType>::ProtectedObject()
{
    initialize();
}

template<class ObjectType>
ProtectedObject<ObjectType>::ProtectedObject(ObjectType init)
{
    initialize();

    sem_wait(&semaphore);
    object = init;
    sem_post(&semaphore);
}

template<class ObjectType>
void ProtectedObject<ObjectType>::set(ObjectType o)
{
    lock();
    object = o;
    unlock();
}

template<class ObjectType>
ObjectType ProtectedObject<ObjectType>::get()
{
    ObjectType returnValue;

    lock();
    returnValue = object;
    unlock();

    return returnValue;
}

template<class ObjectType>
void ProtectedObject<ObjectType>::lock()
{
    sem_wait(&semaphore);
}

template<class ObjectType>
void ProtectedObject<ObjectType>::unlock()
{
    sem_post(&semaphore);
}

template<class ObjectType>
void ProtectedObject<ObjectType>::rawSet(ObjectType o)
{
    object = o;
}

template<class ObjectType>
ObjectType ProtectedObject<ObjectType>::rawGet()
{
    return object;
}

#endif

