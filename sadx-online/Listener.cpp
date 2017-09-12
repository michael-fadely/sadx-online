#include "stdafx.h"
#include <thread>
#include "Listener.h"
#include "reliable.h"

using namespace sws;
using namespace std::chrono;
using namespace reliable;

using protover_t = uint16_t;

static constexpr protover_t PROTOCOL_VERSION = 1;

Listener::Listener()
	: socket(std::make_shared<UdpSocket>(false))
{
}

SocketState Listener::host(const Address& address)
{
	if (is_bound)
	{
		return SocketState::done;
	}

	const SocketState result = socket->bind(address);
	is_bound = result == SocketState::done;
	return result;
}

SocketState Listener::listen(std::shared_ptr<Connection>& connection)
{
	if (!is_bound)
	{
		throw std::runtime_error("cannot listen for connections on unbound socket");
	}

	Packet  in, out;
	Address address;

	SocketState result = socket->receive_from(in, address);

	if (result != SocketState::done)
	{
		return result;
	}

	while (!in.end())
	{
		manage_id id = manage_id::eop;
		in >> id;

		if (id == manage_id::eop)
		{
			break;
		}

		if (id != manage_id::connect)
		{
			result = SocketState::in_progress;
			break;
		}

		protover_t version = 0;
		in >> version;

		if (version == PROTOCOL_VERSION)
		{
			result = SocketState::done;
			out << manage_id::connected << manage_id::eop;
		}
		else
		{
			result = SocketState::in_progress;
			out << manage_id::bad_version << PROTOCOL_VERSION << manage_id::eop;
		}

		socket->send_to(out, address);
	}

	connection = std::make_shared<Connection>(socket, this, address);
	connections[address] = connection;
	return result;
}

SocketState Listener::connect(const Address& host_address, std::shared_ptr<Connection>& connection)
{
	if (is_connected)
	{
		return SocketState::done;
	}

	SocketState result;

	if (!is_bound)
	{
		const Address bind_addr = Address::get_addresses("localhost", Socket::any_port, host_address.family)[0];
		result = socket->bind(bind_addr);

		is_bound = result == SocketState::done;

		if (!is_bound)
		{
			return result;
		}
	}

	if (clock::now() - last_connect >= 500ms)
	{
		last_connect = clock::now();

		Packet out;
		out << manage_id::connect << PROTOCOL_VERSION << manage_id::eop;

		result = socket->send_to(out, host_address);

		if (result != SocketState::done)
		{
			return result;
		}
	}

	Packet  in;
	Address recv_address;

	result = socket->receive_from(in, recv_address);

	if (result != SocketState::done)
	{
		return result;
	}

	if (recv_address != host_address)
	{
		return SocketState::in_progress;
	}

	while (!in.end())
	{
		manage_id id = manage_id::eop;
		in >> id;

		switch (id)
		{
			case manage_id::connected:
				break;

			case manage_id::bad_version:
				throw std::runtime_error("version mismatch with server");

			case manage_id::eop:
				in.clear();
				continue;

			default:
				throw std::runtime_error("invalid message id for connection process");
		}
	}

	connection = std::make_shared<Connection>(socket, this, host_address);
	connections[host_address] = connection;
	is_connected = true;
	return SocketState::done;
}

SocketState Listener::receive(bool block, const size_t count)
{
	size_t i = 0;

	Packet packet;
	Address remote_address;

	SocketState result = SocketState::done;

	while ((block && result == SocketState::in_progress) || result == SocketState::done)
	{
		result = socket->receive_from(packet, remote_address);

		if (result == SocketState::in_progress)
		{
			std::this_thread::sleep_for(1ms);
			continue;
		}

		const auto it = connections.find(remote_address);

		if (it == connections.end())
		{
			packet.clear();
			result = SocketState::in_progress;
		}
		else
		{
			result = it->second->store_inbound(packet);
		}

		// TODO: decouple from pure ACKs
		if (count && ++i >= count)
		{
			break;
		}
	}

	return result;
}

bool Listener::connected() const
{
	return !connections.empty();
}
