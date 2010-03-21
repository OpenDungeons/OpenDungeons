#ifndef PROTECTEDOBJECT_H
#define PROTECTEDOBJECT_H

#include <semaphore.h>

template <class ObjectType>
class ProtectedObject
{
	public:
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

template <class ObjectType>
ProtectedObject<ObjectType>::ProtectedObject()
{
	sem_init(&semaphore, 0, 1);
}

template <class ObjectType>
ProtectedObject<ObjectType>::ProtectedObject(ObjectType init)
{
	object = init;
	sem_init(&semaphore, 0, 1);
}

template <class ObjectType>
void ProtectedObject<ObjectType>::set(ObjectType o)
{
	lock();
	object = o;
	unlock();
}

template <class ObjectType>
ObjectType ProtectedObject<ObjectType>::get()
{
	ObjectType returnValue;

	lock();
	returnValue = object;
	unlock();
	
	return returnValue;
}

template <class ObjectType>
void ProtectedObject<ObjectType>::lock()
{
	sem_wait(&semaphore);
}

template <class ObjectType>
void ProtectedObject<ObjectType>::unlock()
{
	sem_post(&semaphore);
}

template <class ObjectType>
void ProtectedObject<ObjectType>::rawSet(ObjectType o)
{
	object = o;
}

template <class ObjectType>
ObjectType ProtectedObject<ObjectType>::rawGet()
{
	return object;
}

#endif

