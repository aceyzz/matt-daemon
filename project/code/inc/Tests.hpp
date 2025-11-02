#pragma once

struct Runner {
    int failures = 0;
    int assertions = 0;
    std::string failingList;

    void expectTrue(const std::string& name, bool ok);
    void expectEq(const std::string& name, const std::string& expected, const std::string& obtained, bool ok);
    void expectContains(const std::string& name, const std::string& haystack, const std::string& needle);
};

int	test_FileOps();
int	test_TintinReporter();

// utils
std::string	nowstr();
void		print_result(const std::string& name, const std::string& expected, const std::string& obtained, bool ok);
void		final_result(const std::string& testName, int failures, int total);