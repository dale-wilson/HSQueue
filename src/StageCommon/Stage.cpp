// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <StageCommon/StagePch.h>

#include "Stage.h"

using namespace HighQueue;
using namespace Stages;

Stage::Stage()
    : paused_(false)
    , stopping_(false)
{
}

Stage::~Stage()
{
    try
    {
        stop();
        finish();
    }
    catch(const std::exception & ex)
    {
        LogError("Stage destruction: " << ex.what());
    }
}

bool Stage::configure(const ConfigurationNode & configuration)
{
    // default to do nothing
    return true;
}

void Stage::attachDestination(const StagePtr & destination)
{
    primaryDestination_ = destination;
}

void Stage::attachDestination(const std::string & name, const StagePtr & destination)
{
    destinations_.push_back(std::make_pair(name, destination));
}

void Stage::attachConnection(const ConnectionPtr & connection)
{
    // default.  No thank you.
}

void Stage::attachMemoryPool(const MemoryPoolPtr & pool)
{
    // default.  No thank you.
}

void Stage::attachIoService(const AsioServicePtr & connection)
{
    // default.  No thank you.
}

void Stage::validate()
{
    // default to do nothing
}

void Stage::start()
{
}

void Stage::pause()
{
    paused_ = true;
}

void Stage::resume()
{
    paused_ = false;
}

void Stage::stop()
{
    stopping_ = true;
}


void Stage::finish()
{
    // default to do nothing.
}

void Stage::mustHaveDestination()
{
    if(!primaryDestination_)
    {
        throw std::runtime_error("Configuration error: Destination required");
    }
}

void Stage::mustHaveDestination(const std::string & name)
{
    if(destinationIndex(name) == ~size_t(0))
    {
        std::stringstream msg;
        msg << "Configuration error: Destination " << name << "required";
        throw std::runtime_error(msg.str());
    }
}

void Stage::mustNotHaveDestination()
{
    if(primaryDestination_ || destinations_.size() > 0)
    {
        throw std::runtime_error("Configuration error: Destination not allowed");
    }
}

size_t Stage::destinationIndex(const std::string & name)
{
    for(size_t index = 0; index < destinations_.size(); ++index)
    {
        auto & namedDestination = destinations_[index];
        if(namedDestination.first == name)
        {
            return index;
        }
    }
    return ~size_t(0);
}
