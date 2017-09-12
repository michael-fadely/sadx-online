#pragma once

#include <cstdint>
#include <sws/Packet.h>

namespace reliable
{
	enum class manage_id : uint8_t
	{
		eop         = 127,
		connect     = 'C',
		connected   = 'O',
		type        = 'T',
		sequence    = 'S',
		ack         = 'A',
		bad_version = 'V',
	};

	enum class reliable_t : uint8_t
	{
		/**
		 * \brief no special treatment required
		 */
		none,

		/**
		 * \brief no ack required, but check the sequence number
		 */
		newest,

		/**
		 * \brief receiver should acknowledge once received;
		 * no other special treatment required
		 */
		ack,

		/**
		 * \brief receiver should store only if newer than last received
		 */
		ack_newest,

		/**
		 * \brief same as regular ack, but the client holds back
		 * until an ack is received back
		 */
		ordered
	};

	void reserve(sws::Packet& packet, reliable_t type);
	sws::Packet reserve(reliable_t type);
}

sws::Packet& operator <<(sws::Packet& packet, reliable::manage_id   data);
sws::Packet& operator >>(sws::Packet& packet, reliable::manage_id&  data);
sws::Packet& operator <<(sws::Packet& packet, reliable::reliable_t  data);
sws::Packet& operator >>(sws::Packet& packet, reliable::reliable_t& data);
