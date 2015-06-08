#include "Tests.h"
#include "Tools.h"

DEF_TEST(Tools, Split)
{
	StrVec in = {"", ":", ",:,", "hello:my:name:is"};
	std::vector<StrVec> out = {
		{},
		{"",""},
		{",", ","},
		{"hello", "my", "name", "is"},
	};

	int i = 0;
	for(std::string sin : in)
	{
		StrVec o = Tools::Split(sin, ':');

		TEST_ASSERT(o.size() == out[i].size(), "for input: '" << sin << "' expected size: " << out[i].size() << " but was: " << o.size());

		int j = 0;
		for(auto s : o){
			TEST_ASSERT(s == out[i][j], "for input: '" << sin << "' expected element: " << j << " to be: \"" << out[i][j] << "\" but was: \"" << s << "\"");
			j++;
		}

		i++;
	}
}

DEF_TEST(Tools, SplitLimit)
{
	StrVec in = {"", ":", ",:,", "hello:my:name:is", "::", "b:"};
	std::vector<StrVec> out = {
		{},
		{"",""},
		{",", ","},
		{"hello", "my:name:is"},
		{"", ":"},
		{"b", ""},
	};

	int i = 0;
	for(std::string sin : in)
	{
		StrVec o = Tools::Split(sin, ':', 1);

		TEST_ASSERT(o.size() == out[i].size(), "for input: '" << sin << "' expected size: " << out[i].size() << " but was: " << o.size());

		int j = 0;
		for(auto s : o){
			TEST_ASSERT(s == out[i][j], "for input: '" << sin << "' expected element: " << j << " to be: \"" << out[i][j] << "\" but was: \"" << s << "\"");
			j++;
		}

		i++;
	}
}
