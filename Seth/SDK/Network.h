#pragma once

#include "BitBuffer.h"
#include "Pad.h"
#include "VirtualMethod.h"

#include <functional>

#define MULTIPLAYER_BACKUP 90
#define WEAPON_SUBTYPE_BITS	6
#define	MAX_OSPATH		260			// max length of a filesystem pathname
#define MAX_STREAMS			2    
#define NET_FRAMES_BACKUP	64		// must be power of 2
#define NET_FRAMES_MASK		(NET_FRAMES_BACKUP-1)
#define MAX_SUBCHANNELS		8		// we have 8 alternative send&wait bits
#define SUBCHANNEL_FREE		0	// subchannel is free to use
#define SUBCHANNEL_TOSEND	1	// subchannel has data, but not send yet
#define SUBCHANNEL_WAITING	2   // sbuchannel sent data, waiting for ACK
#define SUBCHANNEL_DIRTY	3	// subchannel is marked as dirty during changelevel
#define DELTA_OFFSET_BITS 5
#define DELTA_OFFSET_MAX ( ( 1 << DELTA_OFFSET_BITS ) - 1 )
#define DELTASIZE_BITS 20	
#define NUM_NEW_COMMAND_BITS		4
#define MAX_NEW_COMMANDS			((1 << NUM_NEW_COMMAND_BITS)-1)
#define NUM_BACKUP_COMMAND_BITS		3
#define MAX_BACKUP_COMMANDS			((1 << NUM_BACKUP_COMMAND_BITS)-1)
#define MAX_COMMANDS				MAX_NEW_COMMANDS + MAX_BACKUP_COMMANDS
#define MAX_PLAYER_NAME_LENGTH		32
#define MAX_CUSTOM_FILES		4		// max 4 files
#define NETMSG_TYPE_BITS	6	// must be 2^NETMSG_TYPE_BITS > SVC_LASTMSG
#define net_NOP 0        // nop command used for padding
#define net_Disconnect 1 // disconnect, last message in connection
#define net_File 2       // file transmission message request/deny
#define net_Tick 3        // send last world tick
#define net_StringCmd 4   // a string command
#define net_SetConVar 5   // sends one/multiple convar settings
#define net_SignonState 6 // signals current signon state
#define clc_ClientInfo 8       
#define clc_Move 9             
#define clc_VoiceData 10       
#define clc_BaselineAck 11     
#define clc_ListenEvents 12    
#define clc_RespondCvarValue 13
#define clc_FileCRCCheck 14    
#define clc_CmdKeyValues 16    
#define FLOW_OUTGOING	0		
#define FLOW_INCOMING	1
#define MAX_FLOWS		2		// in & out

class NetworkChannel
{
public:
	enum
	{
		GENERIC = 0,	// must be first and is default group
		LOCALPLAYER,	// bytes for local player entity update
		OTHERPLAYERS,	// bytes for other players update
		ENTITIES,		// all other entity bytes
		SOUNDS,			// game sounds
		EVENTS,			// event messages
		TEMPENTS,		// temp entities
		USERMESSAGES,	// user messages
		ENTMESSAGES,	// entity messages
		VOICE,			// voice data
		STRINGTABLE,	// a stringtable update
		MOVE,			// client move cmds
		STRINGCMD,		// string command
		SIGNON,			// various signondata
		TOTAL,			// must be last and is not a real group
	};

    VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))
    VIRTUAL_METHOD(bool, sendNetMsg, 37, (void* msg, bool forceReliable = false, bool voice = false), (this, msg, forceReliable, voice))

	PAD(8)
    int outSequenceNr;
    int inSequenceNr;
    int outSequenceNrAck;
    int outReliableState;
    int inReliableState;
    int chokedPackets;
};

class NetworkMessageBase
{
public:
	virtual ~NetworkMessageBase() { };

	virtual void setNetChannel(NetworkChannel* networkChannel) = 0; // netchannel this message is from/for
	virtual void setReliable(bool state) = 0;             // set to true if it's a reliable message

	virtual bool process(void) = 0; // calles the recently set handler to process this message

	virtual bool readFromBuffer(bufferRead& buffer) = 0; // returns true if parsing was OK
	virtual bool writeToBuffer(bufferWrite& buffer) = 0; // returns true if writing was OK

	virtual bool isReliable(void) const = 0; // true, if message needs reliable handling

	virtual int getType(void) const = 0;         // returns module specific header tag eg svc_serverinfo
	virtual int getGroup(void) const = 0;        // returns net message group of this message
	virtual const char* getName(void) const = 0; // returns network message name, eg "svc_serverinfo"
	virtual NetworkChannel* getNetChannel(void) const = 0;
	virtual const char* toString(void) const = 0; // returns a human readable string about message content
};

class NetworkMessage : public NetworkMessageBase
{
public:
	NetworkMessage()
	{
		reliable = true;
		networkChannel = nullptr;
	}

	virtual int getGroup() const { return NetworkChannel::GENERIC; }
	NetworkChannel* getNetChannel() const { return networkChannel; }

	virtual void setReliable(bool state) { reliable = state; };
	virtual bool isReliable() const { return reliable; };
	virtual void setNetChannel(NetworkChannel* netchan) { networkChannel = netchan; }
	virtual bool process() { return false; }; // no handler set

protected:
	bool reliable;          // true if message should be send reliable
	NetworkChannel* networkChannel; // netchannel this message is from/for
};

class CLC_Move : public NetworkMessage
{
public:
	bool readFromBuffer(bufferRead& buffer)
	{
		newCommands = buffer.readUBitLong(NUM_NEW_COMMAND_BITS);
		backupCommands = buffer.readUBitLong(NUM_BACKUP_COMMAND_BITS);
		length = buffer.readWord();
		dataIn = buffer;
		return buffer.seekRelative(length);
	}

	bool writeToBuffer(bufferWrite& buffer)
	{
		buffer.writeUBitLong(getType(), NETMSG_TYPE_BITS);
		length = dataOut.getNumBitsWritten();

		buffer.writeUBitLong(newCommands, NUM_NEW_COMMAND_BITS);
		buffer.writeUBitLong(backupCommands, NUM_BACKUP_COMMAND_BITS);

		buffer.writeWord(length);

		return buffer.writeBits(dataOut.getData(), length);
	}

	const char* toString() const { return "CLC_Move"; }
	int getType() const { return clc_Move; }
	const char* getName() const { return "clc_Move"; }
	void* messageHandler;
	int getGroup() const { return NetworkChannel::MOVE; }
	CLC_Move() { reliable = false; }

public:
	int backupCommands;
	int newCommands;
	int length;
	bufferRead dataIn;
	bufferWrite dataOut;
};