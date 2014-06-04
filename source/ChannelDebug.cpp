#include "ChannelDebug.h"

ChannelDebug::ChannelDebug(){
    fileStreams[static_cast<int>(ChannelColors::white)].open("white.log");
    fileStreams[static_cast<int>(ChannelColors::gray)].open("gray.log");
    fileStreams[static_cast<int>(ChannelColors::silver)].open("silver.log");
    fileStreams[static_cast<int>(ChannelColors::black)].open("black.log");
    fileStreams[static_cast<int>(ChannelColors::maroon)].open("maroon.log");
    fileStreams[static_cast<int>(ChannelColors::red)].open("red.log");
    fileStreams[static_cast<int>(ChannelColors::purple)].open("purple.log");
    fileStreams[static_cast<int>(ChannelColors::fuchsia)].open("fuchsia.log");
    fileStreams[static_cast<int>(ChannelColors::green)].open("green.log");
    fileStreams[static_cast<int>(ChannelColors::lime)].open("lime.log");
    fileStreams[static_cast<int>(ChannelColors::olive)].open("olive.log");
    fileStreams[static_cast<int>(ChannelColors::yellow)].open("yellow.log");
    fileStreams[static_cast<int>(ChannelColors::navy)].open("navy.log");
    fileStreams[static_cast<int>(ChannelColors::blue)].open("blue.log");
    fileStreams[static_cast<int>(ChannelColors::teal)].open("teal.log");
    fileStreams[static_cast<int>(ChannelColors::aqua)].open("aqua.log");
    fileStreams[static_cast<int>(ChannelColors::orange)].open("orange.log");
}

ChannelDebug::~ChannelDebug(){
}
