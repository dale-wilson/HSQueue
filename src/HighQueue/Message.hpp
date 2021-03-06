// Copyright (c) 2014 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#pragma once

#include <Common/HighQueue_Export.hpp>
#include "MessageFwd.hpp"
#include <HighQueue/details/HQMemoryBlockPoolFwd.hpp>
namespace HighQueue
{
    /// @brief A handle for a block of memory
    class HighQueue_Export Message
    {
    public:

        enum class MessageType : uint16_t
        {
            Unused,
            Shutdown,
            Heartbeat,
            MulticastPacket, // Generic.  Could be specialized based on source
            Gap,             // message to replace known lost messages.
            MockMessage,
            LocalType0, LocalType1, LocalType2, LocalType3,   // for locally defined private purposes to avoid redefining this class. 
            LocalType4, LocalType5, LocalType6, LocalType7,   // for locally defined private purposes to avoid redefining this class. 
            ExtraTypeBase
        };        
        static const char * typeName(MessageType type);
        typedef uint64_t Timestamp;
        typedef uint32_t Sequence;

        /// @brief construct an empty Message
        /// @tparam AllocatorPtr points to an Allocator that attaches memory to the Message
        /// concept Allocator {
        ///    void allocate(Message & message);
        /// };
        template <typename AllocatorPtr>
        explicit Message(AllocatorPtr & allocator);

        Message() = delete;
        Message(const Message &) = delete;
        Message(Message &&) = delete;

        /// @brief destruct, returning the memory to the pool.
        ~Message();

        //////////////////////////////////////
        // Support 'raw' access to the buffer

        /// @brief return a pointer to the block of memory cast to the requested type.  
        ///
        /// This should be used when the message already contains an object of the appropriate type.
        /// or when the caller plans to construct/initialize the contents of the message.
        /// The memory is not changed by this call, 
        ///          
        /// @tparam T is the type of object in the message.
        template <typename T = byte_t>
        T* get()const;

        /// @brief return a const pointer to the block of memory cast to the requested type.
        ///
        /// This should be used when the message already contains an object of the appropriate type.
        /// The memory is not changed by this call.
        ///          
        /// @tparam T is the type of object in the message.
        template <typename T = byte_t>
        const T* getConst()const;

        /// @brief Set the number of bytes in the message that contain valid data.
        /// @param used is the total number of bytes used in the message.
        /// @returns its argument for convenience.
        /// @throws runtime_error if used exceeds the message capacity.
        size_t setUsed(size_t used);

        /// @brief Increase the amount of space used in the message.
        /// @tparam T is the type of object
        /// @param count is the number of T's to be added to the message.
        template <typename T = byte_t>
        size_t addUsed(size_t count);

        /// @brief How many bytes in this message contain valid data?
        /// @param returns the number of bytes used.
        size_t getUsed() const;

        /// @brief How many objects of type T can be added to the message.
        /// @tparam T is the type of object
        template <typename T = byte_t>
        size_t available() const;

        /// @brief Is there room for count additional objects of type T in the message?
        /// @tparam T is the type of object
        template <typename T = byte_t>
        bool needAvailable(size_t count = 1) const;

        //////////////////////////////
        // Support writing the message

        /// @brief Return the next available location in the message as a pointer to T.
        /// @tparam T is the type of object
        template <typename T = byte_t>
        T* getWritePosition()const;

        /// @brief construct a new object of type T in the message using placement new.
        /// @tparam T is the type of object to be constructed.
        /// @tparam ArgTypes are the types arguments to pass to the constructor.
        /// @param args are the actual arguments.
        /// @returns a reference to the newly created object.
        template <typename T, typename... ArgTypes>
        T & emplace(ArgTypes&&... args);

