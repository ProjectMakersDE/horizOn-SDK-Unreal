#include "Transport/HorizonLeaderboardTransportContract.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
	constexpr int Port = 18723;

	void Require(bool Condition, const char* Message)
	{
		if (!Condition)
		{
			throw std::runtime_error(Message);
		}
	}

	std::string ReceiveRequest(int Socket)
	{
		std::string Request;
		char Buffer[4096];
		for (;;)
		{
			const ssize_t Read = recv(Socket, Buffer, sizeof(Buffer), 0);
			if (Read <= 0)
			{
				break;
			}
			Request.append(Buffer, static_cast<std::size_t>(Read));
			const std::size_t HeaderEnd = Request.find("\r\n\r\n");
			if (HeaderEnd != std::string::npos)
			{
				const std::size_t LengthHeader = Request.find("Content-Length: ");
				if (LengthHeader != std::string::npos)
				{
					const std::size_t LengthStart = LengthHeader + std::strlen("Content-Length: ");
					const std::size_t LengthEnd = Request.find("\r\n", LengthStart);
					const std::size_t BodyLength = std::stoul(Request.substr(LengthStart, LengthEnd - LengthStart));
					if (Request.size() >= HeaderEnd + 4 + BodyLength)
					{
						break;
					}
				}
			}
		}
		return Request;
	}
}

int main()
{
	using namespace HorizonTransportContract;
	const FLeaderboardSubmitPlan Plan = BuildLeaderboardSubmitPlan(
		"user-720", "session-token-720", 4242, "season one", "");
	Require(Plan.bShouldSend, "signed user did not produce a submit plan");
	Require(!BuildLeaderboardSubmitPlan("", "session-token-720", 99, "season one", "").bShouldSend,
		"missing user id did not block submit");
	Require(!BuildLeaderboardSubmitPlan("user-720", "", 99, "season one", "").bShouldSend,
		"missing session token did not block submit");

	std::atomic<bool> Ready = false;
	std::exception_ptr ServerFailure;
	std::thread Server([&]() {
		try
		{
			const int ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
			Require(ServerSocket >= 0, "could not create server socket");
			int Reuse = 1;
			setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &Reuse, sizeof(Reuse));
			sockaddr_in Address{};
			Address.sin_family = AF_INET;
			Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			Address.sin_port = htons(Port);
			Require(bind(ServerSocket, reinterpret_cast<sockaddr*>(&Address), sizeof(Address)) == 0, "bind failed");
			Require(listen(ServerSocket, 1) == 0, "listen failed");
			Ready = true;
			const int Client = accept(ServerSocket, nullptr, nullptr);
			Require(Client >= 0, "accept failed");
			const std::string Request = ReceiveRequest(Client);
			Require(Request.find("POST /api/v1/app/leaderboards/season%20one/submit HTTP/1.1") != std::string::npos,
				"incorrect endpoint");
			Require(Request.find("X-API-Key: project-key-720") != std::string::npos, "missing project key");
			Require(Request.find("Authorization: Bearer session-token-720") != std::string::npos, "missing bearer token");
			Require(Request.find(Plan.BodyJson) != std::string::npos, "incorrect request body");
			const char Response[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Type: application/json\r\n\r\n{}";
			send(Client, Response, sizeof(Response) - 1, 0);
			close(Client);
			close(ServerSocket);
		}
		catch (...)
		{
			ServerFailure = std::current_exception();
			Ready = true;
		}
	});

	while (!Ready)
	{
		std::this_thread::yield();
	}
	if (ServerFailure)
	{
		Server.join();
		std::rethrow_exception(ServerFailure);
	}

	const int ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	Require(ClientSocket >= 0, "could not create client socket");
	sockaddr_in Address{};
	Address.sin_family = AF_INET;
	Address.sin_port = htons(Port);
	inet_pton(AF_INET, "127.0.0.1", &Address.sin_addr);
	Require(connect(ClientSocket, reinterpret_cast<sockaddr*>(&Address), sizeof(Address)) == 0, "connect failed");

	std::string Request = "POST " + Plan.Endpoint + " HTTP/1.1\r\nHost: 127.0.0.1\r\n";
	for (const auto& Header : BuildHeaders("project-key-720", "session-token-720", Plan.bUseSessionToken))
	{
		Request += Header.first + ": " + Header.second + "\r\n";
	}
	Request += "Content-Type: application/json\r\nContent-Length: " + std::to_string(Plan.BodyJson.size()) +
		"\r\nConnection: close\r\n\r\n" + Plan.BodyJson;
	send(ClientSocket, Request.data(), Request.size(), 0);
	char Response[128];
	const ssize_t ResponseLength = recv(ClientSocket, Response, sizeof(Response), 0);
	Require(ResponseLength > 0 && std::string(Response, static_cast<std::size_t>(ResponseLength)).find("200 OK") != std::string::npos,
		"contract server did not accept request");
	close(ClientSocket);
	Server.join();
	if (ServerFailure)
	{
		std::rethrow_exception(ServerFailure);
	}

	std::cout << "Unreal SDK leaderboard transport contract passed\n";
	return 0;
}
