#pragma once

#include "Pad.h"
#include "UtlMemory.h"
#include "UtlVector.h"

enum AttributeId
{
	isAustraliumItem = 2027,
	lootRarity = 2022,
	itemStyleOverride = 542,
	ancientPowers = 150,
	isFestive = 2053,
	killStreak = 2013,
	killStreakTier = 2025,
	sheen = 2014,
	unusualEffect = 134,
	particleEffect = 370
};

class Attribute
{
public:
	PAD(4)
	unsigned short attributeDefinitionIndex;
	float value;
	PAD(4)

	Attribute(unsigned short attributeDefinitionIndex, float value) noexcept
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

	void addAttribute(int index, float value) noexcept
	{
		//15 = MAX_ATTRIBUTES
		if (attributes.size > 14)
			return;

		Attribute attribute(index, value);

		attributes.addToTail(attribute);
		//memory->notifyManagerOfAttributeValueChanges(this);
	}
};