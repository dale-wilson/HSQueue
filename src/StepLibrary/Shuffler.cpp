// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.hpp>

#include "Shuffler.hpp"
#include <Steps/StepFactory.hpp>
#include <Steps/SharedResources.hpp>
#include <Steps/Configuration.hpp>

#include <HighQueue/MemoryPool.hpp>

using namespace HighQueue;
using namespace Steps;

namespace
{
    StepFactory::Registrar<Shuffler> registerStep("shuffler", "***TESTING*** send messages out of order.");

    const std::string keyLookAhead = "look_ahead";
    const std::string keyPrime = "prime";
}

Shuffler::Shuffler()
    : lookAhead_(0)
    , prime_(relativelyPrime_)
    , position_(0)
    , published_(0)
    , heartbeats_(0)
    , shutdowns_(0)
    , leftovers_(0)
{
}

std::ostream & Shuffler::usage(std::ostream & out) const
{
    out << "    " << keyLookAhead << " The size of the \"deck\" to be shuffled (maximum distance moved)." << std::endl;
    out << "    " << keyPrime << ": A small prime number to seed the shuffle." << std::endl;
    return Step::usage(out);
}


bool Shuffler::configureParameter(const std::string & key, const ConfigurationNode & configuration)
{
    if(key == keyLookAhead)
    {
        uint64_t lookAhead;
        if(configuration.getValue(lookAhead))
        {
            lookAhead_ = size_t(lookAhead);
            return true;
        }
    }
    else if(key == keyPrime)
    {
        uint64_t prime;
        if(configuration.getValue(prime))
        {
            prime_ = size_t(prime);
            return true;
        }
    }
    else
    {
        return Step::configureParameter(key, configuration);
    }
    return false;
}

void Shuffler::configureResources(const SharedResourcesPtr & resources)
{
    resources->requestMessages(lookAhead_);
    return Step::configureResources(resources);
}

void Shuffler::attachResources(const SharedResourcesPtr & resources)
{
    auto & memoryPool = resources->getMemoryPool();
    if(!memoryPool)
    {
        throw std::runtime_error("OrderedMerge: no memory pool available.");
    }

    while(pendingMessages_.size() < lookAhead_)
    {
        pendingMessages_.emplace_back(new Message(memoryPool));
    }
    Step::attachResources(resources);
}

void Shuffler::validate()
{
    mustHaveDestination();

    if(pendingMessages_.size() < lookAhead_)
    {
        throw std::runtime_error("Shuffler working messages not initialized.");
    }
    Step::validate();
}

void Shuffler::handle(Message & message)
{
    auto type = message.getType();
    if(type == Message::MessageType::Heartbeat)
    {
        handleHeartbeat(message);
    }
    else if(type == Message::MessageType::Shutdown)
    {
        handleShutdown(message);
    }
    else
    {
        handleDataMessage(message);
    }
}

void Shuffler::handleHeartbeat(Message & message)
{
    ++heartbeats_;
    publishPendingMessages();
    send(message);
}

void Shuffler::handleShutdown(Message & message)
{
    ++shutdowns_;
    publishPendingMessages();
    send(message);
}

void Shuffler::handleDataMessage(Message & message)
{
    position_ += prime_;
    size_t index = position_ % lookAhead_;
    if(!pendingMessages_[index]->isEmpty())
    {
        ++published_;
        send(*pendingMessages_[index]);
    }
    message.moveTo(*pendingMessages_[index]);
}

void Shuffler::publishPendingMessages()
{
    for(auto & message : pendingMessages_)
    {
        if(!message->isEmpty())
        {
            ++published_;
            send(*message);
        }
    }
}

void Shuffler::finish()
{
    leftovers_ = 0;
    for(auto & message : pendingMessages_)
    {
        if(!message->isEmpty())
        {
            ++leftovers_;
        }
    }
    Step::finish();
}

void Shuffler::logStats()
{
    LogStatistics("Shuffler published: " << published_);
    if(leftovers_ != 0)
    {
        LogStatistics("Shuffler unpublished: " << leftovers_);
    }
    LogStatistics("Shuffler heartbeats: " << heartbeats_);
    LogStatistics("Shuffler shutdowns: " << shutdowns_);
}
