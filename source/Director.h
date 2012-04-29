#include <Goal.h>
#include <GameMap.h>
#include <Scenario.h>
#include <vector>
#include <string>
#include <set>

class Resource{};
class Scenario{};

class Director {
 private:

  std::vector<Scenario> currentScenarios;
  std::set<Resource> currentResources;
  int unloadResources();
  int loadResources();



 public:


  Director();
  ~Director();
  int playNextScenario();
  int playScenario(int ss);
  int addScenario( const std::string& scenarioFileName );
  int addScenario( const std::string& scenarioFileName, int ss );
  int removeScenario();
  int removeScenario(int ss );
  int clearScenarios();
  

};

