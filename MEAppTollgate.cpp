#include "MEAppTollgate.h"


pthread_mutex_t CMEAppTollgate::m_requestMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CMEAppTollgate::m_threadMutex = PTHREAD_MUTEX_INITIALIZER;
int CMEAppTollgate::m_sessionCount = -1;
int CMEAppTollgate::m_tmpSessionCount = -1;

int CMEAppTollgate::m_simCount = -1;
int CMEAppTollgate::m_simPerPod = -1;

map<int, vector<int>> CMEAppTollgate::m_procInfo;


CMEAppTollgate::CMEAppTollgate()
{
	//m_requestMutex = PTHREAD_MUTEX_INITIALIZER;
	//m_threadMutex = PTHREAD_MUTEX_INITIALIZER;
	//m_sessionCount = -1;
	connectionFunc = ConnectionThread;
	simRequestFunc = SimulationThread;
}

CMEAppTollgate::~CMEAppTollgate()
{

}

void CMEAppTollgate::StartServer(int simCount, int simPerPod)
{
	/* Initilize Environment */
	InitEnvironment(simCount, simPerPod);

	/* Socket creation, bind, listen */
	int connectionSockfd = SocketBinding(CONNECTION_PORT);
	int simRequestSockfd = SocketBinding(SIMREQUEST_PORT);

	/* Keep accepting client's request --> infinite blocking function */
	AcceptThread(connectionSockfd, CONNECTION_PORT, connectionFunc).detach();
	AcceptThread(simRequestSockfd, SIMREQUEST_PORT, simRequestFunc).join();
}

void CMEAppTollgate::InitEnvironment(int simCount, int simPerPod)
{
	/* Set Working Directory */
	chdir("/home/");

	/* Create Essential Directories */
	if (access("/home/Session", 0))
		system("mkdir Session");

	m_simCount = simCount;
	m_simPerPod = simPerPod;

	srand(time(NULL));
}

int CMEAppTollgate::SocketBinding(int port)
{
	int sockfd;
	struct sockaddr_in server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		printf("Could not create socket");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		exit(1);
	}
	listen(sockfd, 10);

	return sockfd;
}

int CMEAppTollgate::SocketAccepting(int sockfd, int port, callback_function pFunc)
{
	int client_sock, c;
	struct sockaddr_in client;

	/* Accept and incoming connection */
	//cout << "Waiting for incoming connections on port " << port << endl;
	std::string outMsg = "Waiting for incoming connections on port ";
	std::stringstream ss;
	ss << port;

	outMsg += ss.str();
	puts(outMsg.c_str());

	c = sizeof(struct sockaddr_in);
	pthread_t thread_id;

	/* Requests are handled by threads */
	while ((client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		callback_function func = pFunc;

		func(client_sock).detach();

	}
}

int CMEAppTollgate::SocketConnect(const char* ip_addr, int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("Can't allocate sockfd");
		exit(1);
	}

	//printf("portNum: %d \n", port_num);

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip_addr, &serveraddr.sin_addr) < 0)
	{
		perror("IPaddress Convert Error");
		exit(1);
	}

	if (connect(sockfd, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("Connect Error");
		exit(1);
	}

	return sockfd;
}

thread CMEAppTollgate::AcceptThread(int sockfd, int port, callback_function pFunc)
{
	return thread([=] {accept_handler(sockfd, port, pFunc); });
}

thread CMEAppTollgate::ConnectionThread(int sockfd)
{
	return thread([=] {connection_handler(sockfd); });
}

thread CMEAppTollgate::SimulationThread(int sockfd)
{
	return thread([=] {simulation_handler(sockfd); });
}

void CMEAppTollgate::accept_handler(int sockfd, int port, callback_function pFunc)
{
	SocketAccepting(sockfd, port, pFunc);
}

