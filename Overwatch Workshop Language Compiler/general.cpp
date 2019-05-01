#include "general.h"

#define abstract_min(a, b) (a < b ? a : b)
#define abstract_max(a, b) (a > b ? a : b)

float min(float a, float b)
{
	return abstract_min(a, b);
}

double min(double a, double b)
{
	return abstract_min(a, b);
}

u8 min(u8 a, u8 b)
{
	return abstract_min(a, b);
}

u16 min(u16 a, u16 b)
{
	return abstract_min(a, b);
}

u32 min(u32 a, u32 b)
{
	return abstract_min(a, b);
}

u64 min(u64 a, u64 b)
{
	return abstract_min(a, b);
}

float max(float a, float b)
{
	return abstract_max(a, b);
}

double max(double a, double b)
{
	return abstract_max(a, b);
}

u8 max(u8 a, u8 b)
{
	return abstract_max(a, b);
}

u16 max(u16 a, u16 b)
{
	return abstract_max(a, b);
}

u32 max(u32 a, u32 b)
{
	return abstract_max(a, b);
}

u64 max(u64 a, u64 b)
{
	return abstract_max(a, b);
}
