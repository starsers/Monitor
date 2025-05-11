#ifndef CAMFACTORY_H
#define CAMFACTORY_H

#include "BaseCam.h"
#include <memory>

class CamFactory {
public:
    static std::unique_ptr<BaseCam> createCamera(const std::string& device_path = "");
};

#endif // CAMFACTORY_H