void CMEAppTollgate::connection_handler(int sockfd)
{
	// Alg
	// 1: Session initilize & increment

	int sock = sockfd;

	int sessionCount = 0;

	pthread_mutex_lock(&m_requestMutex);
	m_sessionCount++;
	sessionCount = m_sessionCount;
	pthread_mutex_unlock(&m_requestMutex);


	char dirPath[MAX_LINE] = "/home/Session/";
	strcat(dirPath, my_itoa(sessionCount));

	char dirCreate[100] = "mkdir /home/Session/";
	strcat(dirCreate, my_itoa(sessionCount));

	char dirDelete[100] = "rm -rf /home/Session/";
	strcat(dirDelete, my_itoa(sessionCount));


	/* Session Directory Create */
	if (access(dirPath, 0))
		system(dirCreate);
	else {
		system(dirDelete);
		system(dirCreate);
	}

	/* Copy Model Structure Files to the Session Directory */
	// Where are the essential files?

	char cpCmd[100] = "cp /home/files/* ";
	strcat(cpCmd, dirPath);
	system(cpCmd);


	// 2: Generate Random variables for model
	// Car Range? (50~150; int)
	// AV_Ratio? (0~10; int)
	// EP_Ratio? (0-10; int)

	int carNumber = rand() % 101 + 50;
	//int avRatio = rand() % 11;
	int avRatio = rand() % 9 + 1;
	//int epRatio = rand() % 11;
	int epRatio = rand() % 9 + 1;


	// 3: Revise ModelStructure xml based on the generated random variable
	char xmlPath[MAX_LINE] = "";
	strcpy(xmlPath, dirPath);
	strcat(xmlPath, "/ModelStructure.xml");


	char carCmd[100] = "MaxCustomers\\\"Value=\\\"";
	strcat(carCmd, my_itoa(carNumber));
	strcat(carCmd, "\\\"\\\\/\\>/g ");
	strcat(carCmd, xmlPath);

	char carCmd2[200] = "sed -i s/MaxCustomers.*/";
	strcat(carCmd2, carCmd);
	system(carCmd2);

	char avCmd[100] = "AutonomousVehicleRatio\\\"Value=\\\"";
	strcat(avCmd, my_itoa(avRatio));
	strcat(avCmd, "\\\"\\\\/\\>/g ");
	strcat(avCmd, xmlPath);

	char avCmd2[200] = "sed -i s/AutonomousVehicleRatio.*/";
	strcat(avCmd2, avCmd);
	system(avCmd2);

	char epCmd[100] = "ElectronicPaymentRatio\\\"Value=\\\"";
	strcat(epCmd, my_itoa(epRatio));
	strcat(epCmd, "\\\"\\\\/\\>/g ");
	strcat(epCmd, xmlPath);

	char epCmd2[200] = "sed -i s/ElectronicPaymentRatio.*/";
	strcat(epCmd2, epCmd);
	system(epCmd2);


	// 4: Send Model Info & Session Number
	// Model Info: 
	//  Processor Information (number, type)
	//  CarNumber
	//  avRatio
	//  epRatio
	//  Structure

	// Analyze Model by ModelStructure.xml

	FILE *cp;
	char tempLine[1024] = { 0 };
	char cp2[100] = "grep 'ProcType' ";
	strcat(cp2, xmlPath);

	cp = popen(cp2, "r");
	if (cp == NULL) {
		printf("Failed to run command\n");
		exit(1);
	}


	m_procInfo.insert(pair<int, vector<int>>(sessionCount, vector<int>()));

	//vector<int> procType;

	while (fgets(tempLine, sizeof(tempLine), cp) != NULL)
	{
		for (int i = 0; i < strlen(tempLine); i++) {
			if (isdigit(tempLine[i]))
				m_procInfo[sessionCount].push_back(tempLine[i] - '0');
			//procType.push_back(tempLine[i] - '0');
		}
	}

	if (pclose(cp) == -1)
	{
		printf("pclose error \n");
	}


	// Send File

	ofstream fp;
	char filePath[MAX_LINE] = "";
	strcpy(filePath, dirPath);
	strcat(filePath, "/ModelInfo.txt");

	fp.open(filePath, ofstream::out);
	fp << sessionCount << "\n";
	fp << carNumber << "\t" << avRatio << "\t" << epRatio << "\n"
		<< m_procInfo[sessionCount].size() << "\t";
	for (int i = 0; i < m_procInfo[sessionCount].size(); i++) {
		fp << m_procInfo[sessionCount][i] << "\t";
	}

	//fp << "aa\t" << 1<<"\t" << 2.5<<"\t" << 3 <<"\t"<< 4.9 <<"\t"<< 5<<"\t" << 6<<"\t" << 7 <<"\t"<< 8<<"\t" << 9;

	fp.close();

	int procSize = m_procInfo[sessionCount].size();

	if (send(sock, &sessionCount, sizeof(int), 0) == -1)
	{
		perror("Can't send sessionCount");
		exit(1);
	}

	if (send(sock, &carNumber, sizeof(int), 0) == -1)
	{
		perror("Can't send carNumber");
		exit(1);
	}

	if (send(sock, &avRatio, sizeof(int), 0) == -1)
	{
		perror("Can't send avRatio");
		exit(1);
	}

	if (send(sock, &epRatio, sizeof(int), 0) == -1)
	{
		perror("Can't send epRatio");
		exit(1);
	}

	if (send(sock, &procSize, sizeof(int), 0) == -1)
	{
		perror("Can't send procSize");
		exit(1);
	}

	usleep(10);

	for (int i = 0; i < procSize; i++) {

		int procType = m_procInfo[sessionCount][i];

		if (send(sock, &procType, sizeof(int), 0) == -1)
		{
			perror("Can't send procType");
			exit(1);
		}

	}

	//FILE *fp2 = fopen(filePath, "wb");

	//int sendRes = sendfile(fp2, sock);

	//printf("File send completed to Client App, %d\n", sendRes);

	//shutdown(sock, SHUT_WR);

	//fclose(fp2);

}
void CMEAppTollgate::simulation_handler(int sockfd)
{
	int sock = sockfd;


	// Alg
	// 1: Receive Session Number
/*
	int sessionCount = 0;
	int isAV = 0;
	int isEP = 0;
	int direction = 0;
*/
	int sessionCount = -1;
	int isAV = -1;
	int isEP = -1;
	int direction = -1;

	int tmpSC;
	int tmpAV;
	int tmpEP;
	int tmpDir;


	/* Receive SessionCount, isAV, isEP, direction */

	if (recv(sock, &tmpSC, sizeof(int), 0) == -1)
	{
		perror("Can't receive fileSize");
		exit(1);
	}

	if (recv(sock, &tmpAV, sizeof(int), 0) == -1)
	{
		perror("Can't receive fileSize");
		exit(1);
	}

	if (recv(sock, &tmpEP, sizeof(int), 0) == -1)
	{
		perror("Can't receive fileSize");
		exit(1);
	}

	if (recv(sock, &tmpDir, sizeof(int), 0) == -1)
	{
		perror("Can't receive fileSize");
		exit(1);
	}

	sessionCount = bswap_32(tmpSC);
	isAV = bswap_32(tmpAV);
	isEP = bswap_32(tmpEP);
	direction = bswap_32(tmpDir);


	cout << sessionCount << endl;
	cout << isAV << endl;
	cout << isEP << endl;
	cout << direction << endl;

	// 2: Retrieve Model based on the Session Number

	char dirPath[MAX_LINE] = "/home/Session/";
	strcat(dirPath, my_itoa(sessionCount));

	// 3: Request Simulation using API or something, to Simulation Service & Receive

	SIM_OPT simOpt;
	simOpt.simulation_count_total = m_simCount;
	simOpt.simulation_count_per_pod = m_simPerPod;

	char xmlPath[MAX_LINE] = "";
	strcpy(xmlPath, dirPath);
	strcat(xmlPath, "/ModelStructure.xml");

	FILE *resFP = SS_API::SimulationRequest(xmlPath, dirPath, simOpt, dirPath);

	fclose(resFP);
	// 4: Analyze Simulation Result --> Calculate Target Tollgate Number

	ifstream fp;
	char resPath[MAX_LINE] = "";
	strcpy(resPath, dirPath);
	strcat(resPath, "/SimulationResult.txt");
	fp.open(resPath);

	double data[10] = { 0 };

	std::string line;
	char line2[100];

	double tmp = 0;

	int lines = 0;
	int selectedTollgate = 0;


	double avgTotalServiceTime = 0;
	double avgBusyTime = 0;
	double avgETCSBusyTime = 0;
	double avgTCSBusyTime = 0;
	double avgWaitTime = 0;
	double avgETCSWaitTime = 0;
	double avgTCSWaitTime = 0;
	double avgQueueLength = 0;
	double avgETCSQLength = 0;
	double avgTCSQLength = 0;

	while (getline(fp, line))
	{
		lines++;
		memset(line2, 0, 100);
		strcpy(line2, line.c_str());

		char **tokens;
		tokens = str_split(line2, '\t');

		avgTotalServiceTime += atof(tokens[0]);
		avgBusyTime += atof(tokens[1]);
		if (IsDouble(tokens[2]))
			avgETCSBusyTime += atof(tokens[2]);
		else
			avgETCSBusyTime -= 1;

		if (IsDouble(tokens[3]))
			avgTCSBusyTime += atof(tokens[3]);
		else
			avgTCSBusyTime -= 1;

		avgWaitTime += atof(tokens[4]);
		if (IsDouble(tokens[5]))
			avgETCSWaitTime += atof(tokens[5]);
		else
			avgETCSWaitTime -= 1;

		if (IsDouble(tokens[6]))
			avgTCSWaitTime += atof(tokens[6]);
		else
			avgTCSWaitTime -= 1;

		avgQueueLength += atof(tokens[7]);
		if (IsDouble(tokens[8]))
			avgETCSQLength += atof(tokens[8]);
		else
			avgETCSQLength -= 1;

		if (IsDouble(tokens[9]))
			avgTCSQLength += atof(tokens[9]);
		else
			avgTCSQLength -= 1;

	}

	avgTotalServiceTime /= lines;
	avgBusyTime /= lines;
	avgETCSBusyTime /= lines;
	avgTCSBusyTime /= lines;
	avgWaitTime /= lines;
	avgETCSWaitTime /= lines;
	avgTCSWaitTime /= lines;
	avgQueueLength /= lines;
	avgETCSQLength /= lines;
	avgTCSQLength /= lines;

	cout << avgTotalServiceTime << endl;
	cout << avgBusyTime << endl;
	cout << avgETCSBusyTime << endl;
	cout << avgTCSBusyTime << endl;
	cout << avgWaitTime << endl;
	cout << avgETCSWaitTime << endl;
	cout << avgTCSWaitTime << endl;
	cout << avgQueueLength << endl;
	cout << avgETCSQLength << endl;
	cout << avgTCSQLength << endl;

	data[0] = avgTotalServiceTime;
	data[1] = avgBusyTime;
	data[2] = avgETCSBusyTime;
	data[3] = avgTCSBusyTime;
	data[4] = avgWaitTime;
	data[5] = avgETCSWaitTime;
	data[6] = avgTCSWaitTime;
	data[7] = avgQueueLength;
	data[8] = avgETCSQLength;
	data[9] = avgTCSQLength;


	int res = calcTargetTCS(direction, isEP, sessionCount);

	cout << "Result: " << res << endl;


	// 5: Send Tollgate Number, Average of Simulation Results

	ofstream fp2;
	char outPath[MAX_LINE] = "";
	strcpy(outPath, dirPath);
	strcat(outPath, "/output.txt");

	fp2.open(outPath, ofstream::out);

	fp2 << res << "\n";
	fp2 << avgTotalServiceTime << "\t" << avgBusyTime << "\t" << avgETCSBusyTime << "\t" <<
		avgTCSBusyTime << "\t" << avgWaitTime << "\t" << avgETCSWaitTime << "\t" <<
		avgTCSWaitTime << "\t" << avgQueueLength << "\t" << avgETCSQLength << "\t" <<
		avgTCSQLength;

	fp2.close();

	FILE *fp3 = fopen(outPath, "rb");
	if (fp3 == NULL)
	{
		perror("Can't open file");
		exit(1);
	}

	fseek(fp3, 0, SEEK_END);
	int fileSize = ftell(fp3);
	fseek(fp3, 0, SEEK_SET);

	if (send(sock, &fileSize, sizeof(int), 0) == -1)
	{
		perror("Can't send fileSize");
		exit(1);
	}

	int sendRes = sendfile(fp3, sock);

	printf("file send completed, %d\n", sendRes);

}

