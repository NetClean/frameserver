#include "UuidGenerator.h"
#include "RandChar.h"
#include <cstdint>

class CUuidGenerator : public UuidGenerator {
	public:
	std::string GenerateUuid(RandChar* randChar){
		uint8_t uuid[36];
		char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		
		randChar->Generate((char*)uuid, 36);

		// xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx

		for(int i = 0; i < (int)sizeof(uuid); i++){
			if(i == 8 || i == 13 || i == 18 || i == 23)
				uuid[i] = '-';
			else if(i == 14)
				uuid[i] = '4';
			else if(i == 19)
				uuid[i] = hex[8 + uuid[i] % 4];
			else
				uuid[i] = hex[uuid[i] % 16];
		}

		return std::string((char*)uuid, sizeof(uuid));
	}
};


UuidGenerator* UuidGenerator::Create()
{
	return new CUuidGenerator();
}
