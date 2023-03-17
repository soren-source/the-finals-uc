#pragma once
#include <common.h>

namespace uknowncheats
{
	class aimbot
	{
	public:
		explicit aimbot() = default;
		~aimbot() = default;

		void tick();
	private:
	};

	inline aimbot g_aimbot;
}
