#ifndef ERIZO_SRC_ERIZO_MEDIA_NULLOUTPUT_HPP
#define ERIZO_SRC_ERIZO_MEDIA_NULLOUTPUT_HPP

#include <memory>

#include "MediaDefinitions.h"

namespace erizo {

class NullOutput : public MediaSink {
private:
    void close() override;
    int deliverAudioData_(std::shared_ptr<dataPacket> data_packet) override;
    int deliverVideoData_(std::shared_ptr<dataPacket> data_packet) override;
};

}


#endif //ERIZO_SRC_ERIZO_MEDIA_NULLOUTPUT_HPP
