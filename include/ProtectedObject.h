#ifndef PROTECTEDOBJECT_H
#define PROTECTEDOBJECT_H

#include <semaphore.h>

template <class ObjectType>
class ProtectedObject
{
	public:
		ProtectedObject();
		ProtectedObject(ObjectType init);

		void setObject(ObjectType o);
		ObjectType getObject();
		void lock();
		void unlock();
		void rawSetObject(ObjectType o);
		ObjectType rawGetObject();

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
void ProtectedObject<ObjectType>::setObject(ObjectType o)
{
	lock();
	object = o;
	unlock();
}

template <class ObjectType>
ObjectType ProtectedObject<ObjectType>::getObject()
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
void ProtectedObject<ObjectType>::rawSetObject(ObjectType o)
{
	object = o;
}

template <class ObjectType>
ObjectType ProtectedObject<ObjectType>::rawGetObject()
{
	return object;
}

#endif

