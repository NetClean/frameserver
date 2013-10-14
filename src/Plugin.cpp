#include "Plugin.h"

class CPlugin: public Plugin {
	public:
	CPlugin()
	{
	}
	
	void Run(){
	}
};

PluginPtr Plugin::Create(std::string executable)
{
	return PluginPtr(new CPlugin());
}
