// Copyright (C) namazso 2014. Released under the following license:
//
//         DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
//                    Version 2, December 2004 
//
// Copyright (C) 2004 Sam Hocevar <sam@hocevar.net> 
//
// Everyone is permitted to copy and distribute verbatim or modified 
// copies of this license document, and changing it is allowed as long 
// as the name is changed. 
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>

// Winsock Library
#pragma comment(lib,"ws2_32.lib")

enum ServerData
{
	SERVERDATA_RESPONSEVALUE	= 0,
	SERVERDATA_EXEC_COMMAND		= 2,
	SERVERDATA_AUTH_RESPONSE	= 2,
	SERVERDATA_AUTH				= 3,
};

#define BUFSIZE    4096

int rcon_cmd(const char* command, int32_t server_data, uint8_t* packet)
{
	int32_t p_size = (int32_t)(strlen(command) + 10); //Packet Size (Integer)
	int32_t p_req = 0; //Request Id (Integer)
	int32_t p_srvd = server_data; //SERVERDATA_EXECCOMMAND / SERVERDATA_AUTH (Integer)

	unsigned c = 0;

	packet[c++] = (uint32_t)p_size & 0xFF;
	packet[c++] = ((uint32_t)p_size >> 8) & 0xFF;
	packet[c++] = ((uint32_t)p_size >> 16) & 0xFF;
	packet[c++] = ((uint32_t)p_size >> 24) & 0xFF;

	packet[c++] = (uint32_t)p_req & 0xFF;
	packet[c++] = ((uint32_t)p_req >> 8) & 0xFF;
	packet[c++] = ((uint32_t)p_req >> 16) & 0xFF;
	packet[c++] = ((uint32_t)p_req >> 24) & 0xFF;

	packet[c++] = (uint32_t)p_srvd & 0xFF;
	packet[c++] = ((uint32_t)p_srvd >> 8) & 0xFF;
	packet[c++] = ((uint32_t)p_srvd >> 16) & 0xFF;
	packet[c++] = ((uint32_t)p_srvd >> 24) & 0xFF;

	int i;
	for (i = c; command[i - c] && i < BUFSIZE; ++i)
	{
		packet[i] = command[i - c];
	}
	c = i;

	if (c >= (BUFSIZE - 2))
		return -1;

	packet[c++] = 0;
	packet[c++] = 0;

	return c;
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		perror("Usage: rcon_client <ip> <port> <password>");
		return 1;
	}

	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;

	printf("Initialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		fprintf(stderr, "Failed. Error Code : %d\n", WSAGetLastError());
		return 2;
	}

	printf("Initialized.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "Could not create socket : %d", WSAGetLastError());
		return 3;
	}

	printf("Socket created.\n");

	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons((short)strtol(argv[2], (char **)NULL, 10));

	//Connect to remote server
	if (connect(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		fprintf(stderr, "Connection error: %d.\n", errno);
		return 4;
	}

	printf("Connected\n");

	uint8_t packet[BUFSIZE];

	int packet_length = rcon_cmd(argv[3], SERVERDATA_AUTH, packet);
	if (packet_length < 0)
	{
		fprintf(stderr, "Password too long.\n");
		return 5;
	}

	if (send(s, packet, packet_length, 0) == SOCKET_ERROR)
	{
		fprintf(stderr, "send failed: %d.\n", errno);
		return 6;
	}

	printf("Logging in..\n");

	int recv_size;
	if ((recv_size = recv(s, packet, BUFSIZE, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "recv failed: %d.\n", errno);
		return 7;
	}
	if ((recv_size = recv(s, packet, BUFSIZE, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "recv failed: %d.\n", errno);
		return 8;
	}

	printf("Logged in.\n");

	while (1)
	{
		char cmd[BUFSIZE];
		scanf_s("%s", cmd, BUFSIZE);

		packet_length = rcon_cmd(cmd, SERVERDATA_EXEC_COMMAND, packet);
		if (packet_length < 0)
		{
			fprintf(stderr, "Command too long\n");
			continue;
		}

		if (send(s, packet, packet_length, 0) == SOCKET_ERROR)
		{
			fprintf(stderr, "send failed: %d.\n", errno);
			if (errno == ECONNRESET)
				break;
			continue;
		}
		if ((recv_size = recv(s, packet, BUFSIZE, 0)) == SOCKET_ERROR)
		{
			fprintf(stderr, "recv failed: %d.\n", errno);
			if (errno == ECONNRESET)
				break;
			continue;
		}
		puts(&(packet[12]));
	}

	WSACleanup();

	return 0;
}
