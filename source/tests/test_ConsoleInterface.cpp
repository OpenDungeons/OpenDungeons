#include "modes/ConsoleInterface.h"
#include <string>
#include "mocks/TestModeManager.h"

#define BOOST_TEST_MODULE ConsoleInterface
#include "BoostTestTargetConfig.h"

template<typename StringT>
void appendText(StringT text)
{
    BOOST_TEST_MESSAGE("printed: " << text);
}

using MockConsole = ConsoleInterface;

Command::Result testCommand(const Command::ArgumentList_t& args, MockConsole& console, AbstractModeManager& mm)
{
    console.print("multiple aliases function");
    for(auto&& arg : args)
    {
        console.print("Argument: " + arg + "\n");
    }
    int x = 0;
    ++x;
    try {
        x = std::stoi(args[1]);
    }
    catch (const std::invalid_argument& e)
    {
        return Command::Result::INVALID_ARGUMENT;
    }
    catch (const std::out_of_range)
    {
        return Command::Result::INVALID_ARGUMENT;
    }

    return Command::Result::SUCCESS;
}

BOOST_AUTO_TEST_CASE(test_Command)
{
    auto t = TestModeManager::ModeType::GAME;
    Command c(testCommand,"descr",{t});
    BOOST_CHECK(c.isAllowedInMode(t));
}

BOOST_AUTO_TEST_CASE(test_ConsoleInterface)
{

    TestModeManager modeManager(TestModeManager::ModeType::GAME);
    TestModeManager::ModeType mt = TestModeManager::ModeType::GAME;
    using ModeManager = TestModeManager;
    MockConsole interface(appendText<std::string>);
    {
        BOOST_CHECK(
            interface.addCommand("test1", "test1 description",
                                 [](const Command::ArgumentList_t& args, MockConsole& console, AbstractModeManager& mm)
                                    {
                                        for(auto&& arg : args)
                                        {
                                            console.print("Arg: " + arg);
                                        }
                                        return Command::Result::SUCCESS;
                                    },
                                 {ModeManager::ModeType::ALL}

            )
        );
    }
    BOOST_CHECK(interface.addCommand("aliasedcommand",
                         "aliased command desciption",
                         testCommand,
                         {ModeManager::ModeType::GAME},
                         {"aliasedcmd",
                         "alsdcmd"}));

    BOOST_CHECK(interface.addCommand(" commnand with spaces and \n \t other characters",
                                     "description",
                                     testCommand,
                                     {ModeManager::ModeType::ALL}) == false);

    unsigned int count = 0;
    //Check that scrolling up or down without a history does not do anything
    BOOST_CHECK(interface.scrollCommandHistoryPositionDown() == false);
    BOOST_CHECK(interface.scrollCommandHistoryPositionUp("text") == false);
    //try executing commands
    BOOST_CHECK(interface.tryExecuteCommand("help",mt, modeManager) == Command::Result::SUCCESS); ++count;
    interface.tryExecuteCommand("test0",mt, modeManager); ++count;
    interface.tryExecuteCommand("test1",mt, modeManager); ++count;
    interface.tryExecuteCommand("test1 123 abc",mt, modeManager); ++count;
    BOOST_CHECK(interface.tryExecuteCommand("aliasedcommand", TestModeManager::ModeType::EDITOR, modeManager) == Command::Result::WRONG_MODE); ++count;
    BOOST_CHECK(interface.tryExecuteCommand("aliasedcommand 1",mt, modeManager) == Command::Result::SUCCESS); ++count;
    BOOST_CHECK(interface.tryExecuteCommand("aliasedcmd argument1",mt, modeManager) == Command::Result::INVALID_ARGUMENT); ++count;
    BOOST_CHECK(interface.tryExecuteCommand("aliasedcmd 184467440737095516100",mt, modeManager) == Command::Result::INVALID_ARGUMENT); ++count;
    interface.tryExecuteCommand("alsdcmd argument1",mt, modeManager); ++count;
    //Test command completion
    BOOST_CHECK(interface.tryCompleteCommand("alia") == false);
    auto result = interface.tryCompleteCommand("aliasedco");
    BOOST_CHECK(result);
    BOOST_CHECK((*result).compare("aliasedcommand") == 0);
    //More scrolling tests
    BOOST_CHECK(interface.scrollCommandHistoryPositionDown() == false);
    BOOST_CHECK((*interface.scrollCommandHistoryPositionUp("commandPrompt")).compare("alsdcmd argument1") == 0);
    BOOST_CHECK((*interface.scrollCommandHistoryPositionDown()).compare("not commandPrompt") != 0);
    BOOST_CHECK(interface.scrollCommandHistoryPositionDown() == false);

    //Check command history, we should be at the bottom now:
    BOOST_CHECK(interface.scrollCommandHistoryPositionDown() == false);
    unsigned int test_count = 0;
    while(true)
    {
        auto res = interface.scrollCommandHistoryPositionUp("commandPrompt");
        if(!res)
        {
            break;
        }
        ++test_count;
        BOOST_TEST_MESSAGE("command" << *res << " count: " << test_count);
    }
    BOOST_CHECK(test_count == count);

}