int CMEAppTollgate::sendfile(FILE *fp, int sockfd)
{
	int n;
	char sendline[MAX_LINE] = { 0 };
	ssize_t total = 0;

	while ((n = fread(sendline, sizeof(char), MAX_LINE, fp)) > 0)
	{
		total += n;
		if (n != MAX_LINE && ferror(fp))
		{
			perror("Read File Error");
			exit(1);
		}

		if (send(sockfd, sendline, n, 0) == -1)
		{
			perror("Can't send file");
			exit(1);
		}
		memset(sendline, 0, MAX_LINE);
	}

	return total;
}


const char *CMEAppTollgate::my_itoa_buf(char *buf, size_t len, int num)
{
	char loc_buf[sizeof(int) * 8];

	if (!buf)
	{
		buf = loc_buf;
		len = sizeof(loc_buf);
	}

	if (snprintf(buf, len, "%d", num) == -1)
		return "";

	return buf;
}

const char *CMEAppTollgate::my_itoa(int num)
{
	return my_itoa_buf(NULL, 0, num);
}

char** CMEAppTollgate::str_split(char* a_str, const char a_delim)
{
	char** result = 0;
	size_t count = 0;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	count += last_comma < (a_str + strlen(a_str) - 1);

	count++;

	result = (char**)malloc(sizeof(char*) * count);

	if (result)
	{
		size_t idx = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		*(result + idx) = 0;
	}

	return result;
}

