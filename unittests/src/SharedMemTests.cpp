#include "Tests.h"
#include "SharedMem.h"
#include <cstring>

DEF_TEST(SharedMem, ReadWrite)
{
	SharedMemPtr write = SharedMem::Create("test", true, 4096);
	SharedMemPtr read = SharedMem::Open("test", false);

	strcpy((char*)write->GetPtrRw(), "hello");

	TEST_ASSERT(!strcmp((const char*)read->GetPtrRo(), "hello"), "unexpected contents of memory area");
}
