#pragma once
#include <common.h>

namespace uknowncheats
{
	class pointers
	{
	public:
		explicit pointers() = default;
		~pointers() = default;

		void scan_patterns();
		uintptr_t steam_overlay_present;
	private:
	};

	inline pointers g_pointers;
}
