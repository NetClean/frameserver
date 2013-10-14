#ifndef PLUGIN_H
#define PLUGIN_H

#include <memory>
#include <string>

class Plugin;
typedef std::shared_ptr<Plugin> PluginPtr; 

class Plugin
{
	public:
	virtual void Run() = 0;
	PluginPtr Create(std::string executable);
};

#endif
