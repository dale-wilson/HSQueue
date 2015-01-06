#include "Common/ProntoQueuePch.h"
#define BOOST_TEST_NO_MAIN ProntoQueuePerformanceTest
#include <boost/test/unit_test.hpp>

#include <ProntoQueue/Producer.h>
#include <ProntoQueue/Consumer.h>
#include <Common/Stopwatch.h>
#include <PQPerformance/TestMessage.h>

using namespace ProntoQueue;
#define MATCH_PRONGHORN
namespace
{
    byte_t testArray[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:,.-_+()*@@@@@@@@@@@@@@";// this is Pronghorn's test message

    volatile std::atomic<uint32_t> threadsReady;
    volatile bool producerGo = false;

    void producerFunction(Connection & connection, uint32_t producerNumber, uint64_t messageCount)
    {
        Producer producer(connection, true);
        ProntoQueue::Message producerMessage;
        if(!connection.allocate(producerMessage))
        {
            std::cerr << "Failed to allocate message for producer Number " << producerNumber << std::endl;
            return;
        }

        ++threadsReady;
        while(!producerGo)
        {
            std::this_thread::yield();
        }

        for(uint64_t messageNumber = 0; messageNumber < messageCount; ++messageNumber)
        {
#ifdef MATCH_PRONGHORN
            producerMessage.appendBinaryCopy(testArray, sizeof(testArray));
#else // MATCH_PRONGHORN
            auto testMessage = producerMessage.construct<TestMessage>(producerNumber, messageNumber);
#endif //MATCH_PRONGHORN
            producer.publish(producerMessage);
        }
        // send an empty message
        producerMessage.setUsed(0);
        producer.publish(producerMessage);
    }

    void copyFunction(Connection & inConnection, Connection & outConnection, bool passThru)
    {
        Consumer consumer(inConnection);
        ProntoQueue::Message consumerMessage;
        if(!inConnection.allocate(consumerMessage))
        {
            std::cerr << "Failed to allocate consumer message for copy thread." << std::endl;
            return;
        }

        Producer producer(outConnection, true);
        ProntoQueue::Message producerMessage;
        if(!outConnection.allocate(producerMessage))
        {
            std::cerr << "Failed to allocate producer message for copy thread." << std::endl;
            return;
        }
        ++threadsReady;
        if(passThru)
        {
            while(true)
            {
                consumer.getNext(consumerMessage);
                auto used = consumerMessage.getUsed();
                producer.publish(consumerMessage);
                if(used == 0)
                {
                    return;
                }
            }
        }
        else
        {
            while(true)
            {
                consumer.getNext(consumerMessage);
                auto used = consumerMessage.getUsed();
                if(used == 0)
                {
                    producerMessage.setUsed(0);
                    producer.publish(producerMessage);
                    return;
                }
#ifdef MATCH_PRONGHORN
                producerMessage.appendBinaryCopy(consumerMessage.get(), used);
#else // MATCH_PRONGHORN
                producerMessage.construct<TestMessage>(*consumerMessage.get<TestMessage>());
#endif // MATCH_PRONGHORN
                producer.publish(producerMessage);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(testPipelinePerformance)
{
    static const size_t consumerLimit = 1;
    static const size_t copyLimit = 1;
    static const size_t producerLimit = 1;

    static const size_t entryCount = 10000;
    static const size_t messageSize = sizeof(TestMessage);
    static const uint64_t targetMessageCount = 3000000; 

    // how many buffers do we need?
    static const size_t messageCount = entryCount + consumerLimit + copyLimit + producerLimit;

    static const size_t spinCount = 10000;
    static const size_t yieldCount = ConsumerWaitStrategy::FOREVER;
    bool passThru = true;

    std::cerr << "Pipeline " << (producerLimit + copyLimit + consumerLimit) << (passThru?"+":"") << " stage: ";

    ConsumerWaitStrategy strategy(spinCount, yieldCount);
    CreationParameters parameters(strategy, entryCount, messageSize, messageCount);

    std::vector<std::shared_ptr<Connection> > connections;
    for(size_t nConn = 0; nConn < copyLimit + consumerLimit; ++nConn)
    {
        std::shared_ptr<Connection> connection(new Connection);
        connections.push_back(connection);
        std::stringstream name;
        name << "Connection " << nConn;
        connection->createLocal(name.str(), parameters);
    }

    Consumer consumer(*connections.back());
    ProntoQueue::Message consumerMessage;
    BOOST_REQUIRE(connections.back()->allocate(consumerMessage));

    producerGo = false;
    threadsReady = 0;
    uint64_t nextMessage = 0u;

    std::vector<std::thread> threads;
    for(size_t nCopy = connections.size() - 1; nCopy > 0; --nCopy)
    {
        threads.emplace_back(std::bind(copyFunction,
            std::ref(*connections[nCopy - 1]),
            std::ref(*connections[nCopy]),
            passThru)
            );
    }
    threads.emplace_back(
        std::bind(producerFunction, std::ref(*connections[0]), 1, targetMessageCount));
 
    // All wired up, ready to go.  Wait for the threads to initialize.
    while(threadsReady < threads.size())
    {
        std::this_thread::yield();
    }

    Stopwatch timer;
    producerGo = true;

    for(uint64_t messageNumber = 0; messageNumber < targetMessageCount; ++messageNumber)
    {
        consumer.getNext(consumerMessage);
#ifdef MATCH_PRONGHORN 
        // Pronghorn final stage ignores incoming data.
#else // MATCH_PRONGHORN
        auto testMessage = consumerMessage.get<TestMessage>();
        testMessage->touch();
        auto producerNumber = testMessage->producerNumber_;
        auto & msgNumber = nextMessage[producerNumber];
        if(msgNumber != testMessage->messageNumber_)
        {
            // the if avoids the performance hit of BOOST_CHECK_EQUAL unless it's needed.
            BOOST_CHECK_EQUAL(messageNumber, testMessage->messageNumber_);
        }
        ++ msgNumber; 
#endif // MATCH_PRONGHORN
    }

    auto lapse = timer.nanoseconds();
    
    for(auto pThread = threads.begin(); pThread != threads.end(); ++pThread)
    {
        pThread->join();
    }

#ifdef MATCH_PRONGHORN
    auto messageBytes = sizeof(testArray);
#else // MATCH_PRONGHORN
    auto messageBytes = sizeof(TestMessage);
#endif // MATCH_PRONGHORN
    auto messageBits = messageBytes * 8;

    std::cout << " Passed " << targetMessageCount << ' ' << messageBytes << " byte messages in "
        << std::setprecision(9) << double(lapse) / double(Stopwatch::nanosecondsPerSecond) << " seconds.  " 
        << lapse / targetMessageCount << " nsec./message "
        << std::setprecision(3) << double(targetMessageCount) / double(lapse) << " GMsg/second "
        << std::setprecision(3) << double(targetMessageCount * messageBytes) / double(lapse) << " GByte/second "
        << std::setprecision(3) << double(targetMessageCount * messageBits) / double(lapse) << " GBit/second."
        << std::endl;

    // for connections that may share buffers, close them all before any go out of scope.
    for(auto pConn = connections.begin(); pConn != connections.end(); ++pConn)
    {
        (*pConn)->close();
    }
}
