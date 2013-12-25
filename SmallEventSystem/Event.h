#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <list>

class Event{

protected:
    std::list<std::string> actionList;
public:


    Event();
    Event(const std::list<std::string> &);
    ~Event();
    std::list<std::string> getActionList();
    bool setActionList(const std::list<std::string> &);
    };

#endif //EVENT_H
