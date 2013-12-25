#include "Event.h"
using std::string;
using std::list;

Event::Event(){}
Event::Event(const list<string> &ll):actionList(ll){}
Event::~Event(){}
list<string> Event::getActionList(){}
bool  Event::setActionList(const list<string> &ll){ actionList = ll;}
