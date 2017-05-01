#pragma once

template<typename T>
class ResizableArray
{
public:
	ResizableArray(size_t capacity = 16) : capacity(capacity), size(0)
	{
		data = new T[capacity];
	}
	ResizableArray(const ResizableArray& a) = delete; //No copy. Implement if needed.
	ResizableArray(const ResizableArray&& a) = delete; //No move
	~ResizableArray()
	{
		delete data;
	}
	void SetCapacity(size_t newCapacity)
	{
			capacity = newCapacity;
			delete data;
			data = new T[capacity];
	};

	T* data;
	size_t capacity;
	size_t size;
};