#ifndef ITEST_H
#define ITEST_H

#include <string>
#include <flog.h>
#include "Macros.h"

struct TestFailedEx : std::exception {
	std::string section;
	std::string subsection;
	std::string msg;

	TestFailedEx(std::string section, std::string subsection, std::string msg) : section(section), subsection(subsection), msg(msg) {} 
	~TestFailedEx() throw() {}
};

class ITest {
	protected:

	public:
	virtual void RunTest() = 0;
	virtual std::string GetSection() = 0;
	virtual std::string GetSubsection() = 0;
};

#define TEST_EX(_msg) \
	throw TestFailedEx(GetSection(), GetSubsection(), Str(_msg));

#define TEST_ASSERT(_v, _msg) \
	if(!(_v)){ TEST_EX(_msg); };

#define TEST_ASSERT_THROWS(_what, _ex_type) \
	{\
		bool threw = false;\
		try { _what; } catch(_ex_type ex){ threw = true; }\
		TEST_ASSERT(threw, #_what << " did not throw exception " << #_ex_type);\
	}

#define TEST_NAME(_sec, _subsec) \
	Test ## _ ## _sec ## _ ## _subsec

#define DECL_TEST(_sec, _subsec) \
	class TEST_NAME(_sec, _subsec) : public ITest { void RunTest(); std::string GetSection(); std::string GetSubsection(); }

#define DEF_TEST(_sec, _subsec) \
	std::string TEST_NAME(_sec, _subsec)::GetSection(){ return #_sec ; } \
	std::string TEST_NAME(_sec, _subsec)::GetSubsection(){ return #_subsec ; } \
	void TEST_NAME(_sec, _subsec)::RunTest()

#endif
