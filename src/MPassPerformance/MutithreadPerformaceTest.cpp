#include "Common/MPassPch.h"
#define BOOST_TEST_NO_MAIN MPassPerformanceTest
#include <boost/test/unit_test.hpp>

#include <InfiniteVector/IvProducer.h>
#include <InfiniteVector/IvConsumer.h>
#include <Common/Stopwatch.h>

using namespace MPass;
using namespace InfiniteVector;

namespace
{
    struct TestMessage
    {
        uint64_t data_;
        TestMessage(uint64_t data)
        :data_(data)
        {
        }
    };
}

BOOST_AUTO_TEST_CASE(testBasicMessagePassingPerformance)
{
    IvConsumerWaitStrategy strategy;
    size_t entryCount = 100000;
    size_t bufferSize = sizeof(TestMessage);
    size_t bufferCount = entryCount + 10;
    IvCreationParameters parameters(strategy, entryCount, bufferSize, bufferCount);
    IvConnection connection;
    connection.CreateLocal("LocalIv", parameters);

    IvProducer producer(connection);
    IvConsumer consumer(connection);
    Buffers::Buffer producerBuffer;
    connection.allocate(producerBuffer);
    Buffers::Buffer consumerBuffer;
    connection.allocate(consumerBuffer);

    Stopwatch timer;

    for(uint64_t nMessage = 0; nMessage < entryCount; ++nMessage)
    {
        producerBuffer.construct<TestMessage>(nMessage);
        producer.publish(producerBuffer);
    }
    auto publishTime = timer.microseconds();
    timer.reset();
    // if we published another message now, it would hang.
    // todo: think of some way around that.


    // consume the messages.
    for(uint64_t nMessage = 0; nMessage < entryCount; ++nMessage)
    {
        consumer.getNext(producerBuffer);
        auto testMessage = producerBuffer.get<TestMessage>();
        if(nMessage != testMessage->data_)
        {
            // the if avoids the performance hit of BOOST_CHECK_EQUAL unless it's needed.
            BOOST_CHECK_EQUAL(nMessage, testMessage->data_);
        }
    }
    auto consumeTime = timer.microseconds();

    std::cout << entryCount << " messages.  Publish " << publishTime * 1000 / entryCount << " nsec.  Consume " << consumeTime * 1000 / entryCount << " nsec." << std::endl;
    timer.reset();
    uint64_t limit1 = 100000;
    uint64_t limit2 = 1000;
    uint64_t messageNumber = 0;
    for(uint64_t nLoop1 = 0; nLoop1 < limit1; ++nLoop1)
    {
        for(uint64_t nLoop2 = 0; nLoop2 < limit2; ++nLoop2)
        {
            ++messageNumber;
            producerBuffer.construct<TestMessage>(messageNumber);
            producer.publish(producerBuffer);
            consumer.getNext(consumerBuffer);
            auto testMessage = consumerBuffer.get<TestMessage>();
            if(messageNumber != testMessage->data_)
            {
                // the if avoids the performance hit of BOOST_CHECK_EQUAL unless it's needed.
                BOOST_CHECK_EQUAL(messageNumber, testMessage->data_);
            }
        }
    }
    auto messagePassTime = timer.microseconds();
    std::cout << messageNumber << " messages.  Message Pass time " << messagePassTime * 1000 / messageNumber << " nsec." << std::endl;

}
