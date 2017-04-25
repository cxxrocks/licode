//
// Created by haoy on 2017/4/20.
//

#include "RttStatHandler.h"

#include "WebRtcConnection.h"

namespace erizo {

void RttStatHandler::processRtpPacket(std::shared_ptr<dataPacket> packet) {
    if (!connection_) {
        return;
    }

    RtpHeader* head = reinterpret_cast<RtpHeader*>(packet->data);
    uint32_t ssrc = head->getSSRC();

    if (connection_->isVideoSourceSSRC(ssrc)) {
        // incoming video packet
        auto headerLength = head->getHeaderLength();
        unsigned char* magic = (unsigned char*)(packet->data + headerLength + 4);
        if ((headerLength + 4 + 2 + 8) > packet->length || magic[0] != 0xFF || magic[1] != 0xFF) {
            return;
        }

        char* time = packet->data + headerLength + 6;

        uint64_t sendTimeMs;
        memcpy(&sendTimeMs, time, sizeof(sendTimeMs));
        time_point now = clock_->now();
        uint64_t nowMs = ClockUtils::timePointToMs(clock_->now());
        uint64_t rtt = nowMs - sendTimeMs;

        if (!stats_->getNode()[ssrc].hasChild("rtt")) {
            stats_->getNode()[ssrc].insertStat("rtt", MovingAverageStat{3});
        }
        stats_->getNode()[ssrc]["rtt"] += rtt;
        last_rtp_time_ = now;
    } else if (connection_->isVideoSinkSSRC(ssrc)) {
        // outgoing video packet
        auto headerLength = head->getHeaderLength();
        if ((headerLength + 4 + 2 + 8) > packet->length) {
            return;
        }
        unsigned char* magic = (unsigned char*)(packet->data + headerLength + 4);
        magic[0] = 0xFF; magic[1] = 0xFF;

        char* time = packet->data + headerLength + 6;
        time_point now = clock_->now();
        uint64_t nowMs = ClockUtils::timePointToMs(clock_->now());
        memcpy(time, &nowMs, sizeof(now));

        // drop expired stat
        if (now - last_rtp_time_ > kStatTimeout) {
            auto ssrc = connection_->getVideoSourceSSRC();
            stats_->getNode()[ssrc].removeStat("rtt");
        }
    }
}
void RttStatHandler::processRtcpPacket(std::shared_ptr<dataPacket> packet) {
    if (!connection_) {
        return;
    }

    uint32_t ssrc = 0;
    RtcpHeader* chead = reinterpret_cast<RtcpHeader*>(packet->data);
    if (chead->isFeedback()) {
        ssrc = chead->getSourceSSRC();
        if (!connection_->isVideoSourceSSRC(ssrc)) {
            return;
        }
    } else {
        ssrc = chead->getSSRC();
        if (!connection_->isVideoSinkSSRC(ssrc)) {
            return;
        }
    }

    if (chead->packettype == RTCP_Sender_PT) {
        uint64_t now = ClockUtils::timePointToMs(clock_->now());
        uint32_t ntp = chead->get32MiddleNtp();
        sr_delay_data_.push_back(std::make_shared<SrDelayData>(ntp, now));
        if (sr_delay_data_.size() > kMaxSrListSize) {
            sr_delay_data_.pop_front();
        }
    } else if (chead->packettype == RTCP_Receiver_PT) {
        uint32_t delay_since_last_ms = (chead->getDelaySinceLastSr() * 1000) / 65536;
        int64_t now_ms = ClockUtils::timePointToMs(clock_->now());
        auto last_sr = chead->getLastSr();
        auto value = std::find_if(sr_delay_data_.begin(), sr_delay_data_.end(),
                                  [last_sr](const std::shared_ptr<SrDelayData> sr_info) {
                                      return sr_info->sr_ntp == last_sr;
                                  });
        if (value != sr_delay_data_.end()) {
            uint32_t delay = now_ms - (*value)->sr_send_time - delay_since_last_ms;
            stats_->getNode()[ssrc].insertStat("rtt", CumulativeStat{delay});
        }
    }
}

void RttStatHandler::read(Handler::Context *ctx, std::shared_ptr<dataPacket> packet) {
    RtcpHeader* chead = reinterpret_cast<RtcpHeader*> (packet->data);
    if (!chead->isRtcp() && kUsingRTPHack) {
        processRtpPacket(packet);
    }
    if (chead->isRtcp() && !kUsingRTPHack) {
        processRtcpPacket(packet);
    }
    ctx->fireRead(packet);
}

void RttStatHandler::write(Handler::Context *ctx, std::shared_ptr<dataPacket> packet) {
    RtcpHeader* chead = reinterpret_cast<RtcpHeader*> (packet->data);
    if (!chead->isRtcp() && kUsingRTPHack) {
        processRtpPacket(packet);
    }
    if (chead->isRtcp() && !kUsingRTPHack) {
        processRtcpPacket(packet);
    }
    ctx->fireWrite(packet);
}

void RttStatHandler::notifyUpdate() {
    if (connection_) {
        return;
    }
    auto pipeline = getContext()->getPipelineShared();
    connection_ = pipeline->getService<WebRtcConnection>().get();
    stats_ = pipeline->getService<Stats>();
}

std::string RttStatHandler::getName() {
    return "rtt-stat";
}

void RttStatHandler::disable() {}

void RttStatHandler::enable() {}


}