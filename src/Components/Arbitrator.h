// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once
#include <ComponentCommon/HeaderGenerator.h>
#include <HighQueue/Consumer.h>

#define USE_DEBUG_MESSAGE 0
#include <ComponentCommon/DebugMessage.h>

namespace HighQueue
{
    namespace Components
    {
        template<typename CargoClass>
        class Arbitrator : public std::enable_shared_from_this<Arbitrator>
        {
        public:
            Arbitrator(ConnectionPtr & connection, uint32_t messageCount = 0, bool quitOnEmptyMessage = true);
            ~Arbitrator();

            void start();
            void stop();
            void pause();
            void resume();

            uint32_t errors()const
            {
                return sequenceError_;
            }

            void run();
        private:
            HeaderGenerator headerGenerator_;
            ConnectionPtr connection_;
            Consumer consumer_;
            Message message_;
            uint32_t messageCount_;
            bool quitOnEmptyMessage_;
            bool paused_;
            bool stopping_;

            uint32_t sequenceError_;
            
            std::shared_ptr<TestMessageConsumer> me_;
            std::thread thread_;
        };

        template<size_t Extra, typename HeaderGenerator>
        TestMessageConsumer<Extra, HeaderGenerator>::TestMessageConsumer(ConnectionPtr & connection, uint32_t messageCount, bool quitOnEmptyMessage)
            : connection_(connection)
            , consumer_(connection)
            , message_(connection)
            , messageCount_(messageCount)
            , quitOnEmptyMessage_(quitOnEmptyMessage)
            , paused_(false)
            , stopping_(false)
            , sequenceError_(0)
        {
        }

        template<size_t Extra, typename HeaderGenerator>
        TestMessageConsumer<Extra, HeaderGenerator>::~TestMessageConsumer()
        {
            stop();
        }

        template<size_t Extra, typename HeaderGenerator>
        void TestMessageConsumer<Extra, HeaderGenerator>::start()
        {
            me_ = shared_from_this();
            thread_ = std::thread(std::bind(
                TestMessageConsumer<Extra, HeaderGenerator>::run,
                this));
        }

        template<size_t Extra, typename HeaderGenerator>
        void TestMessageConsumer<Extra, HeaderGenerator>::stop()
        {
            stopping_ = true;
            if(me_)
            { 
                thread_.join();
                me_.reset();
            }
        }

        template<size_t Extra, typename HeaderGenerator>
        void TestMessageConsumer<Extra, HeaderGenerator>::pause()
        {
            paused_ = true;
        }

        template<size_t Extra, typename HeaderGenerator>
        void TestMessageConsumer<Extra, HeaderGenerator>::resume()
        {
            paused_ = false;
        }

        template<size_t Extra, typename HeaderGenerator>
        void TestMessageConsumer<Extra, HeaderGenerator>::run()
        {
            DebugMessage("Consumer start.\n");
            uint32_t messageCount = 0; 
            uint32_t nextSequence = 0;
            while(!stopping_)
            {
                stopping_ = !consumer_.getNext(message_);
                if(!stopping_ && quitOnEmptyMessage_ && message_.getUsed() == 0)
                {
                    stopping_ = true;
                }
                if(!stopping_)
                { 
                    headerGenerator_.consumeHeader(message_);
                    auto testMessage = message_.get<ActualMessage>();
                    testMessage->touch();
                    if(nextSequence != testMessage->messageNumber())
                    {
                        ++sequenceError_;
                        nextSequence = testMessage->messageNumber();
                    }
                    ++nextSequence;
                    message_.destroy<ActualMessage>();
                    ++messageCount;
                    stopping_ = !(messageCount_ == 0 || messageCount < messageCount_);
                }
            }
            DebugMessage("Consumer received " << messageCount << " messages with " << sequenceError_ << " errors" << std::endl);
        }
   }
}