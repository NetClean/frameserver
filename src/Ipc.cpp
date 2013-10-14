#include "Ipc.h"
#include "flog.h"

#include <libshmipc.h>
//#include <mutex>

class CIpc : public Ipc {
	shmipc* readQueue, *writeQueue;
	//std::mutex readLock, writeLock;

	public:
	CIpc(std::string name, bool isHost = false){
		std::string writeName = name;
		std::string readName = name;

		std::string* n = isHost ? &writeName : &readName;

		n->append("_host_writer");

		if(isHost){	
			auto err = shmipc_create(writeName.c_str(), 512, 32, SHMIPC_AM_WRITE, &writeQueue);
			AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "could not create writer");
			
			err = shmipc_create(readName.c_str(), 1920 * 1080 * 3, 32, SHMIPC_AM_READ, &readQueue);
			AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "could not create reader");
		}else{
			auto err = shmipc_open(writeName.c_str(), SHMIPC_AM_WRITE, &writeQueue);
			AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "could not open writer");

			err = shmipc_open(readName.c_str(), SHMIPC_AM_READ, &readQueue);
			AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "could not open writer");
		}
	}

	bool ReadMessage(std::string& type, std::string& message, int timeout) {
		char cType[SHMIPC_MESSAGE_TYPE_LENGTH];
		size_t length;
		const char* buffer;

		auto err =  shmipc_acquire_buffer_r(readQueue, cType, &buffer, &length, timeout);

		AssertEx(err == SHMIPC_ERR_SUCCESS || err == SHMIPC_ERR_TIMEOUT, IpcEx, "failed to acquire read buffer");

		if(err == SHMIPC_ERR_SUCCESS){
			message.assign(buffer, length);
			type.assign(cType);
			shmipc_return_buffer_r(readQueue, &buffer);
			return true;
		}

		return false;
	}

	bool WriteMessage(std::string type, std::string message, int timeout) {
		AssertEx(type.length() < SHMIPC_MESSAGE_TYPE_LENGTH, IpcEx, "type too long (" << type.length() << " / " << SHMIPC_MESSAGE_TYPE_LENGTH << ")"); 
		auto err = shmipc_send_message(writeQueue, type.c_str(), message.c_str(), message.length(), timeout);
		AssertEx(err == SHMIPC_ERR_SUCCESS || err == SHMIPC_ERR_TIMEOUT, IpcEx, "failed to send message");
		return (err == SHMIPC_ERR_SUCCESS);
	}

	bool GetReadBuffer(char* out_type, const char** out_buffer, size_t* out_size, int timeout){
		auto err = shmipc_acquire_buffer_r(readQueue, out_type, out_buffer, out_size, timeout);
		AssertEx(err == SHMIPC_ERR_SUCCESS || err == SHMIPC_ERR_TIMEOUT, IpcEx, "failed to acquire read buffer");
		return (err == SHMIPC_ERR_SUCCESS);
	}

	void ReturnReadBuffer(const char** buffer){
		auto err = shmipc_return_buffer_r(readQueue, buffer);
		AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "failed to return read buffer");
	}

	char* GetWriteBuffer(int timeout){
		char* ret = 0;
		auto err = shmipc_acquire_buffer_w(writeQueue, &ret, timeout);

		AssertEx(err == SHMIPC_ERR_SUCCESS || err == SHMIPC_ERR_TIMEOUT, IpcEx, "failed to acquire write buffer");
		
		return ret;
	}

	void ReturnWriteBuffer(std::string type, char** buffer, int len){
		AssertEx(type.length() < SHMIPC_MESSAGE_TYPE_LENGTH, IpcEx, "type too long (" << type.length() << " / " << SHMIPC_MESSAGE_TYPE_LENGTH << ")"); 
		auto err = shmipc_return_buffer_w(writeQueue, buffer, len, type.c_str());
		AssertEx(err == SHMIPC_ERR_SUCCESS, IpcEx, "failed to return read buffer");
	}

	~CIpc(){
		shmipc_destroy(&writeQueue);
		shmipc_destroy(&readQueue);
	}
};

IpcPtr Ipc::Create(std::string name, bool isHost)
{
	return IpcPtr(new CIpc(name, isHost));
}

#if 0

ReadBuffer IPC::GetReadBuffer(int timeout)
{
	WaitForSingleObject(readLock, INFINITE);

	ReadBuffer ret;
	MmapMessageHeader header;
	ret.data = Mmap_AcquireBufferR(readQueue, &header, timeout);

	if(ret.data){
		ret.type = header.type;
		ret.dataLen = header.length;
	}

	ReleaseMutex(readLock);

	return ret;
}

void IPC::ReturnReadBuffer(ReadBuffer buffer)
{
	if(buffer.data){
		Mmap_ReturnBufferR(readQueue, &buffer.data);
	}
}

bool IPC::WriteMessage(std::string type, std::string message, int timeout)
{
	WaitForSingleObject(writeLock, INFINITE);

	bool ret = false;
	char* buffer = Mmap_AcquireBufferW(writeQueue, timeout);

	if(buffer){
		memcpy(buffer, message.c_str(), message.length());
		Mmap_ReturnBufferW(writeQueue, &buffer, message.length(), type.c_str());
		ret = true;
	}

	ReleaseMutex(writeLock);
	return ret;
}

char* IPC::GetWriteBuffer(int timeout)
{
	return Mmap_AcquireBufferW(writeQueue, timeout);
}

void IPC::ReturnWriteBuffer(std::string type, char** buffer, int len)
{
	Mmap_ReturnBufferW(writeQueue, buffer, len, type.c_str());
}

#endif
