#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <string>
#include <utility>
#include <memory>            // <--- added for std::shared_ptr
#include "Animation.h" 

// Forward-declare AudioEmitter so header doesn't need the full definition.
struct AudioEmitter;

class World
{
public:
	World(b2World& worldRef);

	// Collision categories
	static constexpr uint16 CATEGORY_PLAYER = 0x0001;
	static constexpr uint16 CATEGORY_GROUND = 0x0002;
	static constexpr uint16 CATEGORY_OBSTACLE = 0x0004;
	static constexpr uint16 CATEGORY_SENSOR = 0x0008;

	struct ParallaxLayer {
		sf::Texture texture;
		sf::Sprite  sprite;
		float baseYOffset = 0.f;
		float baseXOffset = 0.f;
		float scale = 1.f;
		float speedX = 0.f;
		float speedY = 0.f;
	};

	struct Obstacle {
		b2Body* body;
		sf::RectangleShape shape;
		bool onlyGround;
		size_t textureIndex;

		// New: initial state to allow resets
		b2Vec2       startPosB2{ 0.f, 0.f };
		float        startAngle = 0.f;
		b2BodyType   startType = b2_staticBody;
		uint16       initialCategoryBits = 0;
		uint16       initialMaskBits = 0;

		Obstacle(b2Body* b, const sf::RectangleShape& s, bool og, size_t texIdx)
			: body(b), shape(s), onlyGround(og), textureIndex(texIdx) {
		}
	};

	// ---------------------------------------------------------------------
	// PUBLIC API
	// ---------------------------------------------------------------------
	void update(float dt, const sf::Vector2f& camPos);
	void draw(sf::RenderWindow& window);
	// Accept whether the player is calm (walking or idle) so obstacles may react
	void checkCollision(const sf::RectangleShape& playerShape, bool playerCalm = false);

	void drawParallaxBackground(sf::RenderWindow& window);
	void drawParallaxForeground(sf::RenderWindow& window);
	void drawLayer(sf::RenderWindow& window, ParallaxLayer& layer);

	void createObstacle(float x, float y, bool onlyGround, float sx, float sy, const std::string& texFile);

	int   findObstacleByTextureSubstring(const std::string& substr) const;
	b2Vec2 getObstacleBodyPosition(int index) const;
	int   getLastCollidedObstacleIndex() const;
	Obstacle* getObstacleByTexture(size_t textureIndex);

	// Game-over trigger handshake with Game
	bool consumeGameOverTrigger();

	// Win trigger for level exit
	bool consumeWinTrigger();

	// Expose level extents so Game can clamp camera
	float getLevelMaxX() const;

	// New: Reset the whole world (obstacles, flags)
	void ResetWorld();

	// New: swap parallax theme between normal and psycho mode
	void SetPsychoMode(bool enable);
	bool IsPsychoMode() const;

	std::shared_ptr<AudioEmitter> m_foullCarEmitter;
	bool m_foullCarVoicePlayed = false;
	float m_foullCarTriggerX = 0.f; // world X (pixels)

private:
	b2World& physicsWorld;

	// Parallax
	std::vector<ParallaxLayer> parallaxLayers;
	void initParallax();
	void updateParallax(const sf::Vector2f& camPos);
	bool  parallaxAligned = false;
	float parallaxYOffset = 0.f;

	// helper to (re)load parallax textures from a base path
	void loadParallaxTextures(const std::string& basePath);
	// helper to preload psycho textures so switching is instant
	void preloadPsychoParallaxTextures();

	// Obstacles
	std::vector<Obstacle> obstacles;
	std::vector<sf::Texture> obstacleTextures;
	std::vector<std::string> obstacleTextureFiles;


	// 🟡 Sewer-cap animation
	Animation   m_sewersAnim;
	sf::Sprite  m_sewersSprite;
	bool        m_sewersPlaying = false;

	int         m_sewersLastFrame = -1;
	sf::Vector2f m_sewersBasePos;   // where the cap starts before moving


	// 🐦 Bird animation
	Animation   m_birdAnim;
	sf::Sprite  m_birdSprite;
	bool        m_birdGoingRight = true;
	float       m_birdSpeed = 150.f;   // pixels per second
	float       m_birdMinX = 5000.f;  // left limit
	float       m_birdMaxX = 5600.f;  // right limit
	sf::Vector2f m_birdStartPos;           // starting position
	bool m_poopDropped = false;

	// Doggie angry texture (optional asset)
	sf::Texture m_doggieAngryTexture;

	// Man-fall landing: second-frame texture and landed state
	sf::Texture m_manFellFrame2;
	bool m_manFellLanded = false;

	// Collision debug info
	bool mIsColliding = false;
	int  lastCollidedObstacleIndex = -1;

	// Game over trigger
	bool mGameOverTriggered = false;

	// Win trigger
	bool mWinTriggered = false;

	// Delay before triggering game over after sewer cap collision
	bool m_sewerGameOverPending = false;
	float m_sewerGameOverTimer = 0.f; // seconds

	// Track modified player fixtures so we can restore their original mask on reset
	std::vector<std::pair<b2Fixture*, uint16>> m_modifiedPlayerFixtures;

	// Level extents (pixels)
	float levelMinX = 1e9f;
	float levelMaxX = -1e9f;

	// Parallax theme state
	bool m_parallaxPsycho = false;
	std::string m_parallaxNormalBase = "Assets/Parallax/";
	std::string m_parallaxPsychoBase = "Assets/Parallax/Level2/";

	// Preloaded psycho textures and loaded flag
	std::vector<sf::Texture> m_parallaxPsychoTextures;
	bool m_parallaxPsychoLoaded = false;

	// Sewer lose emitter (declared here; forward-declared above)
	std::shared_ptr<AudioEmitter> m_sewerLoseEmitter;

	// Helpers
	void resetObstacle(Obstacle& o);
};