#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <vector>
#include <dirent.h>
#include <assert.h>

using namespace std;

#define MAX_LINE 8192
#define BUFFSIZE 8192
//#define SERVERPORT 30300
#define SERVERPORT 32000
//#define SERVERPORT 32004
//#define SIM_SERVICE_IP "10.101.0.1"
#define SIM_SERVICE_IP "172.30.100.2"
//#define SIM_SERVICE_IP "172.18.90.181"
//#define SIM_SERVICE_IP "172.18.90.180"
//#define SIM_SERVICE_IP "10.96.100.1"
//#define SIM_SERVICE_IP "10.100.52.5"

typedef struct SIMULATION_OPTION {

	int simulation_count_total;
	int simulation_count_per_pod;
	int is_source_included;

}SIM_OPT;

class SS_API
{
public:
	SS_API() {};
	~SS_API() {};

	static void SetConnectionInfo(char *IP, uint16_t port);

	static FILE* SimulationRequest(char *fileName, SIM_OPT simOpt, char *resPath = NULL);
	static FILE* SimulationRequest(char *fileName, char *dirPath, SIM_OPT simOpt, char *resPath = NULL);


private:

	static string m_IP;
	static uint16_t m_port;

	static int SocketConnection();
	static void SendSimulationModel(int sockfd, char* fileName, SIM_OPT simOpt, int source);
	static void SendSimulationFiles(int sockfd, char* dirPath);

	static FILE* ResultReceive(int sockfd, char *resPath);

	static int sendfile(FILE *fp, int sockfd);

	static char** str_split(char* a_str, const char a_delim);


};
