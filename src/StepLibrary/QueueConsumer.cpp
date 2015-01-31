// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.h>

#include "QueueConsumer.h"
#include <Steps/StepFactory.h>

using namespace HighQueue;
using namespace Steps;

namespace
{
    StepFactory::Registrar<QueueConsumer> registerStep("queue_consumer");
}


QueueConsumer::QueueConsumer()
:stopOnShutdownMessage_(false)
{
    setName("QueueConsumer"); // default name
}

void QueueConsumer::handle(Message & message)
{
    throw std::runtime_error("QueueConsuemr does not accept incoming Messages");
}


bool QueueConsumer::configureParameter(const std::string & key, const ConfigurationNode & configuration)
{
    int todo;
    return Step::configureParameter(key, configuration);
}

void QueueConsumer::configureResources(BuildResources & resources)
{
    int todo;
    return Step::configureResources(resources);
}

void QueueConsumer::attachConnection(const ConnectionPtr & connection)
{
    connection_ = connection;
    consumer_.reset(new Consumer(connection_));
    message_.reset(new Message(connection_));
}

void QueueConsumer::validate()
{
    mustHaveDestination();
    if(!connection_)
    {
        throw std::runtime_error("QueueConsumer must have an attached Connection");
    }
}

void QueueConsumer::setStopOnShutdownMessage(bool value)
{
    stopOnShutdownMessage_ = value;
}

void QueueConsumer::start()
{
    me_ = shared_from_this();
    thread_ = std::thread(std::bind(
        &QueueConsumer::startThread,
        this));
}

void QueueConsumer::startThread()
{
    LogTrace("QueueConsumer thread starts. @" << (void *) this);
    run();
    LogTrace("QueueConsumer thread exits. @" << (void *) this);
}

void QueueConsumer::stop()
{
    Step::stop();
    consumer_->stop();
}
void QueueConsumer::finish()
{
    if(me_)
    {
        if(std::this_thread::get_id() != thread_.get_id())
        {
            thread_.join();
            me_.reset();
        }
        else
        {
            LogWarning("QueueConsumer::finish() should not be called from the running thread.");
        }
    }
}


void QueueConsumer::run()
{
    while(!stopping_)
    {
        stopping_ = !consumer_->getNext(*message_);
        if(!stopping_)
        {
            auto type = message_->getType();
            send(*message_);
            if(stopOnShutdownMessage_ && type == Message::MessageType::Shutdown)
            {
                stop();
            }
            
        }
        else
        {
            LogTrace("QueueConsumer::stopped by getNext? " << (void *) this << ": " << stopping_);
        }
    }
}
