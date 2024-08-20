module;
#include <Framework/CubismFramework.hpp>
module Live2D;

using namespace Csm;
using namespace L2D;

void* Allocator::Allocate(const csmSizeType size)
{
	return malloc(size);
}

void Allocator::Deallocate(void* memory)
{
	free(memory);
}

void* Allocator::AllocateAligned(const csmSizeType size, const csmUint32 alignment)
{
	auto offset = alignment - 1 + sizeof(void*);
	auto allocation = Allocate(size + static_cast<csmUint32>(offset));
	auto alignedAddress = reinterpret_cast<size_t>(allocation) + sizeof(void*);
	if (auto shift = alignedAddress % alignment) alignedAddress += (alignment - shift);
	((void**)alignedAddress)[-1] = allocation;
	return (void*)alignedAddress;
}

void Allocator::DeallocateAligned(void* alignedMemory)
{
	Deallocate(((void**)alignedMemory)[-1]);
}