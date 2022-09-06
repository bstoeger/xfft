// SPDX-License-Identifier: GPL-2.0

template<typename Command, typename... Args>
void Document::place_command(Args&&... args)
{
	place_command_internal(new Command(std::forward<Args>(args)...));
}
