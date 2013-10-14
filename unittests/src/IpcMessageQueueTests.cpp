#include "Tests.h"
#include "IpcMessageQueue.h"

DEF_TEST(IpcMessageQueue, SendRecvMessage)
{
	try {
		IpcMessageQueuePtr host = IpcMessageQueue::Create("test", true);
		IpcMessageQueuePtr client = IpcMessageQueue::Create("test", false);

		bool ret = host->WriteMessage("test", "hello");
		TEST_ASSERT(ret, "unexpected timeout when writing message");

		std::string type, msg;
		ret = client->ReadMessage(type, msg);
		TEST_ASSERT(ret, "unexpected timeout when reading message");

		TEST_ASSERT(type == "test", "unexpected type, expected 'test' but got: '" << type << "'");
		TEST_ASSERT(msg == "hello", "unexpected message contents, expected 'hello' but got: '" << msg << "'");

	} catch (IpcEx ex) {
		TEST_EX("IpcEx, " << ex.GetMsg());
	}
	
}
