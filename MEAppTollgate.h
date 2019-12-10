#pragma once

#include <iostream>
#include <stdio.h>
#include <string.h>	//strlen
#include <stdlib.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <netinet/tcp.h>
#include <unistd.h>	//write
#include <pthread.h>	//for threading , link with lpthread
#include <thread>
#include <map>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <sstream>
#include <random>
#include <byteswap.h>

#include "SS_API.h"

using namespace std;

#define MAX_LINE 8192
#define CONNECTION_PORT 32002
#define SIMREQUEST_PORT 32003

typedef struct {
	int simCount;
	int simNumber;
	int footer;
	int threadCount;
}HANDLER_PARAM;


class CMEAppTollgate {

public:
	CMEAppTollgate();
	~CMEAppTollgate();

	void StartServer(int simCount = 100, int simPerPod = 20);

private:

	typedef thread(*callback_function)(int);
	callback_function connectionFunc, simRequestFunc;

	void InitEnvironment(int simCount, int simPerPod);
	int SocketBinding(int _port);
	int SocketAccepting(int sockfd, int port, callback_function pFunc);

	int SocketConnect(const char* ip_addr, int port);

	thread AcceptThread(int sockfd, int port, callback_function pFunc);
	void accept_handler(int sockfd, int port, callback_function pFunc);

	static thread ConnectionThread(int sockfd);
	static void connection_handler(int sockfd);
	static thread SimulationThread(int sockfd);
	static void simulation_handler(int sockfd);

	static int sendfile(FILE *fp, int sockfd);

	static const char *my_itoa_buf(char *buf, size_t len, int num);
	static const char *my_itoa(int num);

	static pthread_mutex_t m_requestMutex;
	static pthread_mutex_t m_threadMutex;

	static int m_sessionCount;

	static int m_tmpSessionCount;

	static int m_simCount;
	static int m_simPerPod;


	static char **str_split(char* a_str, const char a_delim);
	static int IsDouble(const char *str);

	static int calcTargetTCS(int direction, int payType, int session);
	static int findTCS(int type, int _start, int _end, int session);

	static map<int, vector<int>> m_procInfo;

};