#pragma once
#include <common.h>

namespace uknowncheats
{
	class main
	{
	public:
		explicit main() = default;
		~main() = default;

		HANDLE handle = NULL;
		HMODULE module = NULL;

	private:
	};

	inline main g_main;
}
