#include "RandChar.h"
#include <cstdlib>
#include <ctime>

class CRandChar : public RandChar {
	public:
	static bool hasInit;
	CRandChar(){
		if(!hasInit)
			srand(time(0));
		hasInit = true;
	}

	void Generate(char* buffer, int length){
		for(int i = 0; i < length; i++)
			buffer[i] = rand();
	}
};

bool CRandChar::hasInit = false;

RandChar* RandChar::Create()
{
	return new CRandChar();
}
