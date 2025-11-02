#include "MattDaemon.hpp"

std::string nowstr() {
	char buf[32];
	std::time_t t = std::time(NULL);
	std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&t));
	return buf;
}

void print_result(const std::string& name, const std::string& expected, const std::string& obtained, bool ok) {
	std::cout << GRY1 "[TEST] " << name << RST
			  << GRY2 "\nEXPECTED: " RST << expected
			  << GRY2 "\nOBTAINED: " RST << obtained
			  << "\nRESULT  : " << (ok ? LIME "PASS" : REDD "FAIL") << RST "\n\n";
}

void final_result(const std::string& testName, int failures, int total) {
	if (failures == 0) {
		std::cout << LIME "All " << total << " tests for passed!" RST "\n";
	} else {
		std::cout << REDD "Tests completed with " << failures << " failures out of " << total << " tests." RST "\n";
		std::cout << GOLD "Failing tests: " RST << testName << "\n";
	}
}
