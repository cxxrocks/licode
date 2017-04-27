//
// Created by haoy on 2017/4/19.
//

#include "media/NullOutput.h"

namespace erizo {

void NullOutput::close()
{}

int NullOutput::deliverAudioData_(std::shared_ptr<dataPacket> data_packet)
{return 0;} 

int NullOutput::deliverVideoData_(std::shared_ptr<dataPacket> data_packet)
{return 0;}

}

