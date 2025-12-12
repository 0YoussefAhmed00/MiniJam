#include "Game.h"
#include "World.h"
#include "OptionsUI.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unordered_map>

using namespace sf;

constexpr float PPM = 30.f;
constexpr float INV_PPM = 1.f / PPM;
std::unordered_map<std::string, std::shared_ptr<AudioEmitter>> m_playerEmitters;

static float randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
static float randomOffset(float magnitude) {
	return ((rand() % 2000) / 1000.f - 1.f) * magnitude;
}

void Game::MyContactListener::BeginContact(b2Contact* contact) {
	b2Fixture* a = contact->GetFixtureA();
	b2Fixture* b = contact->GetFixtureB();
	if (a == footFixture && !b->IsSensor()) footContacts++;
	if (b == footFixture && !a->IsSensor()) footContacts++;
}
void Game::MyContactListener::EndContact(b2Contact* contact) {
	b2Fixture* a = contact->GetFixtureA();
	b2Fixture* b = contact->GetFixtureB();
	if (a == footFixture && !b->IsSensor()) footContacts--;
	if (b == footFixture && !a->IsSensor()) footContacts--;
	if (footContacts < 0) footContacts = 0;
}




Game::Game()
	: m_window(VideoMode::getDesktopMode(),
		"SFML + Box2D + AudioManager + Persona Demo",
		Style::Fullscreen),
	m_camera(FloatRect(0, 0,
		1920,
		1080)),
	m_defaultView(m_window.getDefaultView()),
	m_gravity(0.f, 20.f),
	m_world(m_gravity),
	m_player(nullptr),
	m_diagMark(8.f),
	m_fxMark(8.f),
	psychoMode(false),
	nextPsychoSwitch(0.f),
	splitMode(false),
	splitDuration(0.f),
	nextSplitCheck(0.f),
	inTransition(false),
	pendingEnable(false),
	inputLocked(false),
	inputLockPending(false),
	nextInputLockCheck(0.f),
	m_running(true),
	m_state(GameState::MENU),
	m_lastAppliedAudioState(PlayerAudioState::Neutral)
{
	srand(static_cast<unsigned>(time(nullptr)));
	m_window.setFramerateLimit(60);
	m_world.SetContactListener(&m_contactListener);
	m_worldView = std::make_unique<World>(m_world);

	// Store World (creates obstacles and holds category bits)
	m_worldView = std::make_unique<World>(m_world);

	// Ground (Box2D)
	b2BodyDef groundDef;
	groundDef.type = b2_staticBody;
	groundDef.position.Set(640 * INV_PPM, 880 * INV_PPM);
	b2Body* ground = m_world.CreateBody(&groundDef);

	b2PolygonShape groundBox;
	groundBox.SetAsBox(2000.0f * 10 * INV_PPM, 10.0f * INV_PPM);

	b2FixtureDef groundFix;
	groundFix.shape = &groundBox;
	groundFix.filter.categoryBits = m_worldView->CATEGORY_GROUND;
	groundFix.filter.maskBits = m_worldView->CATEGORY_PLAYER | m_worldView->CATEGORY_SENSOR | m_worldView->CATEGORY_OBSTACLE;
	ground->CreateFixture(&groundFix);

	// Player
	m_player = std::make_unique<Player>(&m_world, 140.f, 800.f);
	m_contactListener.footFixture = m_player->GetFootFixture();

	// Audio setup
	m_diagMark.setFillColor(Color::Yellow);
	m_diagMark.setOrigin(8.f, 8.f);
	m_fxMark.setFillColor(Color::Cyan);
	m_fxMark.setOrigin(8.f, 8.f);

	m_dialogueEmitter = std::make_shared<AudioEmitter>();
	m_dialogueEmitter->id = "dialogue1";
	m_dialogueEmitter->category = AudioCategory::Dialogue;
	m_dialogueEmitter->position = b2Vec2((640 - 100) * INV_PPM, (680 - 50) * INV_PPM);
	m_dialogueEmitter->minDistance = 0.5f;
	m_dialogueEmitter->maxDistance = 20.f;
	m_dialogueEmitter->baseVolume = 1.f;

	m_effectEmitter = std::make_shared<AudioEmitter>();
	m_effectEmitter->id = "effect1";
	m_effectEmitter->category = AudioCategory::Effects;
	m_effectEmitter->position = b2Vec2((640 + 100) * INV_PPM, (680 - 50) * INV_PPM);
	m_effectEmitter->minDistance = 0.5f;
	m_effectEmitter->maxDistance = 20.f;
	m_effectEmitter->baseVolume = 1.f;


	// player reply emitter (just like refuse)
	m_playerReply = std::make_shared<AudioEmitter>();
	m_playerReply->id = "playerReply";
	m_playerReply->category = AudioCategory::Dialogue;
	m_playerReply->minDistance = 0.5f;
	m_playerReply->maxDistance = 20.f;
	m_playerReply->baseVolume = 1.f;
	m_playerReply->position = b2Vec2(0.f, 0.f); // optional: track player position if you want
	if (!m_playerReply->loadBuffer("assets/Audio/player_reply.wav")) {
		std::cerr << "Warning: player reply audio not loaded\n";
	}
	m_playerReply->sound.setLoop(false);
	m_audio.RegisterEmitter(m_playerReply);

	// find the grocery obstacle by filename substring (change "grocery" to match your filename)
	m_groceryObstacleIndex = m_worldView->findObstacleByTextureSubstring("grocery");

	if (m_groceryObstacleIndex >= 0)
	{
		auto makeGroceryEmitter = [&](const std::string& id, const std::string& filePath)->std::shared_ptr<AudioEmitter> {
			auto e = std::make_shared<AudioEmitter>();
			e->id = id;
			e->category = AudioCategory::Dialogue; // grocery is dialogue-like
			e->minDistance = 0.5f;
			e->maxDistance = 50.f;
			e->baseVolume = 1.f;
			// initial position (will be updated each frame in update())
			b2Vec2 p = m_worldView->getObstacleBodyPosition(m_groceryObstacleIndex);
			e->position = p;
			if (!e->loadBuffer(filePath)) {
				std::cerr << "Warning: grocery audio not loaded: " << filePath << "\n";
			}
			e->sound.setLoop(false);
			m_audio.RegisterEmitter(e);
			return e;
			};

		// filenames — create these WAV/OGG files in your assets folder
		m_groceryA = makeGroceryEmitter("groceryA", "assets/Audio/grocery_line1.wav");
		m_groceryB = makeGroceryEmitter("groceryB", "assets/Audio/grocery_line2.wav");
		m_groceryCollision = makeGroceryEmitter("groceryCollision", "assets/Audio/grocery_collision.wav");

		m_groceryClock.restart();
		m_nextGroceryLineTime = randomFloat(5.f, 10.f);
	}

	// --- Bus visual + audio setup ---
	if (!m_busTexture.loadFromFile("Assets/Obstacles/bus.png")) {
		std::cerr << "Warning: bus texture not loaded (Assets/Obstacles/bus.png)\n";
	}
	m_busSpawnClock.restart();
	m_busTravelTime = 3.5f;         // enforce 2 seconds travel
	m_busSpawnInterval = 16.f;     // 30 seconds between spawns

	// Create/register a single bus emitter (reused for each pass).
	m_busEmitter = std::make_shared<AudioEmitter>();
	m_busEmitter->id = "bus_pass";
	m_busEmitter->category = AudioCategory::Dialogue; // or Dialogue depending on your mixer
	m_busEmitter->minDistance = 0.5f;
	m_busEmitter->maxDistance = 50.f;
	m_busEmitter->baseVolume = 1.f;
	m_busEmitter->position = b2Vec2(0.f, 0.f);
	if (!m_busEmitter->loadBuffer(m_busAudioPath)) {
		std::cerr << "Warning: bus pass audio not loaded: " << m_busAudioPath << "\n";
	}
	m_busEmitter->sound.setLoop(false);
	m_audio.RegisterEmitter(m_busEmitter);



	m_audio.SetCrossfadeTime(1.2f);
	bool ok = m_audio.loadMusic("Assets/Audio/music_neutral.ogg", "Assets/Audio/music_crazy.ogg");
	if (!ok) std::cerr << "Warning: music not loaded. Replace file paths with your assets.\n";

	if (!m_dialogueEmitter->loadBuffer("assets/Audio/dialogue.wav"))
		std::cerr << "Warning: dialogue.wav not loaded.\n";
	if (!m_effectEmitter->loadBuffer("assets/Audio/effect.wav"))
		std::cerr << "Warning: effect.wav not loaded.\n";

	m_audio.RegisterEmitter(m_dialogueEmitter);
	m_audio.RegisterEmitter(m_effectEmitter);
	m_dialogueEmitter->sound.setLoop(true);
	m_effectEmitter->sound.setLoop(true);

	m_audio.SetMasterVolume(1.f);
	m_audio.SetMusicVolume(0.9f);
	m_audio.SetBackgroundVolume(0.6f);
	m_audio.SetDialogueVolume(1.0f);
	m_audio.SetEffectsVolume(1.f);

	// --- Player emitters ---
	auto createPlayerEmitter = [&](const std::string& id, const std::string& filePath) {
		auto emitter = std::make_shared<AudioEmitter>();
		emitter->id = id;
		emitter->category = AudioCategory::Dialogue; // <-- make it dialogue so dialogue volume affects it
		emitter->position = m_player->GetBody()->GetPosition();
		emitter->minDistance = 0.5f;
		emitter->maxDistance = 20.f;
		emitter->baseVolume = 1.f;

		if (!emitter->loadBuffer(filePath))
			std::cerr << "Warning: " << filePath << " not loaded.\n";

		emitter->sound.setLoop(false);
		m_audio.RegisterEmitter(emitter);
		m_playerEmitters[id] = emitter;
		};

	createPlayerEmitter("refuse", "assets/Audio/refuse.wav");
	createPlayerEmitter("player_reply", "assets/Audio/player_reply.wav");
	auto dbgIt = m_playerEmitters.find("player_reply");
	if (dbgIt == m_playerEmitters.end() || !dbgIt->second || !dbgIt->second->buffer) {
		std::cerr << "DEBUG: player_reply emitter missing or buffer not loaded. Check path & case sensitivity.\n";
	}
	else {
		std::cerr << "DEBUG: player_reply emitter loaded successfully.\n";
	}

	//createPlayerEmitter("jump", "assets/Audio/jump.wav");
	//createPlayerEmitter("attack", "assets/Audio/attack.wav");
	//// Add more player sounds as needed

	nextPsychoSwitch = randomFloat(6.f, 8.f);
	psychoClock.restart();

	nextSplitCheck = randomFloat(1.f, 3.f);
	splitClock.restart();

	nextInputLockCheck = randomFloat(3.f, 6.f);
	inputLockClock.restart();

	if (!m_font.loadFromFile("assets/Font/Myriad Arabic Regular.ttf")) {
		std::cerr << "Warning: font not loaded (assets/arial.ttf)\n";
	}
	m_debugText.setFont(m_font);
	m_debugText.setCharacterSize(16);
	m_debugText.setFillColor(Color::White);
	m_debugText.setPosition(10.f, 10.f);

	m_mainMenu = std::make_unique<MainMenu>(m_window.getSize());
	m_mainMenu->SetFont(&m_font);
	m_mainMenu->BuildLayout();

	// Pause UI init
	m_pauseOverlay.setSize(Vector2f((float)m_window.getSize().x, (float)m_window.getSize().y));
	m_pauseOverlay.setFillColor(Color(0, 0, 0, 160));

	// Create simple resume/back buttons using MenuButton style
	m_pauseResumeButton = std::make_unique<MenuButton>(m_font, "Resume", Vector2f((float)m_window.getSize().x * 0.5f, (float)m_window.getSize().y * 0.45f), Vector2f(360.f, 72.f));
	m_pauseBackButton = std::make_unique<MenuButton>(m_font, "Back to Menu", Vector2f((float)m_window.getSize().x * 0.5f, (float)m_window.getSize().y * 0.55f), Vector2f(360.f, 72.f));

	m_pauseResumeButton->SetEnabled(true);
	m_pauseBackButton->SetEnabled(true);

	m_pauseResumeButton->SetPersistentAccent(true);

	// initialize game-over text
	m_gameOver = false;
	m_gameOverDelay = 3.f;
	m_gameOverText.setFont(m_font);
	m_gameOverText.setCharacterSize(72);           // big text
	m_gameOverText.setStyle(sf::Text::Bold);
	m_gameOverText.setFillColor(sf::Color::Red);
	m_gameOverText.setOutlineThickness(2.f);
	m_gameOverText.setOutlineColor(sf::Color::Black);
	m_gameOverText.setString(""); // initially empty


	// Wire button callbacks
	m_pauseResumeButton->RefreshLayout();
	m_pauseBackButton->RefreshLayout();

	m_mainMenu->OnPlay = [this]() {
		ResetGameplay(true);

		m_state = GameState::PLAYING;
		m_audio.StartMusic();

		// start looping ambient emitters only if buffers exist
		if (m_dialogueEmitter && m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
		if (m_effectEmitter && m_effectEmitter->buffer) m_effectEmitter->sound.play();
		};
	m_mainMenu->OnExit = [this]() {
		m_window.close();
		};
	m_mainMenu->OnOptions = [this]() {
		OptionsUI::Show(m_audio, m_font, m_mainMenu.get());
		};


	m_frameClock.restart();
}

Game::~Game() {}

int Game::Run()
{
	while (m_window.isOpen() && m_running) {
		float dt = m_frameClock.restart().asSeconds();

		processEvents();

		if (m_state == GameState::PLAYING && !m_paused) {
			if (m_worldView) m_worldView->update(dt, m_camera.getCenter());
			update(dt);
		}
		else if (m_state == GameState::PLAYING && m_paused) {
			// paused -> only process pause UI updates (tween buttons)
			m_pauseResumeButton->Update(dt, Vector2f((float)Mouse::getPosition(m_window).x, (float)Mouse::getPosition(m_window).y));
			m_pauseBackButton->Update(dt, Vector2f((float)Mouse::getPosition(m_window).x, (float)Mouse::getPosition(m_window).y));
		}
		else {
			m_audio.StopMusic();
			m_mainMenu->Update(dt, m_window);
		}

		render();
	}
	return 0;
}

void Game::processEvents()
{
	Event ev;
	while (m_window.pollEvent(ev)) {

		// inside processEvents(), right after: while (m_window.pollEvent(ev)) {
	// If we're in game-over countdown, ignore almost all input except window close
		

		if (ev.type == Event::Closed)
			m_window.close();


		

		// Global pause handling (toggle on Escape while playing)
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape) {
			if (m_state == GameState::MENU) {
				m_window.close();
			}
			else if (m_state == GameState::PLAYING) {
				// Toggle pause
				m_paused = !m_paused;
				if (m_paused) {
					// Pause audio emitters and music
					// store previous statuses and pause
					m_emitterPrevStatus.clear();
					if (m_dialogueEmitter && m_dialogueEmitter->buffer) m_emitterPrevStatus["dialogue"] = m_dialogueEmitter->sound.getStatus(), m_dialogueEmitter->sound.pause();
					if (m_effectEmitter && m_effectEmitter->buffer) m_emitterPrevStatus["effect"] = m_effectEmitter->sound.getStatus(), m_effectEmitter->sound.pause();
					if (m_playerReply && m_playerReply->buffer) m_emitterPrevStatus["playerReply"] = m_playerReply->sound.getStatus(), m_playerReply->sound.pause();
					// pause registered player emitters
					for (auto& kv : m_playerEmitters) {
						if (kv.second && kv.second->buffer) {
							m_emitterPrevStatus[kv.first] = kv.second->sound.getStatus();
							kv.second->sound.pause();
						}
					}
					m_audio.PauseMusic();
				}
				else {
					// Resume
					// restore previously playing sounds
					if (m_dialogueEmitter && m_dialogueEmitter->buffer) { if (m_emitterPrevStatus["dialogue"] == sf::Sound::Playing) m_dialogueEmitter->sound.play(); }
					if (m_effectEmitter && m_effectEmitter->buffer) { if (m_emitterPrevStatus["effect"] == sf::Sound::Playing) m_effectEmitter->sound.play(); }
					if (m_playerReply && m_playerReply->buffer) { if (m_emitterPrevStatus["playerReply"] == sf::Sound::Playing) m_playerReply->sound.play(); }
					for (auto& kv : m_playerEmitters) {
						auto it = m_emitterPrevStatus.find(kv.first);
						if (it != m_emitterPrevStatus.end() && it->second == sf::Sound::Playing) {
							if (kv.second && kv.second->buffer) kv.second->sound.play();
						}
					}
					m_audio.ResumeMusic();
				}

				// If we paused, do not perform menu reset logic below.
				if (m_paused) continue;

				// If unpausing via Escape should instead return to menu, remove continue above.

				// When exiting to menu via Escape (previous behavior) was here; preserved in Back-to-Menu button.
			}
		}

		if (m_state == GameState::MENU) {
			if (ev.type == Event::MouseMoved) {
				sf::Vector2i mp = Mouse::getPosition(m_window);
				m_mainMenu->OnMouseMoved(m_window.mapPixelToCoords(mp));
			}
			if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
				sf::Vector2i mp = Mouse::getPosition(m_window);
				m_mainMenu->OnMousePressed(m_window.mapPixelToCoords(mp));
			}
			continue;
		}

		// If paused, handle pause UI mouse events
		if (m_state == GameState::PLAYING && m_paused) {
			if (ev.type == Event::MouseMoved) {
				sf::Vector2i mp = Mouse::getPosition(m_window);
				Vector2f world = m_window.mapPixelToCoords(mp);
				m_pauseResumeButton->Update(0.f, world);
				m_pauseBackButton->Update(0.f, world);
			}
			if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
				sf::Vector2i mp = Mouse::getPosition(m_window);
				Vector2f world = m_window.mapPixelToCoords(mp);
				// Check resume button
				if (m_pauseResumeButton->Contains(world)) {
					// resume
					m_paused = false;
					// restore audio (same logic as key handler)
					if (m_dialogueEmitter && m_dialogueEmitter->buffer) { if (m_emitterPrevStatus["dialogue"] == sf::Sound::Playing) m_dialogueEmitter->sound.play(); }
					if (m_effectEmitter && m_effectEmitter->buffer) { if (m_emitterPrevStatus["effect"] == sf::Sound::Playing) m_effectEmitter->sound.play(); }
					if (m_playerReply && m_playerReply->buffer) { if (m_emitterPrevStatus["playerReply"] == sf::Sound::Playing) m_playerReply->sound.play(); }
					for (auto& kv : m_playerEmitters) {
						auto it = m_emitterPrevStatus.find(kv.first);
						if (it != m_emitterPrevStatus.end() && it->second == sf::Sound::Playing) {
							if (kv.second && kv.second->buffer) kv.second->sound.play();
						}
					}
					m_audio.ResumeMusic();
				}
				else if (m_pauseBackButton->Contains(world)) {
					// Back to menu
					m_paused = false;
					m_state = GameState::MENU;
					m_audio.StopMusic();
					m_dialogueEmitter->stop();
					m_effectEmitter->stop();
					if (m_mainMenu) m_mainMenu->ResetMobileVisual();

					// Reset gameplay state and timers while in menu so they don't accumulate
					psychoMode = false;
					m_player->SetColor(sf::Color::Red);
					m_player->SetAudioState(PlayerAudioState::Neutral);
					m_lastAppliedAudioState = PlayerAudioState::Neutral;

					splitMode = false;
					inTransition = false;
					pendingEnable = false;
					m_camera.setRotation(0.f);

					nextPsychoSwitch = randomFloat(6.f, 8.f);
					psychoClock.restart();

					nextSplitCheck = randomFloat(1.f, 3.f);
					splitClock.restart();

					inputLocked = false;
					inputLockPending = false;
					nextInputLockCheck = randomFloat(3.f, 6.f);
					inputLockClock.restart();
				}
			}
			continue; // while paused don't process gameplay keys below
		}

		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::M) {
			static bool mToggle = false; mToggle = !mToggle; m_audio.SetMusicVolume(mToggle ? 0.0f : 1.f);
		}
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::B) {
			static bool bToggle = false; bToggle = !bToggle; m_audio.SetBackgroundVolume(bToggle ? 0.1f : 1.f);
		}
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::P) {
			psychoMode = !psychoMode;
			m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
			m_player->SetAudioState(psychoMode ? PlayerAudioState::Crazy : PlayerAudioState::Neutral);

			splitMode = false;
			inTransition = false;
			pendingEnable = false;
			splitClock.restart();
			nextSplitCheck = randomFloat(1.f, 3.f);

			inputLocked = false;
			inputLockPending = false;
			inputLockClock.restart();
			nextInputLockCheck = randomFloat(3.f, 6.f);
		}
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Num1) {
			if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
		}
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Num2) {
			if (m_effectEmitter->buffer) m_effectEmitter->sound.play();
		}
		if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Y) {
			if (m_playerReply && m_playerReply->buffer) {
				m_playerReply->sound.play();
			}
		}
	}
}
// Game.cpp

