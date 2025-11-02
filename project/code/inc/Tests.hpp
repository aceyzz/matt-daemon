#pragma once

int	test_FileOps();

// utils
std::string	nowstr();
void		print_result(const std::string& name, const std::string& expected, const std::string& obtained, bool ok);
void		final_result(const std::string& testName, int failures, int total);