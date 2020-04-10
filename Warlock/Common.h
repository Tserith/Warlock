#pragma once

#define MAX_NAME_SIZE 64
#define MAGIC_VALUE 0xBABECAFE

enum class messageType
{
	received,
	setTarget,
	getInfo,
	rwRequest
};

enum class requestType
{
	read,
	write
};

struct WarlockComms
{
	messageType type;
	union
	{
		struct
		{
			requestType type;
			void* addr;
			unsigned long long value;
		}request;
		struct  
		{
			void* baseAddr;
			void* pid;
		}info;
		struct  
		{
			void* pid;
			wchar_t targetImage[MAX_NAME_SIZE + 1];
		}target;
	};
};