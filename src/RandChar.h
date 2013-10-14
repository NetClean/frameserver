#ifndef RAND_CHAR_H
#define RAND_CHAR_H

class RandChar {
	public:
	virtual void Generate(char* buffer, int length) = 0;
	virtual ~RandChar(){};

	static RandChar* Create();
};

#endif
