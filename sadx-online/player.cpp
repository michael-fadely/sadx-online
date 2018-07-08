#include "stdafx.h"
#include <chrono>

#include <SADXModLoader.h>

#include "MessageID.h"
#include "globals.h"
#include "dirty_t.h"
#include "packet_overloads.h"
#include "Timer.h"

#include "player.h"

using namespace std::chrono;

static constexpr uint16_t STATUS_MASK = Status_Hurt | Status_Ball | Status_LightDash;

static dirty_t<int8_t> last_action;
static dirty_t<int16_t> last_status;

static Timer update_timer(1s);

bool player_reader(MessageID id, pnum_t pnum, sws::Packet& packet)
{
	auto data1 = EntityData1Ptrs[pnum];
	auto data2 = EntityData2Ptrs[pnum];
	auto char2 = CharObj2Ptrs[pnum];

	if (!data1 || !data2 || !char2)
	{
		return false;
	}

	switch (id)
	{
		default:
			return false;

		case MessageID::P_Action:
		{
			packet >> data1->Action;
			PrintDebug("ACTION: %d\n", data1->Action);
			return true;
		}

		case MessageID::P_NextAction:
		{
			packet >> data1->NextAction;
			data1->Status |= Status_DoNextAction;
			return true;
		}

		case MessageID::P_Status:
		{
			short status;
			packet >> status;
			data1->Status &= ~STATUS_MASK;
			data1->Status |= status;
			return true;
		}

		case MessageID::P_Rotation:
		{
			update_timer.start();
			packet >> data1->Rotation;
			return true;
		}

		case MessageID::P_Position:
		{
			update_timer.start();
			packet >> data1->Position;
			return true;
		}

		case MessageID::P_Speed:
		{
			update_timer.start();
			packet >> char2->Speed >> data2->VelocityDirection;
			return true;
		}
	}
}

void player_writer(MessageID id, pnum_t pnum, sws::Packet& packet)
{
	auto data1 = EntityData1Ptrs[pnum];
	auto data2 = EntityData2Ptrs[pnum];
	auto char2 = CharObj2Ptrs[pnum];

	if (!data1 || !data2 || !char2)
	{
		return;
	}

	switch (id)
	{
		default:
			return;

		case MessageID::P_Action:
			packet << static_cast<int8_t>(last_action);
			break;

		case MessageID::P_NextAction:
			packet << static_cast<int8_t>(data1->NextAction);
			break;

		case MessageID::P_Status:
			packet << static_cast<int16_t>((static_cast<int16_t>(last_status) & STATUS_MASK));
			break;

		case MessageID::P_Rotation:
			packet << data1->Rotation;
			break;

		case MessageID::P_Position:
			packet << data1->Position;
			break;

		case MessageID::P_Speed:
			packet << char2->Speed << data2->VelocityDirection;
			break;
	}
}

void events::player_register()
{
	// TODO: provide method to defer processing to the right time in the game loop
	globals::broker->register_reader(MessageID::P_Action, &player_reader);
	globals::broker->register_reader(MessageID::P_NextAction, &player_reader);
	globals::broker->register_reader(MessageID::P_Status, &player_reader);
	globals::broker->register_reader(MessageID::P_Rotation, &player_reader);
	globals::broker->register_reader(MessageID::P_Position, &player_reader);
	globals::broker->register_reader(MessageID::P_Speed, &player_reader);

	globals::broker->register_writer(MessageID::P_Action, &player_writer);
	globals::broker->register_writer(MessageID::P_NextAction, &player_writer);
	globals::broker->register_writer(MessageID::P_Status, &player_writer);
	globals::broker->register_writer(MessageID::P_Rotation, &player_writer);
	globals::broker->register_writer(MessageID::P_Position, &player_writer);
	globals::broker->register_writer(MessageID::P_Speed, &player_writer);

	update_timer.start();
}

void events::player_update()
{
	if (GameState != 15)
	{
		return;
	}

	const auto pnum = globals::broker->player_number();

	auto data1 = EntityData1Ptrs[pnum];

	if (!data1)
	{
		return;
	}

	last_action = data1->Action;

	if (last_action.dirty())
	{
		//globals::broker->write(MessageID::P_Action);
		//globals::broker->write(MessageID::P_Status);
		globals::broker->write(MessageID::P_Rotation);
		globals::broker->write(MessageID::P_Position);
		globals::broker->write(MessageID::P_Speed);
		last_action.clear();
	}

	if (update_timer.done())
	{
		PrintDebug("SEND TIMER!\n");
		//globals::broker->write(MessageID::P_Action);
		globals::broker->write(MessageID::P_Rotation);
		globals::broker->write(MessageID::P_Position);
		globals::broker->write(MessageID::P_Speed);
		update_timer.start();
	}

	if (data1->Status & Status_DoNextAction)
	{
		globals::broker->write(MessageID::P_NextAction);
	}

	last_status = data1->Status & STATUS_MASK;

	if (last_status.dirty())
	{
		//globals::broker->write(MessageID::P_Status);
		last_status.clear();
	}
}
