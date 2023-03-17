#pragma once
#include <common.h>
#include <MinHook.h>
#include <algorithm>
#include <xmmintrin.h>
#include <sstream>

namespace uknowncheats
{
	class memory
	{
	public:
		explicit memory() = default;
		~memory() = default;

		void hook_present();

		#define FIND_NT_HEADER(x) reinterpret_cast<PIMAGE_NT_HEADERS>( uint64_t(x) + ((PIMAGE_DOS_HEADER)(x))->e_lfanew )

		static __forceinline void AddPageGuardProtect(void* addr) {
			DWORD old;
			MEMORY_BASIC_INFORMATION mbi;
			SYSTEM_INFO sysInfo;

			GetSystemInfo(&sysInfo);
			VirtualQuery(addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			VirtualProtect(addr, sysInfo.dwPageSize, mbi.Protect | PAGE_GUARD, &old);
		}

		static __forceinline void RemovePageGuardProtect(void* addr) {
			DWORD old;
			MEMORY_BASIC_INFORMATION mbi;
			SYSTEM_INFO sysInfo;

			GetSystemInfo(&sysInfo);
			VirtualQuery(addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			VirtualProtect(addr, sysInfo.dwPageSize, mbi.Protect & ~PAGE_GUARD, &old);
		}

		uint64_t fix_call(uint8_t* address) {
			return reinterpret_cast<uint64_t>(address + *reinterpret_cast<int32_t*>(address + 1) + 5);
		}

		uint64_t fix_mov(uint8_t* patternMatch) {
			return reinterpret_cast<uint64_t>(patternMatch + *reinterpret_cast<int32_t*>(patternMatch + 3) + 7);
		}

		uint8_t* find_pattern(const std::string_view module, const std::string_view signature) {
			std::vector<uint8_t> signature_bytes(signature.size() + 1);

			{
				std::vector<std::string> signature_chunks{ };
				std::string current_chunk{ };

				std::istringstream string_stream{ signature.data() };

				while (std::getline(string_stream, current_chunk, ' '))
					signature_chunks.push_back(current_chunk);

				std::transform(signature_chunks.cbegin(), signature_chunks.cend(), signature_bytes.begin(), [](const std::string& val) -> uint8_t {
					return val.find('?') != std::string::npos ? 0ui8 : static_cast<uint8_t>(std::stoi(val, nullptr, 16));
					});
			}

			uint8_t* found_bytes = nullptr;

			{
				const auto image_start = reinterpret_cast<uint8_t*>(GetModuleHandleA(module.data()));
				const auto image_end = image_start + FIND_NT_HEADER(image_start)->OptionalHeader.SizeOfImage;

				const auto result = std::search(image_start, image_end, signature_bytes.cbegin(), signature_bytes.cend(), [](uint8_t left, uint8_t right) -> bool {
					return right == 0ui8 || left == right;
					});

				found_bytes = (result != image_end) ? result : nullptr;
			}

			return found_bytes;
		}

		template <typename T = uint8_t*>
		inline T sub(uint8_t* address, int offset) {
			return reinterpret_cast<T>(address) - offset;
		}


		inline uintptr_t rip(uintptr_t address) {
			return address + 4;
		}

		template <typename type>
		type as_relative(type address, int offset = 3)
		{
			return reinterpret_cast<type>(reinterpret_cast<uintptr_t>(address) + *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(address) + offset) + (offset + 4i32));
		}

		template <typename type = uintptr_t>
		type as_relative(void* address, int offset = 3)
		{
			return reinterpret_cast<type>(reinterpret_cast<uintptr_t>(address) + *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(address) + offset) + (offset + 4i32));
		}

		template <typename type = uintptr_t>
		type as_relative(uintptr_t address, int offset = 3)
		{
			return (type)(address + *reinterpret_cast<int*>(address + offset) + (offset + 4i32));
		}
	private:
	};

	inline memory g_memory;
}
