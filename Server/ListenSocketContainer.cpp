#include "stdafx.h"
#include "ListenSocketContainer.h"

SOCKET * ListenSocketContainer::sListenSocket;
bool ListenSocketContainer::isActive;

void ListenSocketContainer::SetListenSocket(SOCKET * listenSocket)
{
	sListenSocket = listenSocket;
	isActive = true;
}

SOCKET * ListenSocketContainer::GetListenSocket()
{
	return sListenSocket;
}

void ListenSocketContainer::Deactivate()
{
	isActive = false;
}

bool ListenSocketContainer::IsActive()
{
	return isActive;
}