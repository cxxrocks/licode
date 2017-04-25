//
// Created by haoy on 2017/4/20.
//

#ifndef RTC_TEST_RTTHANDLER_HPP
#define RTC_TEST_RTTHANDLER_HPP

#include "rtp/RtcpProcessor.h"
#include "pipeline/Handler.h"
#include "Stats.h"


namespace erizo {

constexpr uint16_t kMaxSrListSize = 20;
constexpr bool kUsingRTPHack = true;
constexpr std::chrono::steady_clock::duration kStatTimeout = std::chrono::milliseconds(1000);

class WebRtcConnection;

class RttStatHandler : public Handler {
public:
    RttStatHandler() : clock_(std::make_shared<SteadyClock>()) {}

public:
    // required by Handler
    void enable() override;
    void disable() override;
    std::string getName() override;
    void read(Context *ctx, std::shared_ptr<dataPacket> packet) override;
    void write(Context *ctx, std::shared_ptr<dataPacket> packet) override;
    void notifyUpdate() override;

private:

    // for rtt calculating
    std::shared_ptr<Clock> clock_;
    WebRtcConnection* connection_ = nullptr;
    std::shared_ptr<Stats> stats_;
    std::list<std::shared_ptr<SrDelayData>> sr_delay_data_;
    time_point last_rtp_time_;

    // this is a hack, only works when testing loopback stream, do not require rtcp
    void processRtpPacket(std::shared_ptr<dataPacket> packet);
    // this is regular way measuring rtt
    void processRtcpPacket(std::shared_ptr<dataPacket> packet);
};

}



#endif //RTC_TEST_RTTHANDLER_HPP
