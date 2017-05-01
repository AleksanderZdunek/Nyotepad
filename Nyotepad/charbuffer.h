#pragma once
#include <string>

//#ifndef min
//#define min(a,b)            (((a) < (b)) ? (a) : (b))
//#endif
//#ifndef max
//#define max(a,b)            (((a) > (b)) ? (a) : (b))
//#endif

template<typename T>
class CharBuffer
{
public:
	CharBuffer(const T s[]) : str(s), cursorPos(0) {}

	void Insert(T c);
	void Erase();
	void EraseRange(size_t pos, size_t length);
	void pop_back();
	void push_back(T c);
	const T* data() const;
	size_t size() const;
	size_t length() const;
	bool empty() const;
	void clear();

	size_t GetCursorPos() const;
	void SetCursorPos(size_t pos);
	void IncrementCursor();
	void DecrementCursor();

protected:
	std::basic_string<T> str;
	size_t cursorPos;
};

template<typename T>
void CharBuffer<T>::Insert(T c)
{
	str.insert(str.begin()+cursorPos, c);
	IncrementCursor();
}

template<typename T>
void CharBuffer<T>::Erase()
{
	if( 0 < cursorPos ) //do nothing if cursor is at beginning of string
	{
		str.erase(str.begin() + cursorPos - 1);
		DecrementCursor();
	}
}

template<typename T>
inline void CharBuffer<T>::EraseRange(const size_t pos, const size_t length)
{
	SetCursorPos(pos);
	str.erase(cursorPos, length);
}

template<typename T>
void CharBuffer<T>::pop_back()
{
	if(!str.empty()) str.pop_back();
}

template<typename T>
void CharBuffer<T>::push_back(T c)
{
	str.push_back(c);
}

template<typename T>
const T * CharBuffer<T>::data() const
{
	return str.data();
}

template<typename T>
size_t CharBuffer<T>::size() const
{
	return str.size();
}

template<typename T>
size_t CharBuffer<T>::length() const
{
	return str.length();
}

template<typename T>
bool CharBuffer<T>::empty() const
{
	return str.empty();
}

template<typename T>
void CharBuffer<T>::clear()
{
	str.clear();
	SetCursorPos(0);
}

template<typename T>
size_t CharBuffer<T>::GetCursorPos() const
{
	return cursorPos;
}

template<typename T>
void CharBuffer<T>::SetCursorPos(size_t pos)
{
	cursorPos = min(pos, str.size());
}

template<typename T>
void CharBuffer<T>::IncrementCursor()
{
	if( cursorPos < str.length() ) cursorPos++;
}

template<typename T>
void CharBuffer<T>::DecrementCursor()
{
	if( 0 != cursorPos ) cursorPos--;
}

//~~~

template<typename T>
class TextBuffer : public CharBuffer<T>
{
public:
	TextBuffer(const T s[]) : CharBuffer(s), selectionInitMarker(0) {};

	void clear()
	{
		CharBuffer::clear();
		InitializeSelection();
	};

	void InitializeSelection()
	{
		selectionInitMarker = cursorPos;
	}

	void Insert(T c)
	{
		CharBuffer::Insert(c);
		InitializeSelection();
	};

	void Erase()
	{
		CharBuffer::Erase();
		InitializeSelection();
	};

	void EraseSelection()
	{
		CharBuffer::EraseRange(GetSelectionStartPosition(), GetSelectionLength());
		InitializeSelection();
	}

	size_t GetSelectionStartPosition()
	{
		return min(cursorPos, selectionInitMarker);
	}

	size_t GetSelectionLength()
	{
		return max(cursorPos, selectionInitMarker) - min(cursorPos, selectionInitMarker);
	}

private:
	size_t selectionInitMarker;

};

//~~~

typedef	TextBuffer<wchar_t> aizstring;
