#include <Goal.h>
#include <GameMap.h>
#include <Scenario.h>
#include <vector>
#include <string>
#include <set>


using namespace std;

class Resource{};
class Scenario{};

class Director {
 private:

  vector<Scenario> currentScenarios;
  set<Resource> currentResources;
  int unloadResources();
  int loadResources();



 public:


  Director();
  ~Director();
  int playNextScenario();
  int playScenario(int ss);
  int addScenario( string scenarioFileName );
  int addScenario( string scenarioFileName, int ss );
  int removeScenario();
  int removeScenario(int ss );
  int clearScenarios();
  

};

