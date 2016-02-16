#ifndef MACROS_H
#define MACROS_H

#include <string>
#include <sstream>

#ifdef DEBUG
#define IS_DEBUG_BUILD true
#else
#define IS_DEBUG_BUILD false
#endif

#define DefEx(__name) struct __name : ExBase { __name(std::string msg = "") { this->msg = msg; this->type = #__name; } };
#define AssertEx(__exp, __ex, __msg) if(!(__exp)){ throw \
	__ex(Str(__msg << " (" __FILE__":" << __LINE__ << ")")); }

#define Str(__what) [&]() -> std::string {std::stringstream __tmp; __tmp << __what; return __tmp.str(); }()

struct ExBase { std::string type; std::string msg; std::string GetMsg(){ return Str("(" << type << ") " << msg); } };

#endif
