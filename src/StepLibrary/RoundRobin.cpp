// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.hpp>

#include "RoundRobin.hpp"

#include <Steps/StepFactory.hpp>
#include <Steps/SharedResources.hpp>

using namespace HighQueue;
using namespace Steps;

namespace
{
    StepFactory::Registrar<RoundRobin> registerStep("round_robin", "Distribute messages to multiple destinations -- round robin.");
}

RoundRobin::RoundRobin()
    : messagesHandled_(0)
    , heartbeatsHandled_(0)
    , shutdownsHandled_(0)
{
}

void RoundRobin::handle(Message & message)
{
    if(!stopping_)
    { 
        auto type = message.getType();
        if(type == Message::MessageType::Heartbeat || type == Message::MessageType::Shutdown)
        {
            for(size_t nDestination = 0; nDestination < getDestinationCount(); ++nDestination)
            {
                outMessage_->appendBinaryCopy(message.get(), message.getUsed());
                message.copyMetaInfoTo(*outMessage_);
                send(nDestination, *outMessage_);
            }
            if(type == Message::MessageType::Heartbeat)
            {
                ++heartbeatsHandled_;
            }
            else
            {
                ++shutdownsHandled_;
            }
        }
        else
        {
            LogTrace("RoundRobin route " << messagesHandled_ << " to " << (messagesHandled_ % getDestinationCount()));
            send(messagesHandled_ % getDestinationCount(), message);
            ++messagesHandled_;
        }
    }
}

void RoundRobin::logStats()
{
    LogStatistics("RoundRobin messages: " << messagesHandled_);
    LogStatistics("RoundRobin heartbeats: " << heartbeatsHandled_);
    LogStatistics("RoundRobin shutdowns: " << shutdownsHandled_);
}