void Game::ResetGameplay(bool resetPlayerPosition)
{
	// Core gameplay flags
	psychoMode = false;
	splitMode = false;
	inTransition = false;
	pendingEnable = false;
	inputLocked = false;
	inputLockPending = false;

	// Camera & audio state
	m_camera.setRotation(0.f);
	m_lastAppliedAudioState = PlayerAudioState::Neutral;
	if (m_worldView)
		m_worldView->ResetWorld();
	// Reset player audio & visuals
	if (m_player) {
		m_player->SetAudioState(PlayerAudioState::Neutral);
		m_player->SetColor(sf::Color::Red);

		// Reset physics body position and velocities when requested.
		if (resetPlayerPosition) {
			b2Body* body = m_player->GetBody();
			if (body) {
				// use the same start position you used when creating the player
				body->SetTransform(b2Vec2(140.f * INV_PPM, 800.f * INV_PPM), 0.f);
				body->SetLinearVelocity(b2Vec2(0.f, 0.f));
				body->SetAngularVelocity(0.f);
			}
			// If your Player class has additional internal state (health, anim state),
			// add a Reset() method in Player and call it here:
			// m_player->Reset();
		}
		else {
			// If not resetting position, still ensure velocities are zeroed
			b2Body* body = m_player->GetBody();
			if (body) {
				body->SetLinearVelocity(b2Vec2(0.f, 0.f));
				body->SetAngularVelocity(0.f);
			}
		}
	}

	// Reset timers & randomized next-times
	psychoClock.restart();
	nextPsychoSwitch = randomFloat(6.f, 8.f);

	splitClock.restart();
	nextSplitCheck = randomFloat(1.f, 3.f);
	splitDuration = 0.f;

	inputLockClock.restart();
	nextInputLockCheck = randomFloat(3.f, 6.f);

	transitionClock.restart();
	m_transitionStartRotation = 0.f;
	m_transitionTargetRotation = 0.f;

	// Reset player emitters: stop them and set position to player
	b2Vec2 playerPos = { 0.f,0.f };
	if (m_player && m_player->GetBody()) playerPos = m_player->GetBody()->GetPosition();
	for (auto& [id, emitter] : m_playerEmitters) {
		if (emitter) {
			emitter->sound.stop();
			emitter->sound.setPlayingOffset(sf::Time::Zero);
			emitter->position = playerPos;
		}
	}

	// Reset dedicated emitters
	if (m_dialogueEmitter) { m_dialogueEmitter->sound.stop(); m_dialogueEmitter->sound.setPlayingOffset(sf::Time::Zero); }
	if (m_effectEmitter) { m_effectEmitter->sound.stop(); m_effectEmitter->sound.setPlayingOffset(sf::Time::Zero); }
	if (m_playerReply) { m_playerReply->sound.stop(); m_playerReply->sound.setPlayingOffset(sf::Time::Zero); }

	// Grocery / obstacle audio state + timers
	m_groceryCollisionPlayed = false;
	m_groceryWaitingPlayerReply = false;
	m_groceryCooldownActive = false;
	m_groceryClock.restart();
	m_nextGroceryLineTime = randomFloat(5.f, 10.f);
	m_groceryCooldownClock.restart();

	// If grocery emitter exists, update its position to match obstacle
	if (m_groceryObstacleIndex >= 0 && m_worldView) {
		b2Vec2 gpos = m_worldView->getObstacleBodyPosition(m_groceryObstacleIndex);
		if (m_groceryA) { m_groceryA->sound.stop(); m_groceryA->sound.setPlayingOffset(sf::Time::Zero); m_groceryA->position = gpos; }
		if (m_groceryB) { m_groceryB->sound.stop(); m_groceryB->sound.setPlayingOffset(sf::Time::Zero); m_groceryB->position = gpos; }
		if (m_groceryCollision) { m_groceryCollision->sound.stop(); m_groceryCollision->sound.setPlayingOffset(sf::Time::Zero); m_groceryCollision->position = gpos; }
	}

	// Preserve user's mixer volumes set via Options during the current run.
	// Do not overwrite m_audio volume sliders here.

	// Ensure music state is neutral (we don't automatically start music here,
	// caller (OnPlay) will call StartMusic when appropriate)
	m_audio.StopMusic();

	// Ensure the AudioManager will return to neutral music after reset.
	// If psycho/crazy track was active previously, force a transition back to neutral
	// so the crazy music won't keep playing after a reset.
	m_audio.CrossfadeToNeutral();
	m_audio.StartMusic();
}

