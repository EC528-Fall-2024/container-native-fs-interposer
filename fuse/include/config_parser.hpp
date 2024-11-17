#ifndef CONFIG_PARSER_HPP_INCLUDED
#define CONFIG_PARSER_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

json getConfig(std::string configPath);


#endif // CONFIG_PARSER_HPP_INCLUDED
