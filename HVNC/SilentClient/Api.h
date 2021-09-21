#pragma once
#include <windows.h>

typedef NTSTATUS (WINAPI* fRtlGetCompressionWorkSpaceSize)(
	USHORT CompressionFormatAndEngine,
	PULONG CompressBufferWorkSpaceSize,
	PULONG CompressFragmentWorkSpaceSize
);

typedef NTSTATUS (WINAPI* fRtlCompressBuffer)(
	USHORT CompressionFormatAndEngine,
	PUCHAR UncompressedBuffer,
	ULONG  UncompressedBufferSize,
	PUCHAR CompressedBuffer,
	ULONG  CompressedBufferSize,
	ULONG  UncompressedChunkSize,
	PULONG FinalCompressedSize,
	PVOID  WorkSpace
);

static fRtlGetCompressionWorkSpaceSize pRtlGetCompressionWorkSpaceSize = (fRtlGetCompressionWorkSpaceSize)GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlGetCompressionWorkSpaceSize");
static fRtlCompressBuffer pRtlCompressBuffer = (fRtlCompressBuffer)GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlCompressBuffer");