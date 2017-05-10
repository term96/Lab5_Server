#include "stdafx.h"
#include "ListenSocketContainer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <vector>

#define DEFAULT_PORT "80"
#define BUFFER_LEN 1024

using namespace std;
using namespace experimental::filesystem;

void Log(string const & msg)
{
	static ofstream output("log.txt", ios::app);
	time_t t = time(0);
	struct tm * now = new tm;
	localtime_s(now, &t);
	stringstream formattedMsg;

	formattedMsg << '['
		<< now->tm_mday << '.'
		<< now->tm_mon + 1<< '.'
		<< now->tm_year + 1900 << ' '
		<< now->tm_hour << ':'
		<< now->tm_min << ':'
		<< now->tm_sec << "] "
		<< msg;
	
	cout << formattedMsg.str() << '\n';
	output << formattedMsg.str() << '\n';
}

string CreateContentTypeHeader(string const & filePath)
{
	path file = filePath;
	string extension = file.extension().generic_string();
	if (extension.compare(".txt") == 0)
		return "Content-Type: text/plain; charset=utf-8\r\n";
	if (extension.compare(".html") == 0)
		return "Content-Type: text/html; charset=utf-8\r\n";
	if (extension.compare(".css") == 0)
		return "Content-Type: text/css; charset=utf-8\r\n";
	if (extension.compare(".js") == 0)
		return "Content-Type: text/javascript; charset=utf-8\r\n";
	if (extension.compare(".gif") == 0)
		return "Content-Type: image/gif\r\n";
	if (extension.compare(".png") == 0)
		return "Content-Type: image/png\r\n";
	if (extension.compare(".jpg") == 0 || extension.compare(".jpeg") == 0)
		return "Content-Type: image/jpeg\r\n";
	if (extension.compare(".bmp") == 0)
		return "Content-Type: image/bmp\r\n";
	if (extension.compare(".mp3") == 0)
		return "Content-Type: audio/mpeg\r\n";
	if (extension.compare(".ogg") == 0)
		return "Content-Type: audio/ogg\r\n";
	if (extension.compare(".wav") == 0)
		return "Content-Type: audio/wav\r\n";
	if (extension.compare(".webm") == 0)
		return "Content-Type: video/webm\r\n";
	if (extension.compare(".ogg") == 0)
		return "Content-Type: video/ogg\r\n";
	if (extension.compare(".xml") == 0)
		return "Content-Type: application/xml\r\n";
	if (extension.compare(".pdf") == 0)
		return "Content-Type: application/pdf\r\n";
	return "Content-Type: application/octet-stream\r\n";
}

string GetAllFilesJSON()
{
	path filesDir = "files";
	recursive_directory_iterator dir(filesDir);
	vector<string> paths;
	vector<bool> isDir;

	for (auto it : dir)
	{
		paths.push_back(it.path().generic_string());
		isDir.push_back(is_directory(it.path()));
	}

	stringstream json;
	json << '[';
	
	for (size_t i = 0; i < paths.size() - 1; ++i)
	{
		json << "{\"name\":\"" << paths[i] << "\", \"isDir\":\"" << isDir[i] << "\"}, ";
	}
	json << "{\"name\":\"" << paths.back() << "\", \"isDir\":\"" << isDir.back() << "\"}]";

	return json.str();
}

bool GetFile(string filePath, string & holder)
{
	filePath.erase(0, 1);
	ifstream file(filePath, ios::binary);
	if (!file.is_open())
	{
		return false;
	}
	stringstream stream;
	stream << file.rdbuf();

	holder = stream.str();
	return true;
}

void RemoveFile(string filePath)
{
	filePath.erase(0, 1);
	cout << (remove_all(filePath)) ? "removed\n" : "not exist\n";
}

string CreateGetFileResponse(string const & filePath)
{
	string response_body;
	bool fileFound = GetFile(filePath, response_body);
	stringstream response;
	
	if (fileFound)
	{
		response << "HTTP/1.1 200 OK\r\n"
			<< "Version: HTTP/1.1\r\n"
			<< "Access-Control-Allow-Origin: *\r\n"
			<< "Access-Control-Allow-Methods: PUT, GET, POST, DELETE, OPTIONS\r\n"
			<< "Connection: close\r\n"
			<< CreateContentTypeHeader(filePath)
			<< "Content-Length: " << response_body.length()
			<< "\r\n\r\n"
			<< response_body;
	}
	else
	{
		response_body = "404 File not found";
		response << "HTTP/1.1 404 File not found\r\n"
			<< "Version: HTTP/1.1\r\n"
			<< "Access-Control-Allow-Origin: *\r\n"
			<< "Access-Control-Allow-Methods: PUT, GET, POST, DELETE, OPTIONS\r\n"
			<< "Connection: close\r\n"
			<< "Content-Type: text/plain; charset=utf-8\r\n"
			<< "Content-Length: " << response_body.length()
			<< "\r\n\r\n"
			<< response_body;
	}
	
	return response.str();
}

