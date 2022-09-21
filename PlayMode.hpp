#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void PlayMode::game_end(bool didWin);
	void PlayMode::key_pressed(int color);

	//----- game state -----
	enum GameState {
		PLAYING,
		GAMEOVER,
		WAITING,
	} gameState;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//in-game objects to be managed:
	Sound::Sample red_sound;
	Sound::Sample green_sound;
	Sound::Sample blue_sound;
	Sound::Sample yellow_sound;
	Sound::Sample purple_sound;

	Scene::Drawable *texts[5][5];
	Scene::Transform *gameWin;
	Scene::Transform *gameLose;

	glm::vec3 winlose_pos = glm::vec3(0);

	int activeTextRow = 0;
	int activeTextCol = 0;
	bool gotCorrect = true;

	glm::vec3 text_display_pos[5] = {glm::vec3(0)};
	glm::vec3 text_reset_pos = glm::vec3(0.0f, 0.0f, -5.0f);
	
	float elapsed_time_since = 0;
	float color_interval = 3;
	float total_time = 60;

	//background music
	std::shared_ptr< Sound::PlayingSample > bg_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
