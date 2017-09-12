#pragma once

#include <deque>
#include <functional>
#include <unordered_map>
#include <memory>

#include <sws/Address.h>
#include <sws/Packet.h>
#include "Listener.h"

#include "PacketEx.h"
#include "MessageID.h"
#include "typedefs.h"

class PacketBroker
{
	std::shared_ptr<Listener> listener;
	std::deque<std::shared_ptr<Connection>> connections;

	PacketEx packet_out;
	PacketEx packet_in;
	bool is_server;

	using MessageReader = std::function<bool(MessageID message_id, pnum_t player_number, sws::Packet& packet)>;
	using MessageWriter = std::function<void(MessageID message_id, pnum_t player_number, sws::Packet& packet)>;
	std::unordered_map<MessageID, MessageReader> message_readers;
	std::unordered_map<MessageID, MessageWriter> message_writers;

	pnum_t player_number_ = 0;

public:
	PacketBroker() = default;
	~PacketBroker();

	void register_reader(MessageID message_id, const MessageReader& reader);
	void register_writer(MessageID message_id, const MessageWriter& writer);

	void listen(const sws::Address& address);
	void connect(const sws::Address& address);
	void read();
	void write(MessageID message_id, bool allow_dupes = false);
	void init_outbound();

	void finalize();
	static void swap_input(size_t from, size_t to);

	void player_number(pnum_t value);
	pnum_t player_number() const;
};
