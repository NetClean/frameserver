#ifndef UUIDTESTS_H
#define UUIDTESTS_H

#include "ITest.h"

DECL_TEST(IpcMessageQueue, SendRecvMessage);
DECL_TEST(SharedMem, ReadWrite);
DECL_TEST(Tools, Split);
DECL_TEST(Tools, SplitLimit);

#endif

