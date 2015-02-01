// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once
#include "HeartbeatProducerFwd.h"
#include <Steps/AsioStepToMessage.h>

#include <Common/Log.h>

namespace HighQueue
{
    namespace Steps
    {
        class Steps_Export HeartbeatProducer : public AsioStepToMessage
        {
        public:
            HeartbeatProducer();
            void setInterval(std::chrono::milliseconds interval);
            virtual bool configureParameter(const std::string & key, const ConfigurationNode & configuration);
            virtual void configureResources(BuildResources & resources);

            virtual void start();
            virtual void stop();

        private:
            void startTimer();
            void handleTimer(const boost::system::error_code& error);
        private:
            typedef boost::asio::deadline_timer Timer;
            typedef boost::posix_time::millisec Interval;
            Interval interval_;
            std::unique_ptr<Timer> timer_;
            bool cancel_;
        };
    }
}