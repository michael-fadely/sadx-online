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

enum class RegisterType
{
	immediate,
	input,
	tick
};

class PacketBroker
{
	size_t tick = 0;

	std::shared_ptr<Listener> listener;
	std::deque<std::shared_ptr<Connection>> connections;

	PacketEx packet_out;
	PacketEx packet_in;
	bool is_server = false;

	using MessageReader = std::function<bool(MessageID message_id, pnum_t player_number, sws::Packet& packet)>;
	using MessageWriter = std::function<void(MessageID message_id, pnum_t player_number, sws::Packet& packet)>;

	struct StoredPacket
	{
		StoredPacket(PacketEx packet_, size_t tick_);
		StoredPacket(const StoredPacket&) = default;
		StoredPacket(StoredPacket&& other) noexcept;

		StoredPacket& operator=(const StoredPacket&) = default;
		StoredPacket& operator=(StoredPacket&& other) noexcept;

		PacketEx packet;
		size_t tick;
	};

	std::deque<StoredPacket> input_packet_queue, tick_packet_queue;

	std::unordered_map<MessageID, MessageReader> immediate_readers, input_readers, tick_readers;
	std::unordered_map<MessageID, MessageWriter> message_writers;

	pnum_t player_number_ = 0;

	void read_packet_type(PacketEx& packet, const std::unordered_map<MessageID, MessageReader>& readers) const;

public:
	PacketBroker() = default;
	~PacketBroker();

	void register_reader(RegisterType type, MessageID message_id, const MessageReader& reader);
	void register_writer(MessageID message_id, const MessageWriter& writer);

	void listen(const sws::Address& address);
	void connect(const sws::Address& address);
	void read();

	// TODO: better name. Input is not being processed, this is data deferred to the input stage of the game loop.
	void process_input();
	// TODO: better name. This is data deferred to the start of each tick.
	void process_tick();

	void write(MessageID message_id, bool allow_dupes = false);
	void init_outbound();

	void finalize();
	static void swap_input(size_t from, size_t to);

	void player_number(pnum_t value);
	pnum_t player_number() const;
};
