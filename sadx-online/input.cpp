#include "stdafx.h"

#include <SADXModLoader.h>

#include "globals.h"
#include "input.h"
#include "packet_overloads.h"
#include "typedefs.h"

static ControllerData net_input[ControllerPointers_Length] {};
static PolarCoord net_analogs[ControllerPointers_Length] {};

sws::Packet& operator <<(sws::Packet& packet, const PolarCoord& data)
{
	return packet << data.direction << data.magnitude;
}

sws::Packet& operator >>(sws::Packet& packet, PolarCoord& data)
{
	return packet >> data.direction >> data.magnitude;
}

inline Angle to_relative(Angle value)
{
	if (Camera_Data1)
	{
		value += -Camera_Data1->Rotation.y;
	}

	return value;
}

inline Angle to_absolute(Angle value)
{
	if (Camera_Data1)
	{
		value -= -Camera_Data1->Rotation.y;
	}

	return value;
}

bool input_reader(MessageID id, pnum_t pnum, sws::Packet& packet)
{
	switch (id)
	{
		default:
			return false;

		case MessageID::I_Buttons:
			packet >> net_input[pnum].HeldButtons;
			break;

		case MessageID::I_Analog:
			packet >> net_input[pnum].LeftStickX >> net_input[pnum].LeftStickY;
			break;

		case MessageID::I_AnalogAngle:
		{
			auto& net_analog = net_analogs[pnum];
			packet >> net_analog;
			NormalizedAnalogs[pnum] = net_analog;
			break;
		}
	}

	return true;
}

void input_writer(MessageID id, pnum_t pnum, sws::Packet& packet)
{
	switch (id)
	{
		default:
			break;

		case MessageID::I_Buttons:
			packet << net_input[pnum].HeldButtons;
			break;

		case MessageID::I_Analog:
			packet << net_input[pnum].LeftStickX << net_input[pnum].LeftStickY;
			break;

		case MessageID::I_AnalogAngle:
		{
			packet << net_analogs[pnum];
			break;
		}
	}
}

static void send(MessageID type, pnum_t pnum)
{
	using namespace globals;

	const auto pad = ControllerPointers[pnum];
	auto& net_pad = net_input[pnum];

	switch (type)
	{
		default:
			break;

		case MessageID::I_Analog:
		{
			net_pad.LeftStickX = pad->LeftStickX;
			net_pad.LeftStickY = pad->LeftStickY;
			broker->write(MessageID::I_Analog);
			break;
		}

		case MessageID::I_AnalogAngle:
		{
			auto& net_analog = net_analogs[pnum];
			net_analog = NormalizedAnalogs[pnum];
			broker->write(MessageID::I_AnalogAngle);
			break;
		}

		case MessageID::I_Buttons:
		{
			net_pad.HeldButtons = pad->HeldButtons;
			broker->write(MessageID::I_Buttons);
			break;
		}
	}
}

extern "C"
{
	__declspec(dllexport) void __cdecl OnInput()
	{
		using namespace globals;

		if (broker == nullptr)
		{
			return;
		}

		broker->read();

		pnum_t pnum = broker->player_number();

		for (auto i = 0; i < 4; i++)
		{
			ControllerData* pad = ControllerPointers[i];
			auto& net_pad = net_input[i];

			if (i == pnum)
			{
				if (pad->PressedButtons || pad->ReleasedButtons)
				{
					send(MessageID::I_Buttons, i);
				}

				if (pad->LeftStickX != net_pad.LeftStickX || pad->LeftStickY != net_pad.LeftStickY)
				{
					send(MessageID::I_Analog, i);
				}

				continue;
			}

			pad->LeftStickX  = net_pad.LeftStickX;
			pad->LeftStickY  = net_pad.LeftStickY;
			pad->RightStickX = net_pad.RightStickX;
			pad->RightStickY = net_pad.RightStickY;

			pad->HeldButtons    = net_pad.HeldButtons;
			pad->NotHeldButtons = ~pad->HeldButtons;

			// Here we're using net_pad's "Old" since it can in some cases be overwritten
			// by the input update with irrelevant data, thus invalidating Released and Pressed.
			const uint mask = pad->HeldButtons ^ net_pad.Old;
			pad->ReleasedButtons = net_pad.Old & mask;
			pad->PressedButtons  = pad->HeldButtons & mask;

			// Setting pad->Old might not be necessary, but better safe than sorry.
			net_pad.Old = pad->Old = pad->HeldButtons;

			// HACK: Fixes camera rotation in non-emerald hunting modes.
			pad->LTriggerPressure = pad->HeldButtons & Buttons_L ? UCHAR_MAX : 0;
			pad->RTriggerPressure = pad->HeldButtons & Buttons_R ? UCHAR_MAX : 0;
		}
	}

	__declspec(dllexport) void __cdecl OnControl()
	{
		using namespace globals;

		for (int i = 2; i < 8; i++)
		{
			if (!IsControllerEnabled(i))
			{
				Controllers[i] = {};
				continue;
			}

			Controllers[i] = *ControllerPointers[i];
		}

		if (!broker)
		{
			return;
		}

		const pnum_t pnum = broker->player_number();

		for (auto i = 0; i < 4; i++)
		{
			const PolarCoord& current    = NormalizedAnalogs[i];
			const PolarCoord& net_analog = net_analogs[i];

			if (i == pnum)
			{
				//RumblePort_A[i] = 0;

				const auto angle = current.direction;

				if (angle != net_analog.direction || std::abs(current.magnitude - net_analog.magnitude) >= std::numeric_limits<float>::epsilon())
				{
					send(MessageID::I_AnalogAngle, i);
				}

				continue;
			}

			//RumblePort_A[i] = -1;

			NormalizedAnalogs[i] = net_analog;
		}
	}
}

void events::input_register()
{
	globals::broker->register_reader(MessageID::I_Buttons,     &input_reader);
	globals::broker->register_reader(MessageID::I_Analog,      &input_reader);
	globals::broker->register_reader(MessageID::I_AnalogAngle, &input_reader);

	globals::broker->register_writer(MessageID::I_Buttons,     &input_writer);
	globals::broker->register_writer(MessageID::I_Analog,      &input_writer);
	globals::broker->register_writer(MessageID::I_AnalogAngle, &input_writer);
}
