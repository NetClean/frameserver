#ifndef IPC_H
#define IPC_H

#include <memory>
#include "Macros.h"

class Ipc;
typedef std::shared_ptr<Ipc> IpcPtr;

DefEx(IpcEx);

class Ipc
{
	public:
	virtual bool ReadMessage(std::string& type, std::string& message, int timeout = -1) = 0;
	virtual bool WriteMessage(std::string type, std::string message, int timeout = -1) = 0;
	
	virtual bool GetReadBuffer(char* out_type, const char** out_buffer, size_t* out_size, int timeout = -1) = 0;
	virtual void ReturnReadBuffer(const char** buffer) = 0;

	virtual char* GetWriteBuffer(int timeout = -1) = 0;
	virtual void ReturnWriteBuffer(std::string type, char** buffer, int len) = 0;
	
	static IpcPtr Create(std::string name, bool isHost = false);
};

#endif
