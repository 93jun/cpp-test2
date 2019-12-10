
#include "MEAppTollgate.h"


int main(int argc, char **argv)
{
	CMEAppTollgate meapp;

	if (argc > 1) {
		meapp.StartServer(atoi(argv[1]), atoi(argv[2]));
	}
	else {
		meapp.StartServer();
	}

	return 0;
}