int CMEAppTollgate::IsDouble(const char *str)
{
	char *endPtr = 0;
	strtod(str, &endPtr);

	if (*endPtr != '\0' || endPtr == str)
	{
		return 0;
	}
	return 1;
}

int CMEAppTollgate::calcTargetTCS(int direction, int payType, int session)
{
	int start, end;
	int res = 0;

	if (direction == 0)	// LEFT
	{

		start = 0;
		end = m_procInfo[session].size() / 3 - 1;

		if (end < 0)
			end = 0;

	}
	else if (direction == 1) // STRAIGHT
	{
		start = m_procInfo[session].size() / 3;
		end = start + m_procInfo[session].size() / 3 - 1 + m_procInfo[session].size() % 3;
	}

	else // RIGHT
	{
		end = m_procInfo[session].size() - 1;
		start = end + 1 - m_procInfo[session].size() / 3;
		if (m_procInfo[session].size() / 3 == 0)
			start -= 1;
	}

	res = findTCS(payType, start, end, session);
}

int CMEAppTollgate::findTCS(int type, int _start, int _end, int session)
{
	int res = -1;
	int min = 20000000;
	int start = _start, end = _end;

	do {

		for (int i = start; i <= end; i++)
		{

			if (m_procInfo[session][i] == type)
			{

				if (m_procInfo[session].size() < min)
				{
					res = i;
					min = m_procInfo[session].size();
				}
			}
		}

		start -= 2;
		if (start < 0)
			start = 0;
		end += 2;
		if (end > m_procInfo[session].size() - 1)
			end = m_procInfo[session].size() - 1;

	} while (res == -1);

	return res;
}
