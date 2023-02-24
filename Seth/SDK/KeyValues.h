#pragma once

class KeyValues {
public:
	int keyName;
	char* sValue;
	wchar_t* wsValue;

	union
	{
		int intValue;
		float flotValue;
		void* pValue;
		unsigned char color[4];
	};

	char dataType;
	char hasEscapeSequences;
	char evaluateConditionals;
	char unused[1];

	KeyValues* peer;
	KeyValues* sub;
	KeyValues* chain;

public:
	KeyValues(const char* name) noexcept;
	void initialize(char* name) noexcept;
	KeyValues* findKey(const char* keyName, bool create = false) noexcept;
	void setString(const char* keyName, const char* value) noexcept;

	void* operator new(size_t allocSize) noexcept;
};
