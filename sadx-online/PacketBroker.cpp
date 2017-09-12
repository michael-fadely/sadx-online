#include "stdafx.h"

#include <chrono>
#include <thread>

#include <SADXModLoader.h>

#include "PacketBroker.h"
#include "packet_overloads.h"
#include "Listener.h"

using namespace std::chrono;

void spawn_character(const int index, const Characters character)
{
	if (GetCharObj2(index) != nullptr)
	{
		return;
	}

	ObjectFuncPtr load_sub;

	switch (character)
	{
		case Characters_Sonic:
			load_sub = Sonic_Main;
			break;
		case Characters_Eggman:
			load_sub = Eggman_Main;
			break;
		case Characters_Tails:
			load_sub = Tails_Main;
			break;
		case Characters_Knuckles:
			load_sub = Knuckles_Main;
			break;
		case Characters_Tikal:
			load_sub = Tikal_Main;
			break;
		case Characters_Amy:
			load_sub = Amy_Main;
			break;
		case Characters_Gamma:
			load_sub = Gamma_Main;
			break;
		case Characters_Big:
			load_sub = Big_Main;
			break;
		default:
			throw std::runtime_error("invalid character provided");
	}

	ObjectMaster* object = LoadObject(static_cast<LoadObj>(LoadObj_Data2 | LoadObj_Data1 | LoadObj_UnknownA), 1, load_sub);
	object->Data1->CharID = character;
	object->Data1->CharIndex = index;

	EntityData1Ptrs[index] = object->Data1;
	CharObj2Ptrs[index]    = reinterpret_cast<EntityData2*>(object->Data2)->CharacterData;
	PlayerPtrs[index]      = object;
	EntityData2Ptrs[index] = reinterpret_cast<EntityData2*>(object->Data2);

	InitCharacterVars(index, object);
	EnableController(index);
	MovePlayerToStartPoint(object->Data1);
}

PacketBroker::~PacketBroker()
{
	// restore swapped input pointers if any
	swap_input(player_number_, 0);
}

void PacketBroker::register_reader(MessageID message_id, const MessageReader& reader)
{
	message_readers[message_id] = reader;
}

void PacketBroker::register_writer(MessageID message_id, const MessageWriter& writer)
{
	message_writers[message_id] = writer;
}

void PacketBroker::listen(const sws::Address& address)
{
	player_number(0);

	std::shared_ptr<Connection> connection;
	listener = std::make_shared<Listener>();
	is_server = true;

	if (listener->host(address) != sws::SocketState::done)
	{
		throw std::runtime_error("host failed");
	}

	while (listener->listen(connection) != sws::SocketState::done)
	{
		std::this_thread::sleep_for(250ms);
	}

	connections.push_back(connection);

	sws::Packet p = reliable::reserve(reliable::reliable_t::ack);
	p << EntityData1Ptrs[0]->Position;
	connection->send(p, true);

	spawn_character(1, Characters_Sonic);
	EntityData1Ptrs[1]->Position = EntityData1Ptrs[0]->Position;
}

void PacketBroker::connect(const sws::Address& address)
{
	player_number(1);

	std::shared_ptr<Connection> connection;
	listener = std::make_shared<Listener>();
	is_server = false;

	while (listener->connect(address, connection) != sws::SocketState::done)
	{
		std::this_thread::sleep_for(1ms);
	}

	connections.push_back(connection);


	PrintDebug("Waiting for position...\n");
	sws::Packet p;

	if (listener->receive(true, 1) != sws::SocketState::done) 
	{
		throw;
	}

	PrintDebug("Got it!\n");

	connection->pop(p);

	spawn_character(1, Characters_Sonic);
	p >> EntityData1Ptrs[1]->Position;
}

void PacketBroker::read()
{
	size_t i = 0;
	pnum_t pnum = player_number();
	listener->receive(false);

	for (auto& connection : connections)
	{
		connection->update();

		while (connection->pop(packet_in))
		{
			while (!packet_in.end())
			{
				MessageID id = MessageID::None;
				messagelen_t size = 0;

				packet_in >> id >> size;

				if (id == MessageID::N_PlayerNumber)
				{
					packet_in >> pnum;
					continue;
				}

				const ptrdiff_t offset = packet_in.tell(sws::SeekCursor::read);
				const auto it = message_readers.find(id);

				if (it == message_readers.end() || !it->second(id, pnum, packet_in))
				{
					packet_in.seek(sws::SeekCursor::read, sws::SeekType::from_start, offset + size);
					continue;
				}

				if (packet_in.tell(sws::SeekCursor::read) - offset != size)
				{
					throw std::runtime_error("message reader failed to read the correct amount of data.");
				}
			}

			if (is_server)
			{
				for (auto& c : connections)
				{
					if (c != connection)
					{
						c->send(packet_in);
					}
				}
			}
		}

		// TODO
		/*if (state == sws::SocketState::error)
		{
			throw;
		}*/

		DisplayDebugStringFormatted(NJM_LOCATION(1, 1 + i), "RTT %u: %lld ms",
									i, duration_cast<milliseconds>(connection->round_trip_time()).count());
		++i;
	}
}

void PacketBroker::write(MessageID message_id, bool allow_dupes)
{
	const auto it = message_writers.find(message_id);

	if (it == message_writers.end())
	{
		return;
	}

	if (packet_out.empty())
	{
		init_outbound();
	}

	if (!packet_out.add_type(message_id, allow_dupes))
	{
		return;
	}

	it->second(message_id, player_number(), packet_out);
	packet_out.end_type();
}

void PacketBroker::init_outbound()
{
	packet_out.clear();

	reliable::reserve(packet_out, reliable::reliable_t::ack_newest);

	packet_out.add_type(MessageID::N_PlayerNumber);
	packet_out << player_number();
	packet_out.end_type();
}

void PacketBroker::finalize()
{
	if (!listener || !packet_out.has_types())
	{
		return;
	}

	for (auto& connection : connections)
	{
		connection->send(packet_out);
	}

	packet_out.clear();
}

void PacketBroker::swap_input(size_t from, size_t to)
{
	if (from == to)
	{
		return;
	}

	std::swap(ControllerPointers[from], ControllerPointers[to]);
}

void PacketBroker::player_number(pnum_t value)
{
	if (value == player_number_)
	{
		return;
	}

	swap_input(player_number_, 0);
	swap_input(0, value);

	player_number_ = value;
}

pnum_t PacketBroker::player_number() const
{
	return player_number_;
}
