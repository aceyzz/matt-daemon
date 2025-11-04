#include "MattDaemon.hpp"
#include "Daemon.hpp"

static int unit_tests()
{
	int	ret = 0;
	ret += test_FileOps();
	ret += test_TintinReporter();
	ret += test_Server();

	std::cout << "Unit tests completed with " << ret << " errors." << std::endl;
	return (ret > 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

int	main()
{
	if (getuid() != 0)
	{
		std::cerr << "This program must be run as root." << std::endl;
		return (EXIT_FAILURE);
	}

	if (TEST_MODE) return unit_tests();

	Daemon	matt_daemon;;
    return matt_daemon.start();
}
