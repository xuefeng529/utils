#ifndef BASE_MEMORYPOOL_H
#define BASE_MEMORYPOOL_H

#include <boost/noncopyable.hpp>
#include <iostream>

#include <assert.h>
#include <stddef.h>

namespace base
{

/// 单线程内存池
//template<class T>
//class CachedObj
//{
//public:
//	virtual ~CachedObj() {};
//
//	inline void* operator new(size_t size)
//	{
//		if (size != sizeof(T))
//		{
//			throw std::runtime_error("CachedObj: wrong size object in operator new");
//		}
//
//		if (theHead == NULL)
//		{
//			T* array = theMemAllocator.allocate(kExpansionSize);
//			for (size_t i = 0; i != kExpansionSize; ++i)
//			{
//				addToFreeList(&array[i]);
//			}
//		}
//
//		T* p = theHead;
//		theHead = theHead->CachedObj<T>::next_;
//		return p;
//	}
//
//	inline void operator delete(void* p, size_t size)
//	{
//		if (p != NULL)
//		{
//			addToFreeList(reinterpret_cast<T*>(p));
//		}
//	}
//
//protected:
//	T* next_;
//
//private:
//	static void addToFreeList(T* p)
//	{
//		p->CachedObj<T>::next_ = theHead;
//		theHead = p;
//	}
//
//	static const size_t kExpansionSize;
//	static std::allocator<T> theMemAllocator;
//	static T* theHead;
//	
//};
//
//template<class T>
//const size_t CachedObj<T>::kExpansionSize = 32;
//
//template<class T>
//std::allocator<T> CachedObj<T>::theMemAllocator;
//
//template<class T>
//T* CachedObj<T>::theHead = NULL;

/// 单线程内存池
template<size_t N>
class MemoryPool : boost::noncopyable
{
public:
	struct Entry
	{
		Entry* next;		
	};

	explicit MemoryPool()
		: chunkSize_(N > sizeof(Entry) ? N : sizeof(Entry)),
		  freeList_(NULL)
	{
		expandFreeList();
	}

	~MemoryPool()
	{
		Entry* head = freeList_;
		for (; head != NULL; head = freeList_)
		{
			freeList_ = freeList_->next;
			/// 不调用Entry析构函数
			operator delete(head);
		}
	}

	void* malloc()
	{
		if (freeList_ == NULL)
		{
			expandFreeList();
		}

		void* head = freeList_;
		freeList_ = freeList_->next;
		return head;
	}

	void free(void* chunk)
	{
		if (chunk != NULL)
		{
			Entry* head = reinterpret_cast<Entry*>(chunk);
			head->next = freeList_;
			freeList_ = head;
		}
	}

private:
	void expandFreeList()
	{
		assert(freeList_ == NULL);
		for (size_t i = 0; i < kExpansionSize; i++)
		{
			Entry* chunk = reinterpret_cast<Entry*>(new char[chunkSize_]);
			chunk->next = freeList_;
			freeList_ = chunk;
		}
	}

	const size_t chunkSize_;
	Entry* freeList_;

	static const size_t kExpansionSize;
};

template<size_t N>
const size_t MemoryPool<N>::kExpansionSize = 32;

/// 多线程内存池
template<class Lock, size_t N>
class MTMemoryPool : MemoryPool<N>
{
public:
	inline void* malloc()
	{
		lock.lock();
		void* chunk = MemoryPool<N>::malloc();
		lock.unlock();
		return chunk;
	}

	inline void free(void* chunk)
	{
		lock.lock();
		MemoryPool<N>::free(chunk);
		lock.unlock();
	}

private:
	Lock lock;
};

/// 单线程对象池
template<class T>
class ObjectPool
{
public:
	static inline T* malloc()
	{
		if (theFreeList == NULL)
		{
			expandFreeList();
		}

		T* head = reinterpret_cast<T*>(theFreeList);
		theFreeList = theFreeList->next_;
		return head;
	}

	static inline T* malloc(size_t elements)
	{
		if (theFreeList == NULL)
		{
			expandFreeList();
		}

		T* head = reinterpret_cast<T*>(theFreeList);
		theFreeList = theFreeList->next_;
		return head;
	}

	static inline void free(T* element)
	{
		if (element != NULL)
		{
			ObjectPool<T>* head = reinterpret_cast<ObjectPool<T>*>(element);
			head->next_ = theFreeList;
			theFreeList = head;
		}
	}

	static void release()
	{
		ObjectPool<T>* head = theFreeList;
		for (; head != NULL; head = theFreeList)
		{			
			theFreeList = theFreeList->next_;
			/// 不调用ObjectPool<T>析构函数
			operator delete(head);
		}
	}

private:
	static void expandFreeList()
	{
		assert(theFreeList == NULL);
		size_t size = sizeof(T) > sizeof(ObjectPool<T>) ? sizeof(T) : sizeof(ObjectPool<T>);
		for (size_t i = 0; i < kExpansionSize; i++)
		{
			ObjectPool<T>* chunk = reinterpret_cast<ObjectPool<T>*>(new char[size]);
			chunk->next_ = theFreeList;
			theFreeList = chunk;
		}
	}

	ObjectPool<T>* next_;

	static const size_t kExpansionSize;
	static ObjectPool<T>* theFreeList;
};

template<class T>
const size_t ObjectPool<T>::kExpansionSize = 32;

template<class T>
ObjectPool<T>* ObjectPool<T>::theFreeList = NULL;

/// 多线程对象池
template<class Element, class Lock>
class MTObjectPool : public ObjectPool<Element>
{
public:
	static inline Element* malloc()
	{
		theLock.lock();
		Element* element = ObjectPool<Element>::malloc();
		theLock.unlock();
		return element;
	}

	static inline void free(Element* element)
	{
		theLock.lock();
		ObjectPool<Element>::free(element);
		theLock.unlock();
	}

private:
	static Lock theLock;
};

template<class Element, class Lock>
Lock MTObjectPool<Element, Lock>::theLock;

} // namespace base

#endif // BASE_MEMORYPOOL_H
