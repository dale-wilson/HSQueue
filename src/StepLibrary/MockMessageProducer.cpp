// Copyright (c) 2015 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Steps/StepPch.h>

#include "MockMessageProducer.h"
#include <Steps/StepFactory.h>
using namespace HighQueue;
using namespace Steps;

namespace
{
    StepFactory::Registrar<MockMessageProducer<SmallMockMessage> > registerStepSmall("small_test_message_producer", "Produce small test messages");
    StepFactory::Registrar<MockMessageProducer<MediumMockMessage> > registerStepMedium("medium_test_message_producer", "Produce medium test messages");
    StepFactory::Registrar<MockMessageProducer<LargeMockMessage> > registerStepLarge("large_test_message_producer", "Produce large test messages");

    const std::string keyMessageCount = "message_count";
    const std::string keyProducerNumber = "producer_number";
}

BaseMessageProducer::BaseMessageProducer()
    : startSignal_(0)
    , messageCount_(~uint32_t(0))
    , producerNumber_(0)
    , messageNumber_(0)
{
}

BaseMessageProducer::~BaseMessageProducer()
{
}


std::ostream & BaseMessageProducer::usage(std::ostream & out) const
{
    out << "    " << keyMessageCount << ": How many messages to generate (0 means keep going until stopped." << std::endl;
    out << "    " << keyProducerNumber << ": An integer to identify messages generated by this producer." << std::endl;
    return ThreadedStepToMessage::usage(out);
}

bool BaseMessageProducer::configureParameter(const std::string & key, const ConfigurationNode & config)
{
    if(key == keyMessageCount)
    {
        uint64_t messageCount;
        if(!config.getValue(messageCount))
        {
            LogFatal("MockMessageProducer " << name_ << " can't interpret value for " << keyMessageCount);
            return false;
        }
        messageCount_ = uint32_t(messageCount);
    }
    else if(key == keyProducerNumber)
    {
        uint64_t producerNumber;
        if(!config.getValue(producerNumber))
        {
            LogFatal("MockMessageProducer " << name_ << " can't interpret value for " << keyProducerNumber);
            return false;
        }
        producerNumber_ = uint32_t(producerNumber);
    }
    else
    {
        return ThreadedStepToMessage::configureParameter(key, config);
    }
    return true;
}

void BaseMessageProducer::validate()
{
    ThreadedStepToMessage::validate();
    if(messageCount_ == ~uint32_t(0))
    {
        std::stringstream msg;
        msg << "MessageProducer " << name_ << " missing \"" << keyMessageCount << "\" parameter.";
        throw std::runtime_error(msg.str());
    }
}

void BaseMessageProducer::setStartSignal(volatile bool * startSignal)
{
    startSignal_ = startSignal;
}

void BaseMessageProducer::finish()
{
    logStats();
}

void BaseMessageProducer::logStats()
{
    LogStatistics("MockMessageProducer  " << name_ << " " << producerNumber_ << " messages published:" << messageNumber_ );
}
