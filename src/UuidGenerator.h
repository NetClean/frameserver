#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <string>

class RandChar;

class UuidGenerator {
	public:
	virtual std::string GenerateUuid(RandChar* randChar) = 0;
	virtual ~UuidGenerator(){};

	static UuidGenerator* Create();
};

#endif
