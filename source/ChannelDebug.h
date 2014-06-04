#include <iostream>
#include <fstream>
#include <array>

using std::ofstream;

    enum class ChannelColors{
        white,
	gray,
        silver,
        black,
	maroon,
	red,
	purple,
	fuchsia,
	green,
	lime,
	olive,
	yellow,
	navy,
	blue,
	teal,
	aqua,
	orange
	};

class ChannelDebug{
    std::array<bool, 17> fileLogOpen;
    std::array<bool, 17> windowLogOpen;
    std::array<ofstream, 17> fileStreams;   


public:    

 


    void redirectToFile(ChannelColors cc){ fileLogOpen[static_cast<int>(cc)] = true; };
    void stopRedirectToFile(ChannelColors cc){ fileLogOpen[static_cast<int>(cc)] = false; };


    ChannelDebug();
    ~ChannelDebug();
    template< typename F >
    void log(ChannelColors destinationChannel, F message_creator){
	if (fileLogOpen[static_cast<int>(destinationChannel)]){
	    fileStreams[static_cast<int>(destinationChannel)] << message_creator() << std::endl;
	}
	else if(windowLogOpen[static_cast<int>(destinationChannel)]){}
    }
};
