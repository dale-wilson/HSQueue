/// @file IvConnection.cpp
#include <Common/MPassPch.h>
#include "IvConsumer.h"
#include <InfiniteVector/IvReservePosition.h>
using namespace MPass;
using namespace InfiniteVector;

IvConsumer::IvConsumer(IvConnection & connection)
: connection_(connection)
, header_(connection.getHeader())
, resolver_(header_)
, entryAccessor_(resolver_, header_->entries_, header_->entryCount_)
, readPosition_(*resolver_.resolve<volatile Position>(header_->readPosition_))
, publishPosition_(*resolver_.resolve<volatile Position>(header_->publishPosition_))
, cachedPublishPosition_(publishPosition_)
, waitStrategy_(header_->consumerWaitStrategy_)
, spins_(waitStrategy_.spinCount_)
, yields_(waitStrategy_.yieldCount_)
, sleeps_(waitStrategy_.sleepCount_)
{
}

bool IvConsumer::tryGetNext(InfiniteVector::Buffer & buffer)
{
    while(true)
    {
        Position readPosition = readPosition_;
        if(readPosition >= cachedPublishPosition_)
        {
            std::_Atomic_thread_fence(std::memory_order::memory_order_consume);
            readPosition = readPosition_;
            cachedPublishPosition_ = publishPosition_;
            if(readPosition >= cachedPublishPosition_)
            {
                return false;
            }
        }
        IvEntry & entry = entryAccessor_[readPosition];
        if(entry.status_ == IvEntry::Status::OK)
        {
            entry.buffer_.moveTo(buffer);
            ++readPosition_;
            return true;
        }
        ++readPosition_;
    }

}

void IvConsumer::getNext(InfiniteVector::Buffer & buffer)
{
    size_t remainingSpins = spins_;
    size_t remainingYields = yields_;
    size_t remainingSleeps = sleeps_;
    while(true)
    {
        if(tryGetNext(buffer))
        {
            return;
        }
        if(remainingSpins > 0)
        {
            if(remainingSpins != IvConsumerWaitStrategy::FOREVER)
            {
                --remainingSpins;
            }
        }
        else if(remainingYields > 0)
        {
            if(remainingYields != IvConsumerWaitStrategy::FOREVER)
            {
                --remainingYields;
            }
            std::this_thread::yield();
        }
        else if(remainingSleeps > 0)
        {
            if(remainingSleeps != IvConsumerWaitStrategy::FOREVER)
            {
                --remainingSleeps;
            }
            std::this_thread::sleep_for(waitStrategy_.sleepPeriod_);
        }
        else
        {
            std::unique_lock<std::mutex> guard(header_->consumerWaitMutex_);
            if(tryGetNext(buffer))
            {
                return;
            }
            if(header_->consumerWaitConditionVariable_.wait_for(guard, waitStrategy_.mutexWaitTimeout_)
                == std::cv_status::timeout)
            {
                if(tryGetNext(buffer))
                {
                    return;
                }
                throw std::exception("Consumer wait timeout.");
            }
        }
    }
}
    





