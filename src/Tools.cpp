#include "Tools.h"

#include <algorithm>

#include "flog.h"

int Tools::CstrListToMap(const char* const* cstr, char delim, StrStrMap& map)
{
	int ret = 0;

	while (*(cstr)){
		std::string str(*cstr);
		size_t pos = str.find(delim);

		if(pos == std::string::npos)
			throw MalformedList(Str("no delimiter found in: '" << *cstr << "'"));
		
		if(pos == 0)
			throw MalformedList("no key (delimiter is first character)");

		map[str.substr(0, pos)] = pos < str.size() - 1 ? Trim(str.substr(pos + 1)) : ""; 
		
		cstr++;
		ret++;
	}
		
	return ret;
}

bool Tools::StartsWith(const std::string& haystack, const std::string& needle)
{
	if(needle.size() > haystack.size())
		return false;
	
	return haystack.substr(0, needle.size()) == needle;
}

int Tools::ParseCookies(const std::string& str, StrStrMap& map)
{
	StrVec v = Split(str, ';');
	int ret = 0;
	for(auto& s : v){
		size_t pos = s.find('=');

		if(pos == std::string::npos)
			throw ParseCookieEx(Str("no = found in: '" << s << "'"));
		
		if(pos == 0)
			throw ParseCookieEx("no key (= is first character)");

		map[Trim(s.substr(0, pos))] = pos < s.size() - 1 ? Trim(RTrim(s.substr(pos + 1))) : ""; 
		ret++;
	}
	return ret;
}

StrVec Tools::Split(const std::string& s, char delim)
{
	std::vector<std::string> ret;

	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim)) {
		ret.push_back(item);
	}

	return ret;
}

std::string Tools::Join(const StrVec& strs, const std::string delim)
{
	std::stringstream ret;

	bool first = true;
	for(auto s : strs){
		if(!first)
			ret << delim;
		ret << s;
		first = false;
	}

	return ret.str();
}

std::string Tools::CombinePath(const StrVec& strs, char delim)
{
	std::stringstream ret;

	bool first = true;
	for(auto s : strs){
		if(!first)
			ret << delim;
		ret << Trim(s, Str(delim));
		first = false;
	}

	return ret.str();
}

std::string Tools::RTrim(std::string s, const std::string& trimChars)
{
	auto l = [&](char c){ return trimChars.find(c) == std::string::npos; };
	s.erase(std::find_if(s.rbegin(), s.rend(), l).base(), s.end());
	return s;
}

std::string Tools::LTrim(std::string s, const std::string& trimChars)
{
	auto l = [&](char c){ return trimChars.find(c) == std::string::npos; };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), l));
        return s;
}

std::string Tools::Trim(const std::string& s, const std::string& trimChars)
{
	return LTrim(RTrim(s, trimChars), trimChars);
}
