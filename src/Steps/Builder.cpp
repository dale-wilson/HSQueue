// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.h>

#include "Builder.h"
#include <Steps/Configuration.h>
#include <Steps/Step.h>
#include <Steps/StepFactory.h>
#include <Common/ReverseRange.h>

using namespace HighQueue;
using namespace Steps;

namespace
{
    std::string keyPipe("pipe");
    std::string keyDestination("destination");
}

Builder::Builder()
{
}

Builder::~Builder()
{
}
            
bool Builder::construct(const ConfigurationNode & config)
{
    for(auto rootChildren = config.getChildren();
        rootChildren->has();
        rootChildren->next())
    {
        StepPtr noParent;
        auto child = rootChildren->getChild();
        const auto & key = child->getName();
        if(key == keyPipe)
        {
            if(!constructPipe(*child, noParent))
            {
                return false;
            }
        }
        else
        {
            LogFatal("Unknown configuration key: " << key);
            return false;
        }
    }

    // we have created all Steps, and used them to configure the build resources.
    resources_.createResources();
    for(auto & Step : Steps_)
    {
        Step->attachResources(resources_);
    }
    // now check to see if we got it right.
    for(auto & Step : Steps_)
    {
        Step->validate();
    }

    return true;
}

void Builder::start()
{
    for(auto & Step : ReverseRange<Steps>(Steps_))
    {
        Step->start();
    }
    resources_.start();
}

void Builder::stop()
{
    resources_.stop();
    for(auto & Step : Steps_)
    {
        Step->stop();
    }
}

void Builder::finish()
{
    resources_.finish();
    for(auto & Step : Steps_)
    {
        Step->finish();
    }
}
namespace
{
    bool xyzzy(const StepPtr & step, const std::string & str, const ConfigurationNode & node)
    {
        return false;
    }
}
bool Builder::constructPipe(const ConfigurationNode & config, const StepPtr & parentStep)
{
    StepPtr previousStep = parentStep;
    for(auto rootChildren = config.getChildren();
        rootChildren->has();
        rootChildren->next())
    {
        auto child = rootChildren->getChild();
        const auto & key = child->getName();
        const auto & step = StepFactory::make(key);
        if(!step)
        {
            return false;
        }

#ifdef _WIN32 // VC2013 implementation of std::bind sucks (technical term)
        step->setParameterHandler(boost::bind(&Builder::configureParameter, this, _1, _2, _3));
#else // _WIN32
        step->setParameterHandler(boost::bind(&Builder::configureParameter, this, _1, _2, _3));
#endif // WIN32
        if(!step->configure(*child))
        {
            return false;
        }
        step->configureResources(resources_);
        Steps_.emplace_back(step);
        if(previousStep)
        {
            previousStep->attachDestination(step->getName(), step);
        }
        previousStep = step;
    }
    return true;
}

bool Builder::configureParameter(const StepPtr & step, const std::string & key, const ConfigurationNode & configuration)
{
    if(key == keyDestination)
    {
        return constructPipe(configuration, step);
    }
    LogError("Unknown parameter \"" << key << "\" for " << step->getName());
    return false; // false meaning "huh?"
}
