#ifndef EVENT_STRUCTURES_HPP
#define EVENT_STRUCTURES_HPP

struct Event_Window_Resized
{
	int width{}; int height{};
};

struct Event_Window_Should_Close
{

};

struct Event_Level_Editor_Toggled
{
	bool is_enabled{};
};

struct Event_Level_Editor_Simulation
{
	bool is_enabled{};
};

struct Button_Event {
	std::string Event_Name;
};

struct Menu_Event {
	std::string Menu_Name;
	std::string Event_Name;
};

struct Pause_Event
{
	bool is_pause{};
};


#endif