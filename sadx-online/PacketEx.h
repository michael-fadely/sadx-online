#pragma once
#include <sws/Packet.h>
#include "MessageID.h"
#include <unordered_map>

sws::Packet& operator <<(sws::Packet& packet, MessageID data);
sws::Packet& operator >>(sws::Packet& packet, MessageID& data);

class PacketEx : public sws::Packet
{
	bool building = false;
	std::unordered_map<MessageID, bool> message_types;
	size_t start = 0;

public:
	bool add_type(MessageID type, bool allow_dupe = false);
	void end_type();

	bool contains(MessageID type) const;

	void clear() override;
	bool has_types() const;
};
