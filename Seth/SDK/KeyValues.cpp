#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string_view>
#include <utility>
#include <Windows.h>
#include <Psapi.h>

#include "../Memory.h"

#include "KeyValues.h"
#include "VirtualMethod.h"

void KeyValues::initialize(char* name) noexcept
{
	memory->keyValuesInitialize(this, name);
}

KeyValues::KeyValues(const char* name) noexcept
{
	char _name[128];
	sprintf(_name, name);
	this->initialize(const_cast<char*>(_name));
}

KeyValues* KeyValues::findKey(const char* keyName, bool create) noexcept
{
	return memory->keyValuesFindKey(this, keyName, create);
}

void KeyValues::setString(const char* keyName, const char* value) noexcept
{
	KeyValues* dat = findKey(keyName, true);

	if (dat)
	{
		if (dat->dataType == 1 && dat->sValue == value)
			return;

		delete[] dat->sValue;
		delete[] dat->wsValue;
		dat->wsValue = NULL;

		if (!value)
			value = "";

		int len = strlen(value);
		dat->sValue = new char[len + 1];
		memcpy(dat->sValue, value, len + 1);

		dat->dataType = 1;
	}
}

void* KeyValues::operator new(size_t allocSize) noexcept
{
	static auto getKeyValuesSystem = reinterpret_cast<void* (__cdecl*)()>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "KeyValuesSystem"));
	static auto keyValuesSystem = getKeyValuesSystem();

	return VirtualMethod::call<void*, 1>(keyValuesSystem, static_cast<int>(allocSize));
}