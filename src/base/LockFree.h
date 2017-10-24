#ifndef BASE_LOCKFREE_H
#define BASE_LOCKFREE_H

#include <utility>

#include <stdint.h>
#include <sched.h>
#include <assert.h>

namespace base 
{
namespace lockfree
{

#ifdef __GNUC__
// Atomic functions in GCC are present from version 4.1.0 on
// http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html

// Test for GCC >= 4.1.0
#if (__GNUC__ < 4) || \
    ((__GNUC__ == 4) && ((__GNUC_MINOR__ < 1) || \
                        ((__GNUC_MINOR__     == 1) && \
                         (__GNUC_PATCHLEVEL__ < 0))) )

#error Atomic built-in functions are only available in GCC in versions >= 4.1.0
#endif // end of check for GCC 4.1.0

/// @brief atomically adds a_count to the variable pointed by a_ptr
/// @return the value that had previously been in memory
#define AtomicAdd(a_ptr,a_count) __sync_fetch_and_add (a_ptr, a_count)

/// @brief atomically substracts a_count from the variable pointed by a_ptr
/// @return the value that had previously been in memory
#define AtomicSub(a_ptr,a_count) __sync_fetch_and_sub (a_ptr, a_count)

/// @brief Compare And Swap
///        If the current value of *a_ptr is a_oldVal, then write a_newVal into *a_ptr
/// @return true if the comparison is successful and a_newVal was written
#define CAS(a_ptr, a_oldVal, a_newVal) __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

/// @brief Compare And Swap
///        If the current value of *a_ptr is a_oldVal, then write a_newVal into *a_ptr
/// @return the contents of *a_ptr before the operation
#define CASVal(a_ptr, a_oldVal, a_newVal) __sync_val_compare_and_swap(a_ptr, a_oldVal, a_newVal)

#else
#error Atomic functions such as CAS or AtomicAdd are not defined for your compiler. Please add them in atomic_ops.h
#endif // __GNUC__

#define ARRAY_LOCK_FREE_Q_DEFAULT_SIZE 65536 // 2^16 = 65,536 elements by default

// define this variable if calls to "size" must return the real size of the 
// queue. If it is undefined  that function will try to take a snapshot of 
// the queue, but returned value might be bogus
#undef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
//#define ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE 1

/// @brief Lock-free queue based on a circular array
/// No allocation of extra memory for the nodes handling is needed, but it has to add
/// extra overhead (extra CAS operation) when inserting to ensure the thread-safety of the queue
/// ELEM_T     represents the type of elementes pushed and popped from the queue
/// TOTAL_SIZE size of the queue. It should be a power of 2 to ensure 
///            indexes in the circular queue keep stable when the uint32_t 
///            variable that holds the current position rolls over from FFFFFFFF
///            to 0. For instance
///            2    -> 0x02 
///            4    -> 0x04
///            8    -> 0x08
///            16   -> 0x10
///            (...) 
///            1024 -> 0x400
///            2048 -> 0x800
///
///            if queue size is not defined as requested, let's say, for
///            instance 100, when current position is FFFFFFFF (4,294,967,295)
///            index in the circular array is 4,294,967,295 % 100 = 95. 
///            When that value is incremented it will be set to 0, that is the 
///            last 4 elements of the queue are not used when the counter rolls
///            over to 0
template <typename ELEM_T, uint32_t Q_SIZE = ARRAY_LOCK_FREE_Q_DEFAULT_SIZE>
class ArrayLockFreeQueue
{
public:
    /// @brief constructor of the class
    ArrayLockFreeQueue();
    virtual ~ArrayLockFreeQueue();

    /// @brief returns the current number of items in the queue
    /// It tries to take a snapshot of the size of the queue, but in busy environments
    /// this function might return bogus values. 
    ///
    /// If a reliable queue size must be kept you might want to have a look at 
    /// the preprocessor variable in this header file called 'ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE'
    /// it enables a reliable size though it hits overall performance of the queue 
    /// (when the reliable size variable is on it's got an impact of about 20% in time)
    uint32_t size();

