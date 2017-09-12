#pragma once

#include <cstdint>

enum class MessageID : uint8_t
{
	None,

	N_START,            // Marker: Start of Networking messages
		N_Sequence,         // UDP Sequence
		N_VersionCheck,     // Initial version check upon connection
		N_VersionMismatch,  // Client version mismatch
		N_VersionOK,        // Client version match
		N_Bind,             // UDP bind request/confirm
		N_Password,         // Server password
		N_PasswordMismatch, // Incorrect server password
		N_Settings,         // Used for synchronizing settings
		N_Connected,        // Connection setup successful
		N_Ready,            // Client is ready at current sync block
		N_Disconnect,       // Request disconnect
		N_SetPlayerNumber,  // Send a player number change
		N_PlayerNumber,     // The player number this message came from
	N_END,              // Marker: End of Networking messages

	I_START,            // Marker: Start of input messages
		I_Analog,
		I_AnalogAngle,
		I_Buttons,
	I_END,              // Marker: End of input messages

	M_START,            // Marker: Start of menu messages
		// TODO
	M_END,              // Marker: End of menu messages

	P_START,            // Marker: Start of player messages
		P_Action,
		P_NextAction,
		P_Status,
		P_Rotation,
		P_Position,
		P_Speed,
	P_END,              // Marker: End of player messages

	S_START,            // Marker: Start of system messages
		// TODO
	S_END,              // Marker: End of system messages

	Count
};
