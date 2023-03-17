#pragma once
#include <common.h>
#include <MinHook.h>

namespace uknowncheats
{
	class hooking
	{
	public:
		explicit hooking() = default;
		~hooking() = default;

		void hook_present();
	private:
	};

	inline hooking g_hooking;
}