void Game::SpawnBus()
{
	// compute view boundaries (pixels)
	sf::Vector2f camCenter = m_camera.getCenter();
	sf::Vector2f viewSize = m_camera.getSize();
	float leftX = camCenter.x - viewSize.x * 0.5f - m_busSpawnMargin;
	float rightX = (camCenter.x + viewSize.x * 0.5f + m_busSpawnMargin)+700;

	// pick Y relative to player's y (so the bus is near ground/player)
	sf::Vector2f playerPixel = { 640.f, 540.f }; // fallback center
	if (m_player && m_player->GetBody()) {
		b2Vec2 p = m_player->GetBody()->GetPosition();
		playerPixel = { p.x * PPM, p.y * PPM };
	}
	float busY = 940.f; // a bit below the player; tweak as needed

	Bus b;
	b.sprite.setTexture(m_busTexture);
	b.sprite.setOrigin(m_busTexture.getSize().x * 0.5f, m_busTexture.getSize().y * 0.5f);
	// optional scaling:
	b.sprite.setScale(1.5f, 1.5f);

	b.startPos = { leftX, busY };
	b.endPos = { rightX, busY };
	b.duration = m_busTravelTime;
	b.progress = 0.f;
	b.active = true;

	// Play audio immediately when spawning:
	b.playedEmitter = true; // mark true so update() won't replay at mid-point
	if (m_busEmitter && m_busEmitter->buffer) {
		// set emitter to spawn position (meters)
		m_busEmitter->position = b2Vec2(b.startPos.x * INV_PPM, b.startPos.y * INV_PPM);
		m_busEmitter->sound.stop();   // ensure restart
		m_busEmitter->sound.play();   // play right away
	}

	b.sprite.setPosition(b.startPos);

	m_buses.push_back(std::move(b));
}



