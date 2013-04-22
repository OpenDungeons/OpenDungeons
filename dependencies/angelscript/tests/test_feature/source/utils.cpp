#include "utils.h"

#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif
#include <map>

using namespace std;




void Assert(asIScriptGeneric *gen)
{
	bool expr;
	if( sizeof(bool) == 1 )
		expr = gen->GetArgByte(0) ? true : false;
	else
		expr = gen->GetArgDWord(0) ? true : false;
	if( !expr )
	{
		printf("--- Assert failed ---\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			const asIScriptFunction *function = ctx->GetFunction();
			if( function != 0 )
			{
				printf("func: %s\n", function->GetDeclaration());
				printf("mdle: %s\n", function->GetModuleName());
				printf("sect: %s\n", function->GetScriptSectionName());
			}
			printf("line: %d\n", ctx->GetLineNumber());
			ctx->SetException("Assert failed");
			printf("---------------------\n");
		}
	}
}

//#define TRACK_LOCATIONS
//#define TRACK_SIZES

static int numAllocs            = 0;
static int numFrees             = 0;
static size_t currentMemAlloc   = 0;
static size_t maxMemAlloc       = 0;
static int maxNumAllocsSameTime = 0;
static asQWORD sumAllocSize     = 0;

static map<void*,size_t> memSize;
static map<void*,int> memCount;

#ifdef TRACK_SIZES
static map<size_t,int> meanSize;
#endif

#ifdef TRACK_LOCATIONS
struct loc
{
	const char *file;
	int line;

	bool operator <(const loc &other) const
	{
		if( file < other.file ) return true;
		if( file == other.file && line < other.line ) return true;
		return false;
	}
};

static map<loc, int> locCount;
#endif

void *MyAllocWithStats(size_t size, const char *file, int line)
{
	// Avoid compiler warning when variables aren't used
	UNUSED_VAR(line);
	UNUSED_VAR(file);

	// Allocate the memory
	void *ptr = malloc(size);

	// Count number of allocations made
	numAllocs++;

	// Count total amount of memory allocated
	sumAllocSize += size;

	// Update currently allocated memory
	currentMemAlloc += size;
	if( currentMemAlloc > maxMemAlloc ) maxMemAlloc = currentMemAlloc;

	// Remember the size of the memory allocated at this pointer
	memSize.insert(map<void*,size_t>::value_type(ptr,size));

	// Remember the currently allocated memory blocks, with the allocation number so that we can debug later
	memCount.insert(map<void*,int>::value_type(ptr,numAllocs));

	// Determine the maximum number of allocations at the same time
	if( numAllocs - numFrees > maxNumAllocsSameTime )
		maxNumAllocsSameTime = numAllocs - numFrees;

#ifdef TRACK_SIZES
	// Determine the mean size of the memory allocations
	map<size_t,int>::iterator i = meanSize.find(size);
	if( i != meanSize.end() )
		i->second++;
	else
		meanSize.insert(map<size_t,int>::value_type(size,1));
#endif

#ifdef TRACK_LOCATIONS
	// Count the number of allocations for each location in the library
	loc l = {file, line};
	map<loc, int>::iterator i2 = locCount.find(l);
	if( i2 != locCount.end() )
		i2->second++;
	else
		locCount.insert(map<loc,int>::value_type(l,1));
#endif

	return ptr;
}

void MyFreeWithStats(void *address)
{
	// Count the number of deallocations made
	numFrees++;

	// Remove the memory block from the list of allocated blocks
	map<void*,size_t>::iterator i = memSize.find(address);
	if( i != memSize.end() )
	{
		// Decrease the current amount of allocated memory
		currentMemAlloc -= i->second;
		memSize.erase(i);
	}
	else
		assert(false);

	// Verify which memory we are currently removing so we know we did the allocation, and where it was allocated
	map<void*,int>::iterator i2 = memCount.find(address);
	if( i2 != memCount.end() )
	{
//		int numAlloc = i2->second;
		memCount.erase(i2);
	}
	else
		assert(false);

	// Free the actual memory
	free(address);
}

void InstallMemoryManager()
{
#ifdef TRACK_LOCATIONS
	assert( strstr(asGetLibraryOptions(), " AS_DEBUG ") );
#endif

	asSetGlobalMemoryFunctions((asALLOCFUNC_t)MyAllocWithStats, MyFreeWithStats);
}

void PrintAllocIndices()
{
	map<void*,int>::iterator i = memCount.begin();
	while( i != memCount.end() )
	{
		printf("%d\n", i->second);
		i++;
	}
}

void RemoveMemoryManager()
{
	asThreadCleanup();

	PrintAllocIndices();

//	assert( numAllocs == numFrees );
//	assert( currentMemAlloc == 0 );

	printf("---------\n");
	printf("MEMORY STATISTICS\n");
	printf("number of allocations                 : %d\n", numAllocs);                   // 125744
	printf("max allocated memory at any one time  : %d\n", (int)maxMemAlloc);                 // 121042
	printf("max number of simultaneous allocations: %d\n", maxNumAllocsSameTime);        // 2134
	printf("total amount of allocated memory      : %d\n", (int)sumAllocSize);                // 10106765
	printf("medium size of allocations            : %d\n", numAllocs ? (int)sumAllocSize/numAllocs : 0);

#ifdef TRACK_SIZES
	// Find the mean size of allocations
	map<size_t,int>::iterator i = meanSize.begin();
	int n = 0;
	int meanAllocSize = 0;
	while( i != meanSize.end() )
	{
		if( n + i->second > numAllocs / 2 )
		{
			meanAllocSize = (int)i->first;
			break;
		}

		n += i->second;
		i++;
	}
	printf("mean size of allocations              : %d\n", meanAllocSize);
	printf("smallest allocation size              : %d\n", meanSize.begin()->first);
	printf("largest allocation size               : %d\n", meanSize.rbegin()->first);
	printf("number of different allocation sizes  : %d\n", meanSize.size());

	// Print allocation sizes
	i = meanSize.begin();
	while( i != meanSize.end() )
	{
		if( i->second >= 1000 )
			printf("alloc size %d: %d\n", i->first, i->second);
		i++;
	}
#endif

#ifdef TRACK_LOCATIONS
	// Print allocation counts per location
	map<loc,int>::iterator i2 = locCount.begin();
	while( i2 != locCount.end() )
	{
		const char *file  = i2->first.file;
		int         line  = i2->first.line;
		int         count = i2->second;
		printf("%s (%d): %d\n", file, line, count);
		i2++;
	}
#endif

	asResetGlobalMemoryFunctions();
}

int GetNumAllocs()
{
	return numAllocs;
}


asDWORD ComputeCRC32(const asBYTE *buf, asUINT length)
{
	// Compute the lookup table
	asDWORD lup[256];
	for( asUINT pos = 0; pos < 256; pos++ )
	{
		asDWORD val = pos;
		for( int i = 8; i > 0; i-- )
		{
			if( val & 1 )
				val = (val >> 1) ^ 0xEDB88320;
			else
				val >>= 1;
		}
		lup[pos] = val;
	}

	// Calculate the CRC32 value
	asDWORD crc = 0xFFFFFFFF;
	for( asUINT i = 0; i < length; i++ )
	{
		crc = ((crc) >> 8) ^ lup[*buf++ ^ (crc & 0x000000FF)];
	}

	return ~crc;
}
