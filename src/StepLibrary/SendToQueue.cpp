// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.hpp>

#include "SendToQueue.hpp"
#include <Steps/StepFactory.hpp>
#include <Steps/Configuration.hpp>
#include <Steps/SharedResources.hpp>

using namespace HighQueue;
using namespace Steps;

namespace
{
    StepFactory::Registrar<SendToQueue> registerStep("send_to_queue", "Send messages to an input queue defined elsewehere.");

    const std::string keyQueueName = "queue";
}


SendToQueue::SendToQueue()
{
}

std::ostream & SendToQueue::usage(std::ostream & out) const
{
    out << "    " << keyQueueName << ": Identifies the queue to which messages will be sent.  Should match the name of an input_queue." << std::endl;
    return Step::usage(out);
}

bool SendToQueue::configureParameter(const std::string & key, const ConfigurationNode & configuration)
{
    if(key == keyQueueName)
    {
        configuration.getValue(queueName_);
        return true;
    }
    else
    {
        return Step::configureParameter(key, configuration);
    }
}

void SendToQueue::attachResources(const SharedResourcesPtr & resources)
{
    connection_ = resources->findQueue(queueName_);
    if(connection_)
    {
        connection_->willProduce();
    }
    else
    {
        std::stringstream qnames;
        std::string delimiter;
        auto & queues = resources->getQueues();
        for(auto queue : queues)
        {
            qnames << delimiter << queue.first;
            delimiter = ", ";
        }
        LogError("SendToQueue can't find queue \"" << queueName_ << "\" in [" << qnames.str() << "]");
    }
    Step::attachResources(resources);
}


void SendToQueue::validate()
{
    mustNotHaveDestination();
    if(!connection_)
    {
        std::stringstream msg;
        msg << "SendToQueue can't find queue \"" << queueName_ << "\".";
        throw std::runtime_error(msg.str());
    }
    Step::validate();
}

void SendToQueue::start()
{
    producer_.reset(new Producer(connection_));
    Step::start();
}

void SendToQueue::handle(Message & message)
{
    auto type = message.getType();
    producer_->publish(message);
    if(type == Message::MessageType::Shutdown)
    {
        stop();
    }
}

void SendToQueue::stop()
{
    Step::stop();
    if(producer_)
    {
        producer_->stop();
    }
}
