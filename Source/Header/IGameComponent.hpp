/*
	Base class for all game components, implementing common properties
*/

#ifndef I_GAME_COMPONENT_HPP
#define I_GAME_COMPONENT_HPP
class IGameComponent
{
public: 
	virtual ~IGameComponent() = default;
	//Whether component data has been initialized
	bool isInitialized = false;
	//Whether component is active/enabled.
	bool isActive = false;
};


#endif