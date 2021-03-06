// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.hpp>

#include "AsioStepToMessage.hpp"
#include <Steps/SharedResources.hpp>
using namespace HighQueue;
using namespace Steps;
AsioStepToMessage::AsioStepToMessage()
{
}

AsioStepToMessage::~AsioStepToMessage()
{
}

void AsioStepToMessage::configureResources(const SharedResourcesPtr & resources)
{
    resources->requestAsioThread(0, 1);
    StepToMessage::configureResources(resources);
}

void AsioStepToMessage::attachResources(const SharedResourcesPtr & resources)
{
    ioService_ = resources->getAsioService();
    StepToMessage::attachResources(resources);
}

void AsioStepToMessage::validate()
{
    if(!ioService_)
    {
        std::stringstream msg;
        msg << "No Asio service attached to " << name_;
        std::runtime_error(msg.str());
    }
}


void AsioStepToMessage::start()
{
    me_ = shared_from_this();
}

void AsioStepToMessage::finish()
{
    StepToMessage::finish();
    if(me_)
    { 
        me_.reset();
    }
}
