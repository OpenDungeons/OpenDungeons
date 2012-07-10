#include "AIFactory.h"

std::map<std::string, AIFactory::CreateAIFunc>* AIFactory::typeMap = 0;
