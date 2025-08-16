#pragma once

// utilities that are needed all over the different code parts

// #include "AAX.h"
#include "clap/clap.h"
#include <vector>
#include <string>

std::string createAAXId(clap_id id);
uint32_t AAXIDfromString(const char* str);
uint32_t AAXIDfromString(const std::string& str);
std::vector<std::string> generateShortStrings(const std::string& input);