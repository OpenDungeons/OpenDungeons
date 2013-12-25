/*!
 *  \file   RenderManager.h
 *  \date   26 March 2011
 *  \author oln
 *  \brief  handles the render requests
 */

#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include <deque>

#include <OgreSingleton.h>
#include <semaphore.h>

class Event;



class EventManager: public Ogre::Singleton<EventManager>{


  void processRenderRequests();
  std::deque<Event*> eventQueue;

 public:
  EventManager();
  ~EventManager();



};

#endif /* EVENTMANAGER_H_ */
