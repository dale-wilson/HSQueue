// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <StageCommon/StagePch.h>

#include "QueueProducer.h"

using namespace HighQueue;
using namespace Stages;

QueueProducer::QueueProducer()
	: solo_(false)
{
}

void QueueProducer::configureSolo(bool solo)
{
	solo_ = solo;
}


void QueueProducer::handle(Message & message)
{
    auto type = message.getType();
    producer_->publish(message);
    if(type == Message::MessageType::Shutdown)
    {
        stop();
    }
}

void QueueProducer::attachConnection(const ConnectionPtr & connection)
{
    connection_ = connection;
    producer_.reset(new Producer(connection_, solo_));
}

void QueueProducer::validate()
{
    mustNotHaveDestination();
    if(!connection_)
    {
        throw std::runtime_error("QueueProducer must have an attached Connection");
    }
}