void Game::update(float dt)
{

	
	bool isGrounded = (m_contactListener.footContacts > 0);

	if (m_gameOver)
	{
		// Prevent real-time input: flag already set when game over started
		m_disableInputDuringGameOver = true;

		// Zero physics velocity immediately to avoid any drift
		if (m_player && m_player->GetBody()) {
			b2Body* body = m_player->GetBody();
			body->SetLinearVelocity(b2Vec2(0.f, 0.f));
			body->SetAngularVelocity(0.f);
		}

		// Clear player state / animations that might reapply motion
		m_player->SetWalking(false);
		m_player->SetAudioState(PlayerAudioState::Neutral);

		// Stop player emitters (optional)
		for (auto& kv : m_playerEmitters) {
			if (kv.second && kv.second->buffer) kv.second->sound.stop();
		}

		// NOTE: we DO NOT return here. We'll still run the countdown check later in this function.
	}

	// Psycho mode switch
	if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
		psychoMode = !psychoMode;
		m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
		m_player->SetAudioState(psychoMode ? PlayerAudioState::Crazy : PlayerAudioState::Neutral);

		nextPsychoSwitch = randomFloat(6.f, 12.f);
		psychoClock.restart();

		splitMode = false;
		inTransition = false;
		pendingEnable = false;
		splitClock.restart();
		nextSplitCheck = randomFloat(1.f, 3.f);

		inputLocked = false;
		inputLockPending = false;
		inputLockClock.restart();
		nextInputLockCheck = randomFloat(3.f, 6.f);
	}

	static bool refusePlayed = false; // for "refuse" sound once
	bool wavePlayed = false;


	if (psychoMode) {
		float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

		if (inputLockPending) {
			if (isGrounded) {
				inputLockPending = false;
				inputLocked = true;
				inputLockClock.restart();
				m_player->SetColor(Color::Cyan);
			}
		}
		else if (!inputLocked) {
			if (elapsedLock >= nextInputLockCheck) {
				if (rand() % 3 == 0) {
					if (isGrounded) {
						inputLocked = true;
						inputLockClock.restart();
						m_player->SetColor(Color::Cyan);
					}
					else {
						inputLockPending = true;
					}
				}
				else {
					inputLockClock.restart();
					nextInputLockCheck = randomFloat(3.f, 6.f);
				}
			}
		}
		else {
			if (inputLockClock.getElapsedTime().asSeconds() >= INPUT_LOCK_DURATION) {
				inputLocked = false;
				inputLockClock.restart();
				nextInputLockCheck = randomFloat(3.f, 6.f);
				m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
			}
		}

		// Play "refuse" once when input locked
		if (inputLocked) {
			if (!refusePlayed) {
				auto it = m_playerEmitters.find("refuse");
				if (it != m_playerEmitters.end() && it->second) {
					it->second->sound.play();
				}
				if (inputLocked && !wavePlayed)
				{
					m_player->PlayWave();
					wavePlayed = true;
				}
				else if (!inputLocked)
				{
					wavePlayed = false;
				}
				refusePlayed = true;
			}
		}
		else {
			refusePlayed = false;
		}

		// Split mode handling remains unchanged...
		// --- Split mode handling (replace the existing split-mode / inTransition code) ---

		float elapsedSplit = splitClock.getElapsedTime().asSeconds();
		if (!splitMode && !inTransition) {
			if (elapsedSplit >= nextSplitCheck) {
				if (rand() % 2 == 0) {
					inTransition = true;
					pendingEnable = true;
					transitionClock.restart();

					// Start/target rotation for smooth lerp
					m_transitionStartRotation = m_camera.getRotation();
					m_transitionTargetRotation = 180.f; // rotating0 ->180
				}
				else {
					splitClock.restart();
					nextSplitCheck = randomFloat(1.f, 3.f);
				}
			}
		}
		if (splitMode && !inTransition) {
			if (elapsedSplit >= splitDuration) {
				inTransition = true;
				pendingEnable = false;
				transitionClock.restart();

				// Start/target rotation for smooth lerp
				m_transitionStartRotation = m_camera.getRotation();
				m_transitionTargetRotation = 0.f; // rotating180 ->0
			}
		}

		if (inTransition) {
			float t = transitionClock.getElapsedTime().asSeconds();
			float alpha = t / TRANSITION_TIME;
			if (alpha > 1.f) alpha = 1.f;

			// Smoothstep easing for a nicer feel: ease =3a^2 -2a^3
			float ease = alpha * alpha * (3.f - 2.f * alpha);

			// Interpolate rotation and apply every frame while transitioning
			float curRot = m_transitionStartRotation + (m_transitionTargetRotation - m_transitionStartRotation) * ease;
			m_camera.setRotation(curRot);

			if (alpha >= 1.f) {
				// transition finished — set final state exactly and reset flags
				if (pendingEnable) {
					splitMode = true;
					splitDuration = randomFloat(2.f, 4.f);
					m_camera.setRotation(180.f); // ensure exact final value
					splitClock.restart();
				}
				else {
					splitMode = false;
					m_camera.setRotation(0.f); // ensure exact final value
					splitClock.restart();
					nextSplitCheck = randomFloat(1.f, 3.f);
				}
				inTransition = false;
				pendingEnable = false;
			}
		}

	}
	else {
		splitMode = false;
		inTransition = false;
		pendingEnable = false;
		m_camera.setRotation(0.f);

		inputLocked = false;
		inputLockPending = false;
		inputLockClock.restart();
		nextInputLockCheck = randomFloat(3.f, 6.f);
	}

	// --- Movement & input handling (guarded by m_gameOver) ---
	b2Vec2 vel = m_player->GetLinearVelocity();
	float moveSpeed = 5.f;


	// ----- ORIGINAL INPUT CODE (unchanged) -----
	bool leftKey = Keyboard::isKeyPressed(Keyboard::A) || Keyboard::isKeyPressed(Keyboard::Left);
	bool rightKey = Keyboard::isKeyPressed(Keyboard::D) || Keyboard::isKeyPressed(Keyboard::Right);
	bool jumpKeyW = Keyboard::isKeyPressed(Keyboard::W) || Keyboard::isKeyPressed(Keyboard::Up);
	bool jumpKeyS = Keyboard::isKeyPressed(Keyboard::S) || Keyboard::isKeyPressed(Keyboard::Down);
	bool shiftKey = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift);

	// If game-over is active, disable all real-time input — keep vel zero.
	if (m_gameOver)
	{
		// ensure no movement
		vel.x = 0.f;
		vel.y = 0.f;
		m_player->SetLinearVelocity(vel); // if Player wrapper expects this
		m_player->Update(dt, isGrounded); // update animations/logic in a frozen state
	}
	else
	{

		// Shift = Walk (slower), else Run (faster)
		moveSpeed = (shiftKey ? 5.f : 10.f);
		m_player->SetWalking(shiftKey);

		if (inputLocked) {
			leftKey = rightKey = false;
			jumpKeyW = jumpKeyS = false;
		}
		else {
			if (psychoMode) {
				std::swap(leftKey, rightKey);
				jumpKeyW = false;
			}
			else {
				jumpKeyS = false;
			}
		}

		if (leftKey) vel.x = -moveSpeed;
		else if (rightKey) vel.x = moveSpeed;
		else vel.x = 0;

		bool isGroundedNow = (m_contactListener.footContacts > 0);
		bool jumpKey = jumpKeyW || jumpKeyS;

		const float verticalEpsilon = 0.05f; // small threshold to treat as "not changing"
		bool canJumpNow = isGroundedNow && std::abs(vel.y) < verticalEpsilon;

		if (jumpKey && canJumpNow) {
			vel.y = -20.f;
		}

		m_player->SetLinearVelocity(vel);
		m_player->Update(dt, isGroundedNow);
		// ----- end original input code -----
	}


	// Update player emitter positions
	b2Vec2 playerPos = m_player->GetBody()->GetPosition();
	for (auto& [id, emitter] : m_playerEmitters) {
		emitter->position = playerPos;
	}

	// Collision detection
	if (m_worldView && m_player) {
		sf::RectangleShape playerShape;
		b2Body* body = m_player->GetBody();
		if (body) {
			const b2Transform& xf = body->GetTransform();
			float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;

			for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) {
				if (f->IsSensor()) continue;
				const b2Shape* s = f->GetShape();
				if (s->GetType() != b2Shape::e_polygon) continue;

				const b2PolygonShape* poly = static_cast<const b2PolygonShape*>(s);
				for (int i = 0; i < poly->m_count; ++i) {
					b2Vec2 v = b2Mul(xf, poly->m_vertices[i]);
					if (v.x < minx) minx = v.x;
					if (v.x > maxx) maxx = v.x;
					if (v.y < miny) miny = v.y;
					if (v.y > maxy) maxy = v.y;
				}
				break;
			}

			if (maxx > minx && maxy > miny) {
				float w = (maxx - minx) * PPM;
				float h = (maxy - miny) * PPM;
				playerShape.setSize(sf::Vector2f(w, h));
				playerShape.setOrigin(w * 0.5f, h * 0.5f);
				playerShape.setPosition(((minx + maxx) * 0.5f) * PPM, ((miny + maxy) * 0.5f) * PPM);
			}
			else {
				b2Vec2 p = body->GetPosition();
				playerShape.setSize(sf::Vector2f(40.f, 40.f));
				playerShape.setOrigin(20.f, 20.f);
				playerShape.setPosition(p.x * PPM, p.y * PPM);
			}
		}

		// Determine whether player is "calm": either walking (shiftKey true) or standing still
		bool isPlayerMoving = std::abs(vel.x) > 0.05f;
		bool playerCalm = (!isPlayerMoving) || shiftKey;
		m_worldView->checkCollision(playerShape, !playerCalm);

		// --- Game over / respawn countdown handling (robust) ---
		if (!m_gameOver && m_worldView && m_worldView->consumeGameOverTrigger())
		{
			// start countdown exactly once
			m_gameOver = true;
			m_disableInputDuringGameOver = true;
			m_gameOverClock.restart();

			// Stop music & emitters so the scene is quiet while counting down
			m_audio.StopMusic();
			if (m_dialogueEmitter && m_dialogueEmitter->buffer) m_dialogueEmitter->sound.stop();
			if (m_effectEmitter && m_effectEmitter->buffer) m_effectEmitter->sound.stop();
			if (m_playerReply && m_playerReply->buffer) m_playerReply->sound.stop();
			for (auto& kv : m_playerEmitters) {
				if (kv.second && kv.second->buffer) kv.second->sound.stop();
			}

			// Prepare the "YOU LOSE" text
			m_gameOverText.setString("YOU LOSE");
			sf::FloatRect tb = m_gameOverText.getLocalBounds();
			m_gameOverText.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
		}

		// If we're currently in countdown, handle timing (do not let game world progress)
		if (m_gameOver)
		{
			float elapsed = m_gameOverClock.getElapsedTime().asSeconds();
			if (elapsed >= m_gameOverDelay)
			{
				// countdown finished -> respawn
				m_gameOver = false;
				m_disableInputDuringGameOver = false;

				// Reset gameplay (respawn the player) and resume audio
				ResetGameplay(true);
				m_audio.StartMusic();

				// Ensure emitters reflect reset state (they're already stopped in ResetGameplay)
			}
			else
			{
				// Still counting down: freeze gameplay by returning early from update()
				// (Render will still be called by Run(), so the text + countdown appear).
				return;
			}
		}


		// ----- Grocery: update emitter positions -----
		// ----- Grocery: update emitter positions -----
		if (m_groceryObstacleIndex >= 0)
		{
			b2Vec2 gpos = m_worldView->getObstacleBodyPosition(m_groceryObstacleIndex);
			if (m_groceryA) m_groceryA->position = gpos;
			if (m_groceryB) m_groceryB->position = gpos;
			if (m_groceryCollision) m_groceryCollision->position = gpos;
		}

		// ---- Grocery: collision vs ambient behavior ----
		bool collidingWithGrocery = (m_worldView->getLastCollidedObstacleIndex() == m_groceryObstacleIndex);

		if (m_groceryObstacleIndex >= 0)
		{
			// expire cooldown if elapsed
			if (m_groceryCooldownActive &&
				m_groceryCooldownClock.getElapsedTime().asSeconds() >= m_groceryCooldownDuration)
			{
				m_groceryCooldownActive = false;
				// optional debug:
				std::cerr << "DEBUG: grocery cooldown expired\n";
			}

			if (collidingWithGrocery)
			{
				// Stop ambient lines so collision line is clean
				if (m_groceryA && m_groceryA->sound.getStatus() == sf::Sound::Playing) m_groceryA->sound.stop();
				if (m_groceryB && m_groceryB->sound.getStatus() == sf::Sound::Playing) m_groceryB->sound.stop();

				// If we haven't yet started the collision sequence for this contact, start it
				// but only if cooldown is NOT active
				if (!m_groceryCollisionPlayed && !m_groceryCooldownActive)
				{
					if (m_groceryCollision && m_groceryCollision->buffer) {
						m_groceryCollision->sound.stop(); // ensure restart
						m_groceryCollision->sound.play();
						m_groceryWaitingPlayerReply = true; // wait until grocery line finishes (persist even if player leaves)
						std::cerr << "DEBUG: grocery collision line started\n";
					}
					m_groceryCollisionPlayed = true;
				}
				// else: either it's already started for this contact, or we're in cooldown -> do nothing
			}
			else
			{
				// NOT colliding: do NOT clear m_groceryWaitingPlayerReply — we still want the player reply
				// Ambient random chatter should only occur when we're not waiting for a reply
				if (!m_groceryWaitingPlayerReply)
				{
					if (m_groceryClock.getElapsedTime().asSeconds() >= m_nextGroceryLineTime)
					{
						m_groceryClock.restart();
						m_nextGroceryLineTime = randomFloat(5.f, 10.f);

						// skip playing if any ambient is already playing
						if (m_groceryA && m_groceryA->buffer && m_groceryA->sound.getStatus() != sf::Sound::Playing &&
							m_groceryB && m_groceryB->buffer && m_groceryB->sound.getStatus() != sf::Sound::Playing)
						{
							if (rand() % 2 == 0)
							{
								if (m_groceryA && m_groceryA->buffer) m_groceryA->sound.play();
							}
							else
							{
								if (m_groceryB && m_groceryB->buffer) m_groceryB->sound.play();
							}
						}
					}
				}
				// else: we're waiting for grocery collision line to finish -> do nothing
			}

			// ---- waiting-for-reply handler (run regardless of collision state) ----
			if (m_groceryWaitingPlayerReply)
			{
				// Guard: if grocery emitter exists, wait until it reports Stopped
				bool groceryStopped = true; // default true if no emitter (fallback)
				if (m_groceryCollision && m_groceryCollision->buffer) {
					groceryStopped = (m_groceryCollision->sound.getStatus() == sf::Sound::Stopped);
				}

				if (groceryStopped)
				{
					// Play player reply exactly like "refuse" (from map)
					auto itReply = m_playerEmitters.find("player_reply");
					if (itReply != m_playerEmitters.end() && itReply->second && itReply->second->buffer) {
						// restart to ensure we always hear it
						itReply->second->sound.stop();
						itReply->second->sound.play();
						std::cerr << "DEBUG: played player_reply after grocery finished\n";
					}
					else {
						std::cerr << "DEBUG: player_reply emitter not found / buffer missing\n";
					}

					// Done waiting for this collision — reset flags so future collisions can re-trigger,
					// but start cooldown so they cannot re-trigger immediately
					m_groceryWaitingPlayerReply = false;
					m_groceryCollisionPlayed = false;

					// start cooldown to prevent immediate retrigger
					m_groceryCooldownActive = true;
					m_groceryCooldownClock.restart();
					std::cerr << "DEBUG: grocery cooldown started (" << m_groceryCooldownDuration << "s)\n";
				}
			}
		}
		else
		{
			// no collision — reset collision sequence and enable ambient random chatter
			// NOTE: do NOT clear the cooldown here; cooldown should persist even if obstacle isn't present.
			m_groceryCollisionPlayed = false;
			m_groceryWaitingPlayerReply = false;

			if (m_groceryClock.getElapsedTime().asSeconds() >= m_nextGroceryLineTime)
			{
				m_groceryClock.restart();
				m_nextGroceryLineTime = randomFloat(5.f, 10.f);

				// skip playing if grocery collision emitter is mid-play (unlikely since not colliding)
				if (m_groceryA && m_groceryA->buffer && m_groceryA->sound.getStatus() != sf::Sound::Playing &&
					m_groceryB && m_groceryB->buffer && m_groceryB->sound.getStatus() != sf::Sound::Playing)
				{
					if (rand() % 2 == 0)
					{
						if (m_groceryA && m_groceryA->buffer) m_groceryA->sound.play();
					}
					else
					{
						if (m_groceryB && m_groceryB->buffer) m_groceryB->sound.play();
					}
				}
				else
				{
					// If one ambient is playing, just skip this tick and let timer pick the next.
				}
			}
		}


		// --- Bus spawn timer ---
		if (m_busSpawnClock.getElapsedTime().asSeconds() >= m_busSpawnInterval) {
			SpawnBus();
			m_busSpawnClock.restart();
		}

		// Update active buses
		for (auto it = m_buses.begin(); it != m_buses.end(); /*increment inside*/) {
			if (!it->active) {
				it = m_buses.erase(it);
				continue;
			}

			it->progress += dt / it->duration;
			if (it->progress > 1.f) it->progress = 1.f;

			// linear interpolation
			float t = it->progress;
			float nx = it->startPos.x + (it->endPos.x - it->startPos.x) * t;
			float ny = it->startPos.y + (it->endPos.y - it->startPos.y) * t;
			it->sprite.setPosition(nx, ny);

			// update shared bus emitter position (convert to meters)
			if (m_busEmitter) {
				m_busEmitter->position = b2Vec2(nx * INV_PPM, ny * INV_PPM);
			}

			// Play the bus sound once when crossing middle of the view (t >= 0.5)
			if (!it->playedEmitter && t >= 0.5f) {
				if (m_busEmitter && m_busEmitter->buffer) {
					m_busEmitter->sound.stop();
					m_busEmitter->sound.play();
				}
				it->playedEmitter = true;
			}

			// finished
			if (it->progress >= 1.f) {
				it->active = false;
				// stop emitter if still playing
				if (m_busEmitter) m_busEmitter->sound.stop();
				it = m_buses.erase(it);
				continue;
			}

			++it;
		}




	}

	// Audio crossfade logic
	PlayerAudioState cur = m_player->GetAudioState();
	if (cur != m_lastAppliedAudioState) {
		if (cur == PlayerAudioState::Crazy) m_audio.CrossfadeToCrazy();
		else m_audio.CrossfadeToNeutral();
		m_lastAppliedAudioState = cur;
	}

	m_audio.Update(dt, playerPos);
}

