#include "Director.h"

template<> Director* Ogre::Singleton<Director>::msSingleton = 0;

Director::Director() {}
Director::~Director() {}
int Director::playNextScenario() { return 0; }
int Director::playScenario(int ss) { return 0; }
int Director::addScenario(const std::string& scenarioFileName) { return 0; }
int Director::addScenario(const std::string& scenarioFileName, int ss) { return 0; }

int Director::removeScenario() { return 0; }
int Director::removeScenario(int ss) { return 0; }
int Director::clearScenarios() { return 0; }
