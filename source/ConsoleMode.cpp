#include "ConsoleMode.h"
#include "Gui.h"
#include "Socket.h"
#include "Console.h"
#include "LogManager.h"
#include "ASWrapper.h"
#include "RenderManager.h"
#include <list>
#include <string>
#include "PrefixTreeLL.h"


using std::list;  using std::string;


ConsoleMode::ConsoleMode(ModeContext *modeContext, Console* console):AbstractApplicationMode(modeContext),cn(console),prefix("")
{
  pt = new PrefixTreeLL();
  ll = new list<string>();
  pt->build(("./dictionary.txt"));
  nonTagKeyPressed= true;
}

ConsoleMode::~ConsoleMode(){

  delete pt;
  delete ll;
}



bool ConsoleMode::mouseMoved     (const OIS::MouseEvent &arg){
  CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
  if(arg.state.Z.rel == 0 || !cn->visible)
    {
      return false;
    }

  if(mc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
    {
      cn->scrollHistory(arg.state.Z.rel > 0);
    }
  else
    {
      cn->scrollText(arg.state.Z.rel > 0);
    }

    cn->updateOverlay = true;
    return true;
}

bool ConsoleMode::mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id){
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool ConsoleMode::mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id){
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool ConsoleMode::keyPressed     (const OIS::KeyEvent &arg){
  if (!cn->visible)
    {
      return false;
    }


  if( arg.key == OIS::KC_TAB){
    if(!nonTagKeyPressed){




      // it points to postfix candidate



      if (it == ll->end()){


	cn->prompt = prefix ;
	it = ll->begin();

      }

      else{
	cn->prompt = prefix + *it;
	it++;

      }

    }
    else{
      ll->clear();
      cn->prompt += pt->complete(cn->prompt.c_str(), ll);
      prefix = cn->prompt ;
      it = ll-> begin();



    }



    nonTagKeyPressed= false;

  }
  else{
    nonTagKeyPressed = true;
    switch(arg.key)
      {
      case OIS::KC_GRAVE:
      case OIS::KC_ESCAPE:
      case OIS::KC_F12:
	regressMode();
	cn->setVisible(false);
	ODFrameListener::getSingleton().setTerminalActive(false);
	getKeyboard()->setTextTranslation(OIS::Keyboard::Off);
	break;

      case OIS::KC_RETURN:
	{
	  //only do this for non-empty input
	  if(!cn->prompt.empty())
	    {
	      //print our input and push it to the history
	      cn->print(cn->prompt);
	      cn->history.push_back(cn->prompt);
	      ++cn->curHistPos;

	      //split the input into it's space-separated "words"
	      std::vector<Ogre::String> params = cn->split(cn->prompt, ' ');

	      //TODO: remove this until AS console handler is ready
	      Ogre::String command = params[0];
	      Ogre::String arguments = "";
	      for(size_t i = 1; i< params.size(); ++i)
		{
		  arguments += params[i];
		  if(i < params.size() - 1)
		    {
		      arguments += ' ';
		    }
		}
	      //remove until this point

	      // Force command to lower case
	      //TODO: later do this only for params[0]
	      std::transform(command.begin(), command.end(), command.begin(), ::tolower);
	      std::transform(params[0].begin(), params[0].end(), params[0].begin(), ::tolower);



	      //TODO: remove executePromptCommand after it is fully converted
	      //for now try hardcoded commands, and if none is found try AS
	      if(!cn->executePromptCommand(command, arguments))
		{
		  LogManager::getSingleton().logMessage("Console command: " + command + " - arguments: " + arguments + " - actionscript");
		  ASWrapper::getSingleton().executeConsoleCommand(params);
		}

	      cn->prompt = "";
	    }
	  else
	    {
	      //set history position back to last entry
	      cn->curHistPos = cn->history.size();
	    }
	  break;
	}




      case OIS::KC_BACK:
	cn->prompt = cn->prompt.substr(0, cn->prompt.length() - 1);
	break;

      case OIS::KC_PGUP:
	cn->scrollText(true);
	break;

      case OIS::KC_PGDOWN:
	cn->scrollText(false);
	break;

      case OIS::KC_UP:
	cn->scrollHistory(true);
	break;

      case OIS::KC_DOWN:
	cn->scrollHistory(false);
	break;

      case OIS::KC_F10:
	{
	  LogManager::getSingleton().logMessage("RTSS test----------");
	  RenderManager::getSingleton().rtssTest();
	  break;
	}

      default:
	if (std::string("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?1234567890-=\\!@#$%^&*()_+|;\':\"[]{}").find(
																 arg.text) != std::string::npos)
	  {
	    cn->prompt += arg.text;
	  }
	break;
      }
  }

    cn->updateOverlay = true;
    return true;
}

bool ConsoleMode::keyReleased(const OIS::KeyEvent &arg) {
    return true;
}

void ConsoleMode::handleHotkeys(OIS::KeyCode keycode) {

}

bool ConsoleMode::isInGame(){
  //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
  //TODO - we should use a bool or something, not the sockets for this.
  return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
  //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;


}

void ConsoleMode::giveFocus(){

  mc->mMouse->setEventCallback(this);
  mc->mKeyboard->setEventCallback(this);
}
