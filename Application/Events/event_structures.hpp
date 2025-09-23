#ifndef EVENT_STRUCTURES_HPP
#define EVENT_STRUCTURES_HPP

struct Event_Window_Resized
{
	int width{}; int height{};
};

struct Event_Level_Editor_Toggled
{
	bool is_enabled{};
};


#endif