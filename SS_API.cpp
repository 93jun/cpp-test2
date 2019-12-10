#include "SS_API.h"

string SS_API::m_IP;
uint16_t SS_API::m_port;

void SS_API::SetConnectionInfo(char *IP, uint16_t port)
{
	string str1(IP);

	m_IP = str1;
	m_port = port;
}

FILE* SS_API::SimulationRequest(char *fileName, SIM_OPT simOpt, char *resPath)
{
	int sockfd = SocketConnection();
	SendSimulationModel(sockfd, fileName, simOpt, 0);
	FILE *fp = ResultReceive(sockfd, resPath);
	return fp;
}

FILE* SS_API::SimulationRequest(char *fileName, char *dirPath, SIM_OPT simOpt, char *resPath)
{
	int sockfd = SocketConnection();
	SendSimulationModel(sockfd, fileName, simOpt, 1);
	SendSimulationFiles(sockfd, dirPath);
	FILE *fp = ResultReceive(sockfd, resPath);
	return fp;
}
int SS_API::SocketConnection()
{

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("Can't allocate sockfd");
		exit(1);
	}

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	//serveraddr.sin_port = htons(m_port);
	//if (inet_pton(AF_INET, m_IP.c_str(), &serveraddr.sin_addr) < 0)
	serveraddr.sin_port = htons(SERVERPORT);
	if (inet_pton(AF_INET, SIM_SERVICE_IP, &serveraddr.sin_addr) < 0)
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

void SS_API::SendSimulationModel(int sockfd, char* fileName, SIM_OPT simOpt, int source)
{
	SIM_OPT simParam;
	simParam.simulation_count_total = simOpt.simulation_count_total;
	simParam.simulation_count_per_pod = simOpt.simulation_count_per_pod;
	simParam.is_source_included = source;

	if (send(sockfd, (char*)&simParam, sizeof(SIMULATION_OPTION), 0) == -1)
	{
		perror("Can't send simulation options");
		exit(1);
	}

	FILE *fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
		perror("Can't open file");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	int fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (send(sockfd, (int*)&fileSize, sizeof(int), 0) == -1)
	{
		perror("Can't send file size");
		exit(1);
	}

	int res = sendfile(fp, sockfd);
	printf("Send Success, NumBytes = %d\n", res);

	fclose(fp);
}

void SS_API::SendSimulationFiles(int sockfd, char* dirPath)
{
	usleep(20);
	vector<string> fileNames;

	DIR *d;
	struct dirent *dir;

	d = opendir(dirPath);
	if (d) {
		while ((dir = readdir(d)) != NULL) {

			if (strstr(dir->d_name, ".cpp")) {
				string tmp = dirPath;
				tmp.append("/");
				tmp.append(dir->d_name);
				fileNames.push_back(tmp);
			}
			else if (strstr(dir->d_name, ".h")) {
				string tmp = dirPath;
				tmp.append("/");
				tmp.append(dir->d_name);
				fileNames.push_back(tmp);
			}

		}
		closedir(d);
	}

	int fileCount = fileNames.size();

	if (send(sockfd, (int*)&fileCount, sizeof(int), 0) == -1)
	{
		perror("Can't send file size");
		exit(1);
	}

	for (int i = 0; i < fileCount; i++) {

		usleep(15);

		FILE *fp = fopen(fileNames[i].c_str(), "rb");
		char fileName[BUFFSIZE];
		strcpy(fileName, fileNames[i].c_str());
		if (fp == NULL)
		{
			perror("Can't open file");
			exit(1);
		}

		fseek(fp, 0, SEEK_END);
		int fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (send(sockfd, (int*)&fileSize, sizeof(int), 0) == -1)
		{
			perror("Can't send file size");
			exit(1);
		}

		// File name split by '/' before send filename!!!


		char* pTemp = fileName;
		int delimCount = 0;
		while ((pTemp = strchr(pTemp, '/')) != NULL) {
			delimCount++;
			pTemp++;
		}

		char **tokens;
		tokens = str_split(fileName, '/');

		memset(fileName, 0, MAX_LINE);

		strcpy(fileName, tokens[delimCount - 1]);

		int filenameSize = strlen(fileName);

		if (send(sockfd, (int*)&filenameSize, sizeof(int), 0) == -1)
		{
			perror("Can't send file size");
			exit(1);
		}

		if (send(sockfd, fileName, filenameSize, 0) == -1)
		{
			perror("Can't send file size");
			exit(1);
		}

		int res = sendfile(fp, sockfd);
		printf("Send Success, NumBytes = %d\n", res);

		fclose(fp);

	}

}

FILE* SS_API::ResultReceive(int sockfd, char *resPath)
{
	char filename[BUFFSIZE] = "";

	if (resPath != NULL) {
		strcat(filename, resPath);
		strcat(filename, "/");
	}
	//char filename[BUFFSIZE] = "SimulationResult.txt";
	strcat(filename, "SimulationResult.txt");

	char buf[MAX_LINE];
	ssize_t total = 0;
	int read_size = 0;

	FILE *fp = fopen(filename, "wb");

	memset(buf, 0, MAX_LINE);
	while ((read_size = recv(sockfd, buf, MAX_LINE, 0)) > 0)
	{

		//printf("%d \n", read_size);
		total += read_size;
		if (read_size == -1)
		{
			perror("Receive File Error");
			exit(1);
		}

		if (fwrite(buf, sizeof(char), read_size, fp) != read_size)
		{
			perror("Write File Error");
			exit(1);
		}

		//clear the message buffer
		memset(buf, 0, MAX_LINE);
	}

	printf("Receive Success, Numbytes = %ld \n", total);

	close(sockfd);

	fclose(fp);

	fp = fopen(filename, "rb");

	return fp;
}

int SS_API::sendfile(FILE *fp, int sockfd)
{
	int n;
	int total = 0;
	char sendline[MAX_LINE] = { 0 };
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

char** SS_API::str_split(char* a_str, const char a_delim)
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