#include "MattDaemon.hpp"
#include "Daemon.hpp"

int	main()
{
	if (getuid() != 0)
	{
		std::cerr << "This program must be run as root." << std::endl;
		return (EXIT_FAILURE);
	}

	Daemon	matt_daemon;;
    return matt_daemon.start();
}
