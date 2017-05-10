#pragma once
#include "stdafx.h"

class ListenSocketContainer
{
private:
	static SOCKET * sListenSocket;
	static bool isActive;
	ListenSocketContainer() {};
public:
	static void SetListenSocket(SOCKET * listenSocket);
	static SOCKET * GetListenSocket();
	static void Deactivate();
	static bool IsActive();
};

