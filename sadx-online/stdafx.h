#pragma once

#ifndef _DEBUG

#include <Windows.h>
#include <ShellAPI.h>
#include <WinSock2.h>

// STL
#include <string>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <memory>
#include <thread>

#include <SADXModLoader.h>
#include <Trampoline.h>

#include <sws/Socket.h>
#include <sws/TcpSocket.h>
#include <sws/Packet.h>
#include <sws/SocketError.h>
#include "PacketEx.h"
#include "PacketBroker.h"
#include "packet_overloads.h"
#include "MessageID.h"

#endif
