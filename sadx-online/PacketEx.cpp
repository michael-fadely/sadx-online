#include "stdafx.h"
#include "PacketEx.h"
#include "typedefs.h"

sws::Packet& operator<<(sws::Packet& packet, const MessageID data)
{
	return packet << static_cast<uint8_t>(data);
}

sws::Packet& operator>>(sws::Packet& packet, MessageID& data)
{
	uint8_t id;
	packet >> id;
	data = static_cast<MessageID>(id);
	return packet;
}

PacketEx::PacketEx(PacketEx&& other) noexcept
	: Packet(std::move(other))
{
	building = other.building;
	message_types = std::move(other.message_types);
	start = other.start;

	other.building = false;
}

PacketEx& PacketEx::operator=(PacketEx&& other) noexcept
{
	Packet::operator=(std::move(other));

	building = other.building;
	message_types = std::move(other.message_types);
	start = other.start;

	other.building = false;
	return *this;
}

bool PacketEx::add_type(MessageID type, bool allow_dupe)
{
	sws::enforce(!building, "Attempt to add new type while still building an old one.");

	if (!allow_dupe && contains(type))
	{
		return false;
	}

	building = true;
	*this << type << static_cast<messagelen_t>(std::numeric_limits<messagelen_t>::max());
	start = tell(sws::SeekCursor::write);
	message_types[type] = true;

	return true;
}

void PacketEx::end_type()
{
	if (!building)
	{
		return;
	}

	const auto position = tell(sws::SeekCursor::write);
	const auto size = position - start;

	seek(sws::SeekCursor::write, sws::SeekType::from_start, start - sizeof(messagelen_t));
	*this << static_cast<messagelen_t>(size);

	start = 0;
	building = false;

	seek(sws::SeekCursor::write, sws::SeekType::from_start, position);
}

bool PacketEx::contains(MessageID type) const
{
	const auto it = message_types.find(type);
	return it != message_types.end();
}

void PacketEx::clear()
{
	Packet::clear();
	building = false;
	start = false;
	message_types.clear();
}

bool PacketEx::has_types() const
{
	return !message_types.empty();
}
