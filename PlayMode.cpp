#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > strooperz_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("strooperz.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > strooperz_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("strooperz.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = strooperz_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > bg_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("bg.wav"));
});

Load< Sound::Sample > red_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("red.wav"));
});

Load< Sound::Sample > green_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("green.wav"));
});

Load< Sound::Sample > blue_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("blue.wav"));
});

Load< Sound::Sample > yellow_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("yellow.wav"));
});

Load< Sound::Sample > purple_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("purple.wav"));
});

PlayMode::PlayMode() : scene(*strooperz_scene), red_sound(*red_sample), 
						green_sound(*green_sample), blue_sound(*blue_sample), 
						yellow_sound(*yellow_sample), purple_sound(*purple_sample) {

	gameState = PLAYING;

	for (int j = 0; j < 5; j++) {
		for (int i = 0; i < 5; i ++) {
			texts[j][i] = nullptr;
		}
	}

	auto get_color_index = [](char color) {
		if (color == 'R') return 0;
		else if (color == 'G') return 1;
		else if (color == 'B') return 2;
		else if (color == 'Y') return 3;
		else if (color == 'P') return 4;

		return -1;
	};

	for (auto &drawable : scene.drawables) {
		std::string dname = drawable.transform->name;
		if (dname.length() == 2) {
			text_display_pos[get_color_index(dname[0])] = drawable.transform->position;
			texts[get_color_index(dname[0])][get_color_index(dname[1])] = &drawable;
		}
	}

	for (auto &transform : scene.transforms) {
		if (transform.name.length() == 2) {
			transform.position = text_reset_pos;
		}
		if (transform.name == "GameWin") {
			winlose_pos = transform.position;

			gameWin = &transform;
			transform.position = text_reset_pos;
		}
		else if (transform.name == "GameLose") {
			gameLose = &transform;
			transform.position = text_reset_pos;
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}
	//start bg loop playing:)
	bg_loop = Sound::loop(*bg_sample);
}

PlayMode::~PlayMode() {
}

void PlayMode::game_end(bool didWin) {

	gameState = GAMEOVER;
	
	if (didWin) gameWin->position = winlose_pos;
	else gameLose->position = winlose_pos;
}

void PlayMode::key_pressed(int color) {
	if (activeTextCol == color) {
		gameState = WAITING;
		gotCorrect = true;
		texts[activeTextRow][activeTextCol]->transform->position = text_reset_pos;
	}
	else if (gameState == PLAYING) game_end(false);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			key_pressed(0);
			return true;
		} else if (evt.key.keysym.sym == SDLK_f) {
			key_pressed(1);
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			key_pressed(2);
			return true;
		} else if (evt.key.keysym.sym == SDLK_j) {
			key_pressed(3);
			return true;
		} else if (evt.key.keysym.sym == SDLK_k) {
			key_pressed(4);
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (gameState == GAMEOVER) return;

	elapsed_time_since += elapsed;
	total_time -= elapsed;
	if (total_time <= 0.0f) {
		game_end(true);
	}

	if (elapsed_time_since > color_interval) {

		if (!gotCorrect) {
			game_end(false);
			return;
		}

		gotCorrect = false;
		elapsed_time_since = 0;
		color_interval = color_interval <= 0.8f ? 0.8f : color_interval - 0.1f;

		// random number generation based on https://stackoverflow.com/questions/5008804/generating-a-random-integer-from-a-range
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<int> row(0, 4);
		std::uniform_int_distribution<int> col(0, 4);
		std::uniform_int_distribution<int> audio(0, 4);
		
		activeTextRow = row(rng);
		activeTextCol = col(rng);

		texts[activeTextRow][activeTextCol]->transform->position = text_display_pos[activeTextRow];

		switch (audio(rng)) {
			case 0:
				Sound::play(red_sound);
				break;
			case 1:
				Sound::play(blue_sound);
				break;
			case 2:
				Sound::play(green_sound);
				break;
			case 3:
				Sound::play(yellow_sound);
				break;
			case 4:
				Sound::play(purple_sound);
				break;
			default:
				break;
		}

		gameState = PLAYING;
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		float ofs = 2.0f / drawable_size.y;

		if (gameState != GAMEOVER) {
			lines.draw_text("D/F/Space/J/K for colors",
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("D/F/Space/J/K for colors",
				glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));

			float remaining_time = color_interval - elapsed_time_since;
			std::string time = std::to_string(static_cast<int>(remaining_time)) + "." + std::to_string(static_cast<int>(remaining_time * 10) % 10) + std::to_string(static_cast<int>(remaining_time * 100) % 10);
			lines.draw_text(time,
				glm::vec3(-aspect + 5.0f * H, -1.0 + 10.0f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text(time,
				glm::vec3(-aspect + 5.0f * H + ofs, -1.0 + + 10.0f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));

			lines.draw_text("Time : " + std::to_string(static_cast<int>(total_time)),
				glm::vec3(aspect - 4.0f * H, -1.0 + 0.1f * H, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			lines.draw_text("Time : " + std::to_string(static_cast<int>(total_time)),
				glm::vec3(aspect - 4.0f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		} else {
			lines.draw_text("Gameover!",
					glm::vec3(-aspect + 2.0f * H, -1.0 + 10.0f * H, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0x00, 0x00, 0x00, 0x00));
				lines.draw_text("Gameover!",
					glm::vec3(-aspect + 2.0f * H + ofs, -1.0 + + 10.0f * H + ofs, 0.0),
					glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
					glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
	GL_ERRORS();
}