string CreateGetAllResponse()
{
	string response_body = GetAllFilesJSON();
	stringstream response;
	response << "HTTP/1.1 200 OK\r\n"
		<< "Version: HTTP/1.1\r\n"
		<< "Access-Control-Allow-Origin: *\r\n"
		<< "Access-Control-Allow-Methods: GET, DELETE\r\n"
		<< "Connection: close\r\n"
		<< "Content-Type: application/json; charset=utf-8\r\n"
		<< "Content-Length: " << response_body.length()
		<< "\r\n\r\n"
		<< response_body;
	return response.str();
}

string CreateDeleteResponse()
{
	stringstream response;
	response << "HTTP/1.1 200 OK\r\n"
		<< "Version: HTTP/1.1\r\n"
		<< "Access-Control-Allow-Origin: *\r\n"
		<< "Access-Control-Allow-Methods: GET, DELETE\r\n"
		<< "Connection: close\r\n";
	return response.str();
}

void Unescape(string & path)
{
	static const string space = "%20";
	int pos = path.find(space);
	while (pos != string::npos)
	{
		path.replace(pos, 3, " ");
		pos = path.find(space);
	}
}

void HandleConnection(SOCKET & clientSocket)
{
	TIMEVAL timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	char recvbuf[BUFFER_LEN];
	int iSendResult, iResult;
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	iResult = recv(clientSocket, recvbuf, BUFFER_LEN, 0);
	if (iResult > 0) 
	{
		recvbuf[iResult] = '\0';
		stringstream received(recvbuf);
		string method;
		string path;
		received >> method >> path;
		Unescape(path);

		string response;
		if (method.compare("GET") == 0 && path.compare("/") == 0)
		{
			Log("Request: get all files");
			response = CreateGetAllResponse();
		}
		else if (method.compare("GET") == 0)
		{
			Log("Request: get file");
			response = CreateGetFileResponse(path);
		}
		else
		{
			Log("Request: delete file");
			response = CreateDeleteResponse();
			RemoveFile(path);
		}

		iSendResult = send(clientSocket, response.c_str(), response.length(), 0);
		if (iSendResult == SOCKET_ERROR)
		{
			if (ListenSocketContainer::IsActive())
			{
				Log("send failed: " + to_string(WSAGetLastError()));
			}
		}
		else
		{
			Log("Bytes sent: " + to_string(iSendResult));
		}
	}
	else if (iResult == 0)
	{
		Log("Connection closed");
	}
	else
	{
		if (ListenSocketContainer::IsActive())
		{
			Log("recv failed: " + to_string(WSAGetLastError()));
		}
	}

	closesocket(clientSocket);
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT || dwCtrlType == CTRL_CLOSE_EVENT)
	{
		Log("Server is shutting down\n");
		ListenSocketContainer::Deactivate();
		closesocket(*ListenSocketContainer::GetListenSocket());
		WSACleanup();
		return true;
	}
	return false;
}

int main()
{
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		Log("WSAStartup failed: " + to_string(iResult));
		return 1;
	}

	addrinfo hints, *result = NULL;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, "80", &hints, &result);
	if (iResult != 0)
	{
		Log("getaddrinfo failed: " + to_string(iResult));
		WSACleanup();
		return 1;
	}

	SOCKET listenSocket = INVALID_SOCKET;
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		Log("socket failed: " + to_string(WSAGetLastError()));
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	ListenSocketContainer::SetListenSocket(&listenSocket);
	SetConsoleCtrlHandler(HandlerRoutine, true);

	iResult = bind(listenSocket, result->ai_addr, result->ai_addrlen);
	
	if (iResult == SOCKET_ERROR) {
		Log("bind failed: " + to_string(WSAGetLastError()));
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		Log("listen failed: " + to_string(WSAGetLastError()));
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET clientSocket = INVALID_SOCKET;
	Log("Server is started");

	for (;;)
	{
		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			if (ListenSocketContainer::IsActive())
			{
				Log("accept failed: " + to_string(WSAGetLastError()));
				closesocket(listenSocket);
				WSACleanup();
				return 1;
			}
			else
			{
				return 0;
			}
		}
		HandleConnection(clientSocket);
	}
	
    return 0;
}

