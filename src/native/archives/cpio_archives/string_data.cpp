#include "string_data.h"

std::string string_from_nul_padded_string(std::string_view data)
{
	std::string value;

	size_t namesize = data.size();

	while (data[namesize - 1] == '\0')
	{
		namesize--;
	}

	value.assign(data.data(), namesize);

	return value;
}
