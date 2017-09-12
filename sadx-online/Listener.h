#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <sws/UdpSocket.h>
#include "Connection.h"

class Listener
{
	using clock = std::chrono::high_resolution_clock;

	std::shared_ptr<sws::UdpSocket> socket;

	std::unordered_map<sws::Address, std::shared_ptr<Connection>> connections;

	bool is_bound     = false;
	bool is_connected = false;

	clock::time_point last_connect;

public:
	explicit Listener();

	sws::SocketState host(const sws::Address& address);
	sws::SocketState listen(std::shared_ptr<Connection>& connection);
	sws::SocketState connect(const sws::Address& host_address, std::shared_ptr<Connection>& connection);
	sws::SocketState receive(bool block = false, size_t count = 0);

	bool connected() const;
};
