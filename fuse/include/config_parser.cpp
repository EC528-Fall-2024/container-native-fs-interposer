#include "config_parser.hpp"

static json config = NULL;

json getConfig(std::string configPath) {
    if (config != NULL) {
        return config;
    }

    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "Could not open configuration file: "
                  << configPath
                  << std::endl;
        return NULL;
    }

    try {
       configFile >> config;
    } catch (std::exception e) {
        std::cerr << "Error parsing JSON." << std::endl;
        return NULL;
    }

    return config;
}
