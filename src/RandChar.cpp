#include "RandChar.h"
#include "Platform.h"

class CRandChar : public RandChar {
	public:
	PlatformPtr p;
	CRandChar(PlatformPtr p) : p(p)
	{
	}

	void Generate(char* buffer, int length)
	{
		p->GetRandChars(buffer, length);
	}
};

RandChar* RandChar::Create(PlatformPtr p)
{
	return new CRandChar(p);
}
