#include "stdafx.h"
#include "packet_overloads.h"

sws::Packet& operator<<(sws::Packet& packet, const NJS_VECTOR& data)
{
	return packet << data.x << data.y << data.z;
}

sws::Packet& operator>>(sws::Packet& packet, NJS_VECTOR& data)
{
	return packet >> data.x >> data.y >> data.z;
}

sws::Packet& operator<<(sws::Packet& packet, const Angle& data)
{
	return packet << static_cast<int32_t>(data);
}

sws::Packet& operator>>(sws::Packet& packet, Angle& data)
{
	return packet >> *reinterpret_cast<int32_t*>(&data);
}

sws::Packet& operator<<(sws::Packet& packet, const Rotation3& data)
{
	return packet << data.x << data.y << data.z;
}

sws::Packet& operator>>(sws::Packet& packet, Rotation3& data)
{
	return packet >> data.x >> data.y >> data.z;
}
