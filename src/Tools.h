#ifndef TOOLS_H
#define TOOLS_H

#include <unordered_map>
#include <string>
#include <vector>

#include "Macros.h"

typedef std::unordered_map<std::string, std::string> StrStrMap;
typedef std::vector<std::string> StrVec;

DefEx(MalformedList);
DefEx(ParseCookieEx);

class Tools {
	public:
	static int CstrListToMap(const char* const* cstr, char delim, StrStrMap& map);
	static int ParseCookies(const std::string& str, StrStrMap& map);
	static std::string RTrim(std::string s, const std::string& trimChars = " \t");
	static std::string LTrim(std::string s, const std::string& trimChars = " \t");
	static std::string Trim(const std::string& s, const std::string& trimChars = " \t");
	static StrVec Split(const std::string& s, char delim = ' ');
	static std::string Join(const StrVec& strs, const std::string delim = " ");
	static std::string CombinePath(const StrVec& strs, char delim = '/');
	static bool StartsWith(const std::string& haystack, const std::string& needle);
};

#endif
