#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <memory>

#include <memory>
#include "Macros.h"

class SharedMem;
typedef std::shared_ptr<SharedMem> SharedMemPtr;

DefEx(SharedMemEx);

class SharedMem {
	public:
	virtual size_t GetSize() = 0;
	virtual void* GetPtrRw() = 0;
	virtual const void* GetPtrRo() = 0;

	static SharedMemPtr Create(std::string name, size_t size = 4096, bool rw = true);
	static SharedMemPtr Open(std::string name, bool rw = false);
};

#endif
