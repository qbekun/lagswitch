#pragma once
#include <filesystem> 
#include <string> 
#include <fstream>
#include "skStr.h"
#include "json.hpp"
using json = nlohmann::json;


void checkAuthenticated(std::string ownerid) {
	while (true) {
		if (GlobalFindAtomA(ownerid.c_str()) == 0) {
			exit(13);
		}
		Sleep(1000); // thread interval
	}
}
