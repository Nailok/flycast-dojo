#include <atomic>
#include <iostream>
#include <queue>
#include <regex>
#include <string>

#include "../net_platform.h"

class TCPClient
{
private:
	std::string host;
	int port;

	sock_t sock = INVALID_SOCKET;
	sockaddr_in host_addr;

	unsigned char to_send[256];

	bool isLoopStarted;

	void CloseSocket(sock_t& socket) const { closesocket(socket); socket = INVALID_SOCKET; }

public:
	TCPClient();

	bool isStarted;

	std::queue<std::string> transmission_frames;

	bool Init();
	bool CreateSocket();
	void Connect();

	void ClientLoop();
	void TransmissionThread();
};