    /// @brief push an element at the tail of the queue
    /// @param the element to insert in the queue
    /// Note that the element is not a pointer or a reference, so if you are using large data
    /// structures to be inserted in the queue you should think of instantiate the template
    /// of the queue as a pointer to that large structure
    /// @returns true if the element was inserted in the queue. False if the queue was full
    bool push(const ELEM_T &a_data);

    /// @brief pop the element at the head of the queue
    /// @param a reference where the element in the head of the queue will be saved to
    /// Note that the a_data parameter might contain rubbish if the function returns false
    /// @returns true if the element was successfully extracted from the queue. False if the queue was empty
    bool pop(ELEM_T &a_data);

private:
    /// @brief array to keep the elements
    ELEM_T m_theQueue[Q_SIZE];

#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    /// @brief number of elements in the queue
    volatile uint32_t m_count;
#endif

    /// @brief where a new element will be inserted
    volatile uint32_t m_writeIndex;

    /// @brief where the next element where be extracted from
    volatile uint32_t m_readIndex;

    /// @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but  
    /// data is still not in the queue, so the thread trying to read will have 
    /// to wait for those other threads to save the data into the queue
    ///
    /// note this index is only used for MultipleProducerThread queues
    volatile uint32_t m_maximumReadIndex;

    /// @brief calculate the index in the circular array that corresponds
    /// to a particular "count" value
    inline uint32_t countToIndex(uint32_t a_count);
};

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::ArrayLockFreeQueue()
    : m_writeIndex(0),
      m_readIndex(0),
      m_maximumReadIndex(0) // only for MultipleProducerThread queues
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
      , m_count(0)
#endif
{
}

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueue<ELEM_T, Q_SIZE>::~ArrayLockFreeQueue()
{
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline uint32_t ArrayLockFreeQueue<ELEM_T, Q_SIZE>::countToIndex(uint32_t a_count)
{
    // if Q_SIZE is a power of 2 this statement could be also written as 
    // return (a_count & (Q_SIZE - 1));
    return (a_count % Q_SIZE);
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t ArrayLockFreeQueue<ELEM_T, Q_SIZE>::size()
{
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    return m_count;
#else
    uint32_t currentWriteIndex = m_writeIndex;
    uint32_t currentReadIndex = m_readIndex;

    // let's think of a scenario where this function returns bogus data
    // 1. when the statement 'currentWriteIndex = m_writeIndex' is run
    // m_writeIndex is 3 and m_readIndex is 2. Real size is 1
    // 2. afterwards this thread is preemted. While this thread is inactive 2 
    // elements are inserted and removed from the queue, so m_writeIndex is 5
    // m_readIndex 4. Real size is still 1
    // 3. Now the current thread comes back from preemption and reads m_readIndex.
    // currentReadIndex is 4
    // 4. currentReadIndex is bigger than currentWriteIndex, so
    // m_totalSize + currentWriteIndex - currentReadIndex is returned, that is,
    // it returns that the queue is almost full, when it is almost empty

    if (currentWriteIndex >= currentReadIndex)
    {
        return (currentWriteIndex - currentReadIndex);
    }
    else
    {
        return (Q_SIZE + currentWriteIndex - currentReadIndex);
    }
#endif // ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;

    do
    {
        currentWriteIndex = m_writeIndex;
        currentReadIndex = m_readIndex;
        if (countToIndex(currentWriteIndex + 1) ==
            countToIndex(currentReadIndex))
        {
            // the queue is full
            return false;
        }

    } while (!CAS(&m_writeIndex, currentWriteIndex, (currentWriteIndex + 1)));

    // We know now that this index is reserved for us. Use it to save the data
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;

    // update the maximum read index after saving the data. It wouldn't fail if there is only one thread 
    // inserting in the queue. It might fail if there are more than 1 producer threads because this
    // operation has to be done in the same order as the previous CAS
    while (!CAS(&m_maximumReadIndex, currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        sched_yield();
    }

    // The value was successfully inserted into the queue
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    AtomicAdd(&m_count, 1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueue<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // to ensure thread-safety when there is more than 1 producer thread
        // a second index is defined (m_maximumReadIndex)
        currentReadIndex = m_readIndex;
        currentMaximumReadIndex = m_maximumReadIndex;

        if (countToIndex(currentReadIndex) ==
            countToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            return false;
        }

        // retrieve the data from the queue
        //a_data = m_theQueue[countToIndex(currentReadIndex)];
        std::swap(a_data, m_theQueue[countToIndex(currentReadIndex)]);

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (CAS(&m_readIndex, currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
            AtomicSub(&m_count, 1);
#endif
            return true;
        }

        // it failed retrieving the element off the queue. Someone else must
        // have read the element stored at countToIndex(currentReadIndex)
        // before we could perform the CAS operation        

    } while (1); // keep looping to try again!

    // Something went wrong. it shouldn't be possible to reach here
    assert(0);

    // Add this return statement to avoid compiler warnings
    return false;
}

/// @brief especialisation of the ArrayLockFreeQueue to be used when there is
///        only one producer thread
/// No allocation of extra memory for the nodes handling is needed
/// WARNING: This queue is not thread safe when several threads try to insert elements
/// into the queue
/// ELEM_T     represents the type of elementes pushed and popped from the queue
/// TOTAL_SIZE size of the queue. It should be a power of 2 to ensure 
///            indexes in the circular queue keep stable when the uint32_t 
///            variable that holds the current position rolls over from FFFFFFFF
///            to 0. For instance
///            2    -> 0x02 
///            4    -> 0x04
///            8    -> 0x08
///            16   -> 0x10
///            (...) 
///            1024 -> 0x400
///            2048 -> 0x800
///
///            if queue size is not defined as requested, let's say, for
///            instance 100, when current position is FFFFFFFF (4,294,967,295)
///            index in the circular array is 4,294,967,295 % 100 = 95. 
///            When that value is incremented it will be set to 0, that is the 
///            last 4 elements of the queue are not used when the counter rolls
///            over to 0
template <typename ELEM_T, uint32_t Q_SIZE = ARRAY_LOCK_FREE_Q_DEFAULT_SIZE>
class ArrayLockFreeQueueSingleProducer
{
public:
    /// @brief constructor of the class
    ArrayLockFreeQueueSingleProducer();
    virtual ~ArrayLockFreeQueueSingleProducer();

    /// @brief returns the current number of items in the queue
    /// It tries to take a snapshot of the size of the queue, but in busy environments
    /// this function might return bogus values. 
    ///
    /// If a reliable queue size must be kept you might want to have a look at 
    /// the preprocessor variable in this header file called 'ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE'
    /// it enables a reliable size though it hits overall performance of the queue 
    /// (when the reliable size variable is on it's got an impact of about 20% in time)
    uint32_t size();

    /// @brief push an element at the tail of the queue
    /// @param the element to insert in the queue
    /// Note that the element is not a pointer or a reference, so if you are using large data
    /// structures to be inserted in the queue you should think of instantiate the template
    /// of the queue as a pointer to that large structure
    /// @returns true if the element was inserted in the queue. False if the queue was full
    bool push(const ELEM_T &a_data);

    /// @brief pop the element at the head of the queue
    /// @param a reference where the element in the head of the queue will be saved to
    /// Note that the a_data parameter might contain rubbish if the function returns false
    /// @returns true if the element was successfully extracted from the queue. False if the queue was empty
    bool pop(ELEM_T &a_data);

private:
    /// @brief array to keep the elements
    ELEM_T m_theQueue[Q_SIZE];

#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    /// @brief number of elements in the queue
    volatile uint32_t m_count;
#endif

    /// @brief where a new element will be inserted
    volatile uint32_t m_writeIndex;

    /// @brief where the next element where be extracted from
    volatile uint32_t m_readIndex;

    /// @brief calculate the index in the circular array that corresponds
    /// to a particular "count" value
    inline uint32_t countToIndex(uint32_t a_count);
};

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::ArrayLockFreeQueueSingleProducer()
    : m_writeIndex(0),
      m_readIndex(0)
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
      , m_count(0)
#endif
{
}

template <typename ELEM_T, uint32_t Q_SIZE>
ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::~ArrayLockFreeQueueSingleProducer()
{
}

template <typename ELEM_T, uint32_t Q_SIZE>
inline uint32_t ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::countToIndex(uint32_t a_count)
{
    // if Q_SIZE is a power of 2 this statement could be also written as 
    // return (a_count & (Q_SIZE - 1));
    return (a_count % Q_SIZE);
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::size()
{
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    return m_count;
#else
    uint32_t currentWriteIndex = m_writeIndex;
    uint32_t currentReadIndex = m_readIndex;

    // let's think of a scenario where this function returns bogus data
    // 1. when the statement 'currentWriteIndex = m_writeIndex' is run
    // m_writeIndex is 3 and m_readIndex is 2. Real size is 1
    // 2. afterwards this thread is preemted. While this thread is inactive 2 
    // elements are inserted and removed from the queue, so m_writeIndex is 5
    // m_readIndex 4. Real size is still 1
    // 3. Now the current thread comes back from preemption and reads m_readIndex.
    // currentReadIndex is 4
    // 4. currentReadIndex is bigger than currentWriteIndex, so
    // m_totalSize + currentWriteIndex - currentReadIndex is returned, that is,
    // it returns that the queue is almost full, when it is almost empty

    if (currentWriteIndex >= currentReadIndex)
    {
        return (currentWriteIndex - currentReadIndex);
    }
    else
    {
        return (Q_SIZE + currentWriteIndex - currentReadIndex);
    }
#endif // ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::push(const ELEM_T &a_data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;

    currentWriteIndex = m_writeIndex;
    currentReadIndex = m_readIndex;
    if (countToIndex(currentWriteIndex + 1) ==
        countToIndex(currentReadIndex))
    {
        // the queue is full
        return false;
    }

    // save the date into the q
    m_theQueue[countToIndex(currentWriteIndex)] = a_data;

    // No need to increment write index atomically. It is a 
    // a requierement of this queue that only one thred can push stuff in
    m_writeIndex++;

    // The value was successfully inserted into the queue
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
    AtomicAdd(&m_count, 1);
#endif

    return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool ArrayLockFreeQueueSingleProducer<ELEM_T, Q_SIZE>::pop(ELEM_T &a_data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // m_maximumReadIndex doesn't exist when the queue is set up as
        // single-producer. The maximum read index is described by the current
        // write index
        currentReadIndex = m_readIndex;
        currentMaximumReadIndex = m_writeIndex;

        if (countToIndex(currentReadIndex) ==
            countToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            return false;
        }

        // retrieve the data from the queue
        //a_data = m_theQueue[countToIndex(currentReadIndex)];
        std::swap(a_data, m_theQueue[countToIndex(currentReadIndex)]);

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (CAS(&m_readIndex, currentReadIndex, (currentReadIndex + 1)))
        {
            // got here. The value was retrieved from the queue. Note that the
            // data inside the m_queue array is not deleted nor reseted
#ifdef ARRAY_LOCK_FREE_Q_KEEP_REAL_SIZE
            AtomicSub(&m_count, 1);
#endif
            return true;
        }

        // it failed retrieving the element off the queue. Someone else must
        // have read the element stored at countToIndex(currentReadIndex)
        // before we could perform the CAS operation        

    } while (1); // keep looping to try again!

    // Something went wrong. it shouldn't be possible to reach here
    assert(0);

    // Add this return statement to avoid compiler warnings
    return false;
}

} // namespace lockfree
} // namespace base

#endif // BASE_LOCKFREE_H
