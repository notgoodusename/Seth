#pragma once

#include "Pad.h"
#include "UtlMemory.h"
#include "UtlVector.h"

class Attribute
{
public:
	PAD(4)
	unsigned short attributeDefinitionIndex;
	float value;
	PAD(4)

	inline Attribute(unsigned short attributeDefinitionIndex, float value) noexcept
	{
		this->attributeDefinitionIndex = attributeDefinitionIndex;
		this->value = value;
	}
};

class AttributeList
{
public:
	PAD(4)
	UtlVector<Attribute> attributes;

	inline void addAttribute(int index, float value) noexcept
	{
		//15 = MAX_ATTRIBUTES
		if (attributes.size > 14)
			return;

		Attribute attribute(index, value);

		attributes.addToTail(attribute);
	}
};