void Game::render()
{
	if (m_state == GameState::MENU) {
		m_window.setView(m_defaultView);
		m_window.clear(Color(20, 20, 30));
		m_mainMenu->Render(m_window);
		m_window.display();
		return;
	}

	m_player->SyncGraphics();

	m_diagMark.setPosition(m_dialogueEmitter->position.x * PPM, m_dialogueEmitter->position.y * PPM);
	m_fxMark.setPosition(m_effectEmitter->position.x * PPM, m_effectEmitter->position.y * PPM);

	b2Vec2 pos = m_player->GetBody()->GetPosition();
	Vector2f playerPosPixels(pos.x * PPM, pos.y * PPM);
	Vector2f camCenter = { playerPosPixels.x,540 };
	if (inTransition) {
		camCenter.x += randomOffset(TRANSITION_SHAKE_MAG);
		camCenter.y += randomOffset(TRANSITION_SHAKE_MAG);
	}
	else if (psychoMode) {
		camCenter.x += randomOffset(PSYCHO_SHAKE_MAG);
		camCenter.y += randomOffset(PSYCHO_SHAKE_MAG);
	}
	m_camera.setCenter(camCenter);
	m_window.setView(m_camera);

	m_window.clear(Color::Black);

	if (m_worldView) {
		// Draw background layers and obstacles
		m_worldView->drawParallaxBackground(m_window);
		m_worldView->draw(m_window); // draw obstacles
	}

	// Draw player
	m_player->Draw(m_window);


	for (auto& bus : m_buses) {
		if (bus.active) {
			m_window.draw(bus.sprite);
		}
	}
	// Draw foreground layers
	if (m_worldView) {
		m_worldView->drawParallaxForeground(m_window);
	}

	m_window.draw(m_diagMark);
	m_window.draw(m_fxMark);

	// Pause overlay + buttons
	m_window.setView(m_defaultView);
	if (m_paused) {
		m_window.draw(m_pauseOverlay);
		m_pauseResumeButton->Draw(m_window);
		m_pauseBackButton->Draw(m_window);
	}

	m_window.setView(m_defaultView);
	std::string s = std::string("State: PLAYING\n") +
		"PsychoMode: " + (psychoMode ? "ON" : "OFF") + "\n" +
		"InputLock: " + (inputLocked ? std::string("LOCKED") : std::string("FREE")) + "\n" +
		"SplitMode: " + (splitMode ? std::string("ON") : std::string("OFF")) + "\n" +
		"Controls: A/D move, W jump (inverted when psycho)\n" +
		"P: force toggle psycho | M: toggle music vol | B: toggle bg vol\n" +
		"1: play dialogue one-shot |2: play effect one-shot\n" +
		"ESC: back to menu";
	m_debugText.setString(s);
	m_window.draw(m_debugText);

	// Draw game-over HUD if active
	if (m_gameOver) {
		// draw the text in the default (screen) view so it appears as HUD and not world-space
		m_window.setView(m_defaultView);

		// Position the text near the top-center (adjust vertical offset as you like)
		sf::Vector2f dvCenter = m_defaultView.getCenter();
		m_gameOverText.setPosition(dvCenter.x, dvCenter.y - (m_window.getSize().y * 0.3f)); // above center

		// Optionally draw a countdown number under "YOU LOSE"
		float remaining = m_gameOverDelay - m_gameOverClock.getElapsedTime().asSeconds();
		if (remaining < 0.f) remaining = 0.f;
		int secs = static_cast<int>(std::ceil(remaining));
		sf::Text countdown;
		countdown.setFont(m_font);
		countdown.setCharacterSize(42);
		countdown.setFillColor(sf::Color::White);
		countdown.setStyle(sf::Text::Bold);
		countdown.setString(std::to_string(secs));
		sf::FloatRect cb = countdown.getLocalBounds();
		countdown.setOrigin(cb.left + cb.width * 0.5f, cb.top + cb.height * 0.5f);
		countdown.setPosition(dvCenter.x, dvCenter.y - (m_window.getSize().y * 0.22f));

		m_window.draw(m_gameOverText);
		m_window.draw(countdown);

		// restore camera view for other UI or future draws
		m_window.setView(m_camera);
	}


	m_window.display();
}
