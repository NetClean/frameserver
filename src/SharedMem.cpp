#include "SharedMem.h"
#include "libshmipc.h"
#include "flog.h"

class CSharedMem : public SharedMem {
	public:
	shmhandle* handle;
	size_t size;
	void* rwView;
	const void* roView;
	
	// open
	CSharedMem(std::string name, bool rw){
		rwView = 0;
		roView = 0;

		shmipc_error err;

		if(rw)
			err = shmipc_open_shm_rw(name.c_str(), &size, &rwView, &handle);
		else
			err = shmipc_open_shm_ro(name.c_str(), &size, &roView, &handle);
		
		AssertEx(err == SHMIPC_ERR_SUCCESS, SharedMemEx, "could not create shared memory area");
	}

	// create
	CSharedMem(std::string name, bool rw, size_t size){
		rwView = 0;
		roView = 0;

		shmipc_error err;

		if(rw)
			err = shmipc_create_shm_rw(name.c_str(), size, &rwView, &handle);
		else
			err = shmipc_create_shm_ro(name.c_str(), size, &roView, &handle);

		AssertEx(err == SHMIPC_ERR_SUCCESS, SharedMemEx, "could not create shared memory area");
	}

	~CSharedMem(){
		FlogD("destroying shm area");
		shmipc_destroy_shm(&handle);
	}

	size_t GetSize(){
		return 0;
	}
	
	const void* GetPtrRo(){
		return roView;
	}

	void* GetPtrRw(){
		return rwView;
	}
};

SharedMemPtr SharedMem::Create(std::string name, size_t size, bool rw)
{
	return SharedMemPtr(new CSharedMem(name, rw, size));
}

SharedMemPtr SharedMem::Open(std::string name, bool rw)
{
	return SharedMemPtr(new CSharedMem(name, rw));
}
