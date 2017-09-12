#pragma once

#include <WTypes.h>
#include <SADXStructs.h>
#include <sws/Packet.h>
#include <ninja.h>

sws::Packet& operator<<(sws::Packet& packet, const NJS_VECTOR& data);
sws::Packet& operator>>(sws::Packet& packet, NJS_VECTOR&       data);
sws::Packet& operator<<(sws::Packet& packet, const Angle&      data);
sws::Packet& operator>>(sws::Packet& packet, Angle&            data);
sws::Packet& operator<<(sws::Packet& packet, const Rotation3&  data);
sws::Packet& operator>>(sws::Packet& packet, Rotation3&        data);
