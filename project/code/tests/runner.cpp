#include "MattDaemon.hpp"

// Runner
void Runner::expectTrue(const std::string& name, bool ok) {
    ++assertions;
    print_result(name, "true", ok ? "true" : "false", ok);
    if (!ok) { ++failures; failingList += "\n - " + name; }
}

void Runner::expectEq(const std::string& name, const std::string& expected, const std::string& obtained, bool ok) {
    ++assertions;
    print_result(name, expected, ok ? expected : ("false (\"" + obtained + "\")"), ok);
    if (!ok) { ++failures; failingList += "\n - " + name; }
}

void Runner::expectContains(const std::string& name, const std::string& haystack, const std::string& needle) {
    const bool ok = (haystack.find(needle) != std::string::npos);
    ++assertions;
    print_result(name, "true", ok ? "true" : ("false (\"" + haystack + "\")"), ok);
    if (!ok) { ++failures; failingList += "\n - " + name; }
}
