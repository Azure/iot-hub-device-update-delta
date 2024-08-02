#pragma once

namespace archive_diff
{

enum class cpio_format
{
	none = 0,
	binary,
	ascii,
	new_ascii,
	newc_ascii,
};

}