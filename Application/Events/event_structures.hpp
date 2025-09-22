#ifndef EVENT_STRUCTURES_HPP
#define EVENT_STRUCTURES_HPP

struct Window_Resized
{
	int width{}; int height{};
};

struct Level_Editor_Toggled
{
	bool isToggled{};
};


#endif