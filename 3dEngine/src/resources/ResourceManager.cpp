#include <sstream>
#include "UrchinCommon.h"

#include "resources/ResourceManager.h"

namespace urchin
{

    ResourceManager::ResourceManager() :
            Singleton<ResourceManager>()
    {

    }

    ResourceManager::~ResourceManager()
    {
        if (!mResources.empty())
        {
            std::stringstream logStream;
            logStream<<"Resources not released:"<<std::endl;
            for (auto &mResource : mResources)
            {
                logStream<< " - " << mResource.second->getName() << std::endl;
            }
            Logger::logger().logError(logStream.str());
        }
    }

    void ResourceManager::addResource(const std::string &name, Resource *resource)
    {
        mResources[name] = resource;
        resource->setName(name);
    }

    void ResourceManager::removeResource(const std::string &name)
    {
        auto it = mResources.find(name);
        if(it!=mResources.end())
        {
            mResources.erase(it);
        }
    }

}
