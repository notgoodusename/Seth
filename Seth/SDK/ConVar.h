#pragma once

#include <type_traits>

#include "Platform.h"
#include "UtlVector.h"
#include "Pad.h"
#include "VirtualMethod.h"

enum CvarFlags
{
	NONE = 0,
	UNREGISTERED = (1 << 0),
	DEVELOPMENTONLY = (1 << 1),
	GAMEDLL = (1 << 2),
	CLIENTDLL = (1 << 3),
	HIDDEN = (1 << 4),
	PROTECTED = (1 << 5),
	SPONLY = (1 << 6),
	ARCHIVE = (1 << 7),
	NOTIFY = (1 << 8),
	USERINFO = (1 << 9),
	CHEAT = (1 << 14),
	PRINTABLEONLY = (1 << 10),
	UNLOGGED = (1 << 11),
	NEVER_AS_STRING = (1 << 12),
	REPLICATED = (1 << 13),
	DEMO = (1 << 16),
	DONTRECORD = (1 << 17),
	NOT_CONNECTED = (1 << 22),
	ARCHIVE_XBOX = (1 << 24),
	SERVER_CAN_EXECUTE = (1 << 28),
	SERVER_CANNOT_QUERY = (1 << 29),
	CLIENTCMD_CAN_EXECUTE = (1 << 30)
};

class ConCommandBase
{
public:
	void* vmt;
	ConCommandBase* next;
	bool registered;
	const char* name;
	const char* helpString;
	int flags;
	static ConCommandBase* conCommandBases;
	static void* accessor;
};

class ConVar : ConCommandBase
{
public:
	VIRTUAL_METHOD(void, setValue, 10, (const char* value), (this, value))
	void setValue(float value) noexcept //bandaid fix
	{
		setValue(std::to_string(value).data());
	}
	//VIRTUAL_METHOD(void, setValue, 11, (float value), (this, value))
	VIRTUAL_METHOD(void, setValue, 12, (int value), (this, value))

	float getFloat() noexcept
	{
		return parent->value.floatValue;
	}

	int getInt() noexcept
	{
		return parent->value.intValue;
	}

	void* vmt;
	ConVar* parent;
	const char* defaultValue;
	struct Value
	{
		char* string;
		int stringLength;
		float floatValue;
		int	intValue;
	} value;
	bool hasMin;
	float minVal;
	bool hasMax;
	float maxVal;
};