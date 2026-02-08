#ifndef MAIN_HPP
#define MAIN_HPP
/*
	Initialize the game application and all systems
	android_asset_manager should only be set when building for android.
*/
void Game_Initialize(void* android_asset_manager = nullptr);

/*
	One single game loop
*/
void Game_Update();

/*
	Deallocate all resources and safely exit the game application
*/
void Game_Destroy();

#endif