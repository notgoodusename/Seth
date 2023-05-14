#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

#include "../Memory.h"

class EconItemDefinition {
public:
    const char* getPlayerDisplayModel() noexcept
    {
        return baseDisplayModel;
    }
    const char* getWorldDisplayModel() noexcept
    {
        return worldDisplayModel;
    }

    PAD(8)
    unsigned short definitionIndex;
	PAD(4)
    bool enabled;
	unsigned char minItemLevel;
	unsigned char maxItemLevel;
	unsigned char itemQuality;
	unsigned char forcedItemQuality;
	unsigned char itemRarity;
	PAD(4)
	unsigned short defaultDropQuantity;
	unsigned char itemSeries;
	PAD(3)
    struct StaticAttribute
	{
		unsigned short definitionIndex;
		union AttributeDataUnion
		{
			float asFloat;
			unsigned long asUInt32;
			void* asBlobPointer;
		};
		AttributeDataUnion value;
	};

    UtlVector<StaticAttribute> staticAttributes;
    unsigned char popularitySeed;
	const char* itemBaseName;
	bool properName;
	PAD(3)
	const char* itemTypeName;
	const char* itemDesc;
	unsigned long expiration;
	const char* inventoryModel;
	const char* inventoryImage;
	UtlVector<const char*> inventoryOverlayImages;
	int inventoryImagePosition[2]; 
	int inventoryImageSize[2];
	int inspectPanelDistance;
	const char* baseDisplayModel;
	int defaultSkin;
	bool loadOnDemand;
	bool hasBeenLoaded;
	bool hideBodyGroupsDeployedOnly;
	PAD(1)
	const char* worldDisplayModel;
	const char* worldExtraWearableModel;
	const char* worldExtraWearableViewModel;
	const char* visionFilteredDisplayModel;
	const char* collectionReference;
};

class ItemSchema {
public:
    EconItemDefinition* getItemDefinition(int id) noexcept
    {
        return memory->getItemDefinition(this, id);
    }
};
