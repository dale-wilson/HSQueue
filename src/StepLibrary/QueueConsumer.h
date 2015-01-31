// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once
#include <Steps/Step.h>
#include <HighQueue/Consumer.h>

#include <Common/Log.h>

namespace HighQueue
{
    namespace Steps
    {
        class Steps_Export QueueConsumer : public Step
        {
        public:
            QueueConsumer();

            // Implement Step
            virtual void handle(Message & message);

            virtual bool configureParameter(const std::string & key, const ConfigurationNode & configuration);
            virtual void configureResources(BuildResources & resources);
            void attachConnection(const ConnectionPtr & connection);

            virtual void validate();
            virtual void start();
            virtual void stop();
            virtual void finish();

            void setStopOnShutdownMessage(bool value);

            void run();
        private:
            void startThread();

        private:
            ConnectionPtr connection_;
            std::unique_ptr<Consumer> consumer_;
            std::unique_ptr<Message> message_;

            std::shared_ptr<Step> me_;
            std::thread thread_;

            bool stopOnShutdownMessage_;
        };

    }
}