        /// @brief construct a new object of type T at the current end of the message using placement new.
        /// @tparam T is the type of object to be constructed.
        /// @tparam ArgTypes are the types arguments to pass to the constructor.
        /// @param args are the actual arguments.
        /// @returns a reference to the newly created object.
        template <typename T, typename... ArgTypes>
        T & emplaceBack(ArgTypes&&... args);

        /// @brief Use the copy constructor to construct a new object of type T in the next available location in the message.
        ///
        /// @todo This might duplicate the functionality of emplaceBack().  Consider merging them
        ///
        /// @tparam T is the type of object
        /// @param object is the object to be copied.
        /// @returns a pointer to the newly copy-constructed object in the message.
        template <typename T = byte_t>
        T& appendCopy(const T & object);

        /// @brief Use a binary copy to initialize the next available location in the message.
        /// @tparam T is the type of object
        /// @param data is the object to be copied.
        /// @param count is the number of Ts to copy.
        /// @returns a pointer to the newly initialized data in the message.
        template <typename T = byte_t>
        T* appendBinaryCopy(const T * data, size_t count);

        /// @brief Mark the message empty.
        void setEmpty();

        /// @brief Is the message empty?
        bool isEmpty()const;

        ////////////////////////////////
        // Support reading the message

        /// @brief Return the next available location in the message as a pointer to T.
        /// @tparam T is the type of object
        template <typename T = byte_t>
        T* getReadPosition()const;

        /// @brief Set the number of bytes in the message that have been read.
        /// @param read is the total number of bytes that have been read in the message.
        /// @returns its argument for convenience.
        /// @throws runtime_error if read exceeds used.
        size_t setRead(size_t read);

        /// @brief Mark the entire message unread.
        /// A shorthand for setRead(0);  
        void rewind();

        /// @brief Increase the amount of space read from the message.
        /// @tparam T is the type of object
        /// @param count is the number of T's that have been read.
        template <typename T = byte_t>
        size_t addRead(size_t count);

        /// @brief How many bytes in this message have been read?
        /// @returns the number of bytes read.
        size_t getRead() const;

        /// @brief read the next T and update the read position.
        /// @returns a reference to the in-place T
        template <typename T>
        T & read();

        /// @brief read the next T and update the read position.
        /// @returns a reference to the in-place T
        template <typename T>
        const T & readConst() const;

        /// @brief How many obects of type T remain unread in the message
        template <typename T>
        size_t getUnread() const;

        /// @brief Are there count additional objects of type T in the message?
        /// @tparam T is the type of object
        template <typename T = byte_t>
        bool needUnread(size_t count = 1) const;

        /// @brief Calls the in-place destructor on the object at the front of the message
        /// This is a companion for emplace<T>() and/or get<T>()
        /// read position and used position will be set to zero.
        template <typename T>
        void destroy() const;

        /// @brief Calls the in-place destructor on the object at the end of the currently-read portion of the message
        /// This is a companion for emplaceBack<T>() and/or read<T>()
        template <typename T>
        void destroyBack() const;

        MessageType getType()const;
        Timestamp getTimestamp()const;
        Sequence getSequence() const;

        void setType(MessageType type);
        void setTimestamp(Timestamp timestamp);
        void setSequence(Sequence sequence);

        void copyMetaInfoTo(Message & rhs);

        static const char * toText(MessageType type);

        /////////////////////////////////////
        // Support for publishing the message
    public:

        /// @brief Move the data from one message to another, leaving the source message empty.
        /// @param target is the message to receive the data
        /// @throws runtime_exception if the target message is not suitable.
        void moveTo(Message & target);

        ////////////////////////////////////////
        // Initialization and memory management
        // Not for general use
    public:

        /// @brief Associate a memory block from a HQMemoryBlockPool with this message.
        /// The block of data is general purpose.  It can be written to and reused as necessary.
        /// Normally this is only called once per message.  The message continues to use the same memory for its
        /// entire lifetime.  This is a typical use, not a requirement.
        /// @param pool The address of the pool containing ths message.
        /// @param capacity  The capacity of this message
        /// @param offset The offset to this message within the pool
        /// @param used The number of bytes used
        void set(HQMemoryBlockPool * pool, size_t capacity, size_t offset, size_t used = 0);

        /// @brief Undo a set.  Return the memory to the pool (if any), and make the message Invalid.
        void release();

        /// @brief Get the base address of the block of memory containing this message's memory.
        /// NOTE: this is not an interesting function.  Do not use it.
        byte_t * getContainer()const;

        /// @brief Get the offset from the container's base address
        /// NOTE: this is not an interesting function.  Do not use it.
        size_t getOffset()const;

        /// @brief Prepare a message for reuse -- make it Invalid.
        /// Warning:  This is not the method you want.  
        /// Call release() instead, or simply delete the Message. 
        void reset();

    private:
        const static size_t NO_POOL = ~size_t(0);
        
        byte_t * container_;
        size_t capacity_;
        size_t offset_;
        size_t used_;
        size_t read_;
        MessageType type_;
        Timestamp timestamp_; // todo define units
        Sequence sequence_;
    };

    template <typename AllocatorPtr>
    Message::Message(AllocatorPtr & allocator)
        : container_(0)
        , capacity_(0)
        , offset_(0)
        , used_(0)
        , read_(0)
        , type_(Message::MessageType::Unused)
        , timestamp_(0)
        , sequence_(0)
    {
        allocator->allocate(*this);
    }


    inline
    size_t Message::setUsed(size_t used)
    {
        if(used > capacity_)
        {
            throw std::runtime_error("Message used > capacity");
        }
        used_ = used;
        return used_;
    }

    template <typename T>
    size_t Message::addUsed(size_t count)
    {
        return setUsed(used_ + count * sizeof(T));
    }

    inline
    size_t Message::getUsed() const
    {
        return used_;
    }

    template <typename T>
    T* Message::getWritePosition()const
    {
        return reinterpret_cast<T *>(container_ + offset_ + used_);
    }

    inline
    size_t Message::setRead(size_t read)
    {
        if(read > used_)
        {
            throw std::runtime_error("Message read > used");
        }
        read_ = read;
        return read;
    }

    inline
    size_t Message::getRead() const
    {
        return read_;
    }

    template <typename T>
    size_t Message::addRead(size_t count)
    {
        return setRead(read_ + sizeof(T) * count);
    }

    template <typename T>
    T* Message::getReadPosition()const
    {
        return reinterpret_cast<T *>(container_ + offset_ + read_);
    }

    inline
    void Message::setEmpty()
    {
        used_ = 0;
        read_ = 0;
    }

    inline
    void Message::rewind()
    {
        read_ = 0;
    }

    inline
    bool Message::isEmpty()const
    {
        return used_ == 0;
    }

    inline
    Message::MessageType Message::getType()const
    {
        return type_;
    }

    inline
    Message::Timestamp Message::getTimestamp()const
    {
        return timestamp_;
    }

    inline
    Message::Sequence Message::getSequence() const
    {
        return sequence_;
    }

    inline
    void Message::setType(Message::MessageType type)
    {
        type_ = type;
    }

    inline
    void Message::setTimestamp(Message::Timestamp Timestamp)
    {
        timestamp_ = Timestamp;
    }

    inline
    void Message::setSequence(Message::Sequence sequence)
    {
        sequence_ = sequence;
    }

    inline
    void Message::copyMetaInfoTo(Message & rhs)
    {
        rhs.type_ = type_;
        rhs.timestamp_ = timestamp_; // todo define units
        rhs.sequence_ = sequence_;
    }
    
    inline
    void Message::moveTo(Message & rhs)
    {
        std::swap(container_, rhs.container_);
        std::swap(capacity_, rhs.capacity_);
        std::swap(offset_, rhs.offset_);
        rhs.used_ = used_;
        rhs.read_ = read_;
        copyMetaInfoTo(rhs);
        used_ = 0;
        read_ = 0;
    }


    template <typename T>
    T* Message::get()const
    {
        return reinterpret_cast<T *>(container_ + offset_);
    }

    template <typename T>
    const T* Message::getConst()const
    {
        return reinterpret_cast<T *>(container_ + offset_);
    }

    template <typename T>
    T & Message::read()
    {
        auto result = reinterpret_cast<T *>(container_ + offset_ + read_);
        addRead<T>(1);
        return * result;
    }

    template <typename T>
    const T & Message::readConst()const
    {
        auto result = reinterpret_cast<const T *>(container_ + offset_ + read_);
        addRead<T>(1);
        return & result;
    }

    template <typename T, typename... ArgTypes>
    T & Message::emplace(ArgTypes&&... args)
    {
        auto position = get();
        setUsed(sizeof(T)); // set this first to check for overruns
        return * new (position) T(std::forward<ArgTypes>(args)...);
    }

    template <typename T, typename... ArgTypes>
    T & Message::emplaceBack(ArgTypes&&... args)
    {
        auto position = getWritePosition<T>();
        addUsed(sizeof(T)); // set this first to check for overruns
        return * new (position)T(std::forward<ArgTypes>(args)...);
    }

    template <typename T>
    void Message::destroy() const
    {
        auto pT = get<T>();
        pT->~T();
    }

    template <typename T>
    void Message::destroyBack() const
    {
        if(getRead() >= sizeof(T))
        {
            auto pT = getReadPosition<T>();
            pT->~T();
        }
        else
        {
            throw std::runtime_error("Attempt to destroyBack object that is not present in buffer.");
        }
    }

    template <typename T>
    size_t Message::available() const
    {
        return (capacity_ - used_) / sizeof(T);
    }

    template <typename T>
    size_t Message::getUnread() const
    {
        return (used_ - read_) / sizeof(T);
    }

    template <typename T>
    bool Message::needAvailable(size_t count) const
    {
        return available<T>() >= count;
    }

    template <typename T>
    bool Message::needUnread(size_t count) const
    {
        return getUnread<T>() >= count;
    }

    template <typename T>
    T & Message::appendCopy(const T & data)
    {
        auto position = getWritePosition(); 
        // add used before writing to the message to catch overruns first.
        addUsed<T>(1);
        return * new (position) T(data);
    }

    template <typename T>
    T * Message::appendBinaryCopy(const T * data, size_t count)
    {
        auto position = getWritePosition(); 
        // add used before writing to the message to catch overruns first.
        addUsed<T>(count);
        std::memcpy(position, data, count * sizeof(T));
        return reinterpret_cast<T *>(position);
    }

   inline
   std::ostream & operator << (std::ostream & out, Message::MessageType type)
   {
       return out << Message::typeName(type);
   }
   inline const char * Message::typeName(Message::MessageType type)
   {
       switch(type)
       {
            case MessageType::Unused:
                return "Unused";
            case MessageType::Shutdown:
                return "Shutdown";
            case MessageType::Heartbeat:
                return "Heartbeat";
            case MessageType::MulticastPacket: 
                return "MulticastPacket";
            case MessageType::Gap:
                return "Gap";
            case MessageType::MockMessage:
                return "MockMessage";
            case MessageType::LocalType0: 
                return "LocalType0";
            case MessageType::LocalType1: 
                return "LocalType1";
            case MessageType::LocalType2: 
                return "LocalType2";
            case MessageType::LocalType3:
                return "LocalType3";
            case MessageType::LocalType4: 
                return "LocalType4";
            case MessageType::LocalType5: 
                return "LocalType5";
            case MessageType::LocalType6: 
                return "LocalType6";
            case MessageType::LocalType7:
                return "LocalType7";
            default:
               return "ApplicationDefinedType";
       }
   }

} // namespace HighQueue

