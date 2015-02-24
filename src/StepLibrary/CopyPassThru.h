// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once
#include <Steps/StepToMessage.h>
#include <HighQueue/Message.h>

#include <Common/Log.h>

namespace HighQueue
{
    namespace Steps
    {
        template<typename CargoType>
        class CopyPassThru : public StepToMessage
        {

        public:
            explicit CopyPassThru();

            virtual void handle(Message & message);
            virtual void finish();

        private:
            uint32_t messagesHandled_;
        };

        template<typename CargoType>
        CopyPassThru<CargoType>::CopyPassThru()
            : messagesHandled_(0)
        {
        }

        template<typename CargoType>
        void CopyPassThru<CargoType>::handle(Message & message)
        {
            if(!stopping_)
            { 
                LogDebug("CopyPassThru<CargoType> copy.");
                outMessage_->emplace<CargoType>(*message.get<CargoType>());
                message.copyMetaInfoTo(*outMessage_);
                send(*outMessage_);
                ++messagesHandled_;
            }
        }

        template<typename CargoType>
        void CopyPassThru<CargoType>::finish()
        {
            LogStatistics("Copy Pass Thru messages: " << messagesHandled_);
        }
   }
}