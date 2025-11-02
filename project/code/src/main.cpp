#include "MattDaemon.hpp"

int	main()
{
	if (getuid() != 0)
	{
		std::cerr << "This program must be run as root." << std::endl;
		return (EXIT_FAILURE);
	}
	std::cout << "MattDaemon initialized." << std::endl;
	return (EXIT_SUCCESS);
}
