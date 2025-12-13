#include "World.h"
#include "AudioEmitter.h" // kept from first version (safe if present)
#include <iostream>

#include <SFML/Graphics.hpp>
constexpr float PPM = 30.f; // Pixels per meter
constexpr float INV_PPM = 1.f / PPM;

World::World(b2World& worldRef)
	: physicsWorld(worldRef) // Gravity downward
{
	initParallax();

	// Create ALL obstacles here (from the second / latest version)
	createObstacle(400, 568 + 210, true, 170, 190, "Assets/Obstacles/Untitled-2.png"); //0
	createObstacle(1000, 470 + 235, false, 300, 200, "Assets/Obstacles/foull car.png"); //1

	createObstacle(1800, 620 + 235, false, 180, 30, "Assets/Obstacles/Closed_sewers_cap.png"); //2


	createObstacle(2900, 620 + 230, false, 110, 35, "Assets/Obstacles/sewers_cap1.png"); //3

	createObstacle(2910, 620 + 200, false, 350, 200, "Assets/Obstacles/sewers.png"); //4

	createObstacle(3800, 600 + 170, false, 250 * 1.1f, 170 * 1.2f, "Assets/Obstacles/grocery.png"); //5

	createObstacle(4825, 0 + 210, false, 75 / 3, 200 / 3, "Assets/Obstacles/the shit.png"); //6

	//bird
	createObstacle(4800, 0 + 210, false, 150, 100, "Assets/Obstacles/Bird1.png"); //7
	createObstacle(4700, 600 + 210, false, 600, 100, "Assets/Obstacles/Untitled-3.png"); //8

	createObstacle(6000, 640 + 170, false, 220, 220, "Assets/Obstacles/doggie.png"); //9

	createObstacle(7200, -300 + 210, false, 250, 150, "Assets/Obstacles/man falling.png"); //10
	createObstacle(7200, 600 + 210, false, 400, 100, "Assets/Obstacles/Untitled-3.png"); //11


	createObstacle(630, 580 + 210, true, 150, 160, "Assets/Obstacles/trash.png"); //12
	createObstacle(520, 595 + 210, true, 140, 155, "Assets/Obstacles/trash-1.png"); //13


	/////Obstacles for only dicoration (the player does not interact with them)
	createObstacle(680, 645 + 210, true, 40, 45, "Assets/Obstacles/no cola.png");
	createObstacle(-470, 510 + 210, true, 440, 240, "Assets/Obstacles/bus.png");
	createObstacle(-640, 560 + 210, true, 130, 130, "Assets/Obstacles/trash.png");

	// Load doggie angry texture (optional, non-fatal)
	if (!m_doggieAngryTexture.loadFromFile("Assets/Obstacles/doggieangry.png")) {
		std::cerr << "Warning: failed to load doggieangry.png (optional)\n";
	}

	// Load second frame for man-fall (frame2: man fell no effects)
	if (!m_manFellFrame2.loadFromFile("Assets/Obstacles/man fell no effects.png")) {
		std::cerr << "Warning: failed to load man fell no effects.png (optional)\n";
	}

	// Sewers cap animation setup (textureIndex ==3)
	std::vector<std::string> sewerFrames = {
	"Assets/Obstacles/sewers_cap1.png",
	"Assets/Obstacles/sewers_cap2.png",
	"Assets/Obstacles/sewers_cap3.png",
	"Assets/Obstacles/sewers_cap4.png",
	"Assets/Obstacles/sewers_cap5.png",
	"Assets/Obstacles/sewers_cap6.png",
	"Assets/Obstacles/sewers_cap7.png",
	"Assets/Obstacles/sewers_cap8.png"
	};
	m_sewersAnim.AddClip("open", sewerFrames, 0.06f, /*loop=*/false);
	m_sewersAnim.SetClip("open", true); // start on frame0

	// Find the obstacle that uses sewers_cap1.png
	Obstacle* cap = getObstacleByTexture(3);

	if (cap)
	{
		m_sewersBasePos = cap->shape.getPosition();
		m_sewersSprite.setPosition(m_sewersBasePos);
	}

	m_sewersAnim.BindSprite(&m_sewersSprite);
	m_sewersSprite.setScale(0.7f, 0.7f);

	m_sewersPlaying = false;
	m_sewersLastFrame = -1;

	m_sewerGameOverPending = false;
	m_sewerGameOverTimer = 0.f;


	// 🐦 Bird animation setup
	// Bird frames (your3 PNGs)
	std::vector<std::string> birdFrames = {
	"Assets/Obstacles/Bird1.png",
	"Assets/Obstacles/Bird2.png",
	"Assets/Obstacles/Bird3.png"
	};

	// Create a looping clip called "fly"
	m_birdAnim.AddClip("fly", birdFrames, 0.08f, true);
	m_birdAnim.SetClip("fly", true);

	// Find the obstacle that uses Bird1.png
	Obstacle* birdObs = getObstacleByTexture(7); // Bird1 index in this file

	if (birdObs)
	{
		// Starting position for the bird patrol
		m_birdStartPos = birdObs->shape.getPosition();
		m_birdSprite.setPosition(m_birdStartPos);
	}
	else
	{
		// Fallback, just in case
		m_birdStartPos = sf::Vector2f(4800.f, 210.f);
		m_birdSprite.setPosition(m_birdStartPos);
	}

	// Bind sprite to animation
	m_birdAnim.BindSprite(&m_birdSprite);

	// Optional: make the bird smaller
	m_birdSprite.setScale(0.4f, 0.4f);

	// Patrol settings (left/right limits around the start position)
	m_birdMinX = m_birdStartPos.x - 250.f; // left limit
	m_birdMaxX = m_birdStartPos.x + 250.f; // right limit
	m_birdSpeed = 150.f; // pixels per second

	m_birdGoingRight = true;
	m_birdAnim.SetFacingRight(true);

}


void World::createObstacle(float x, float y, bool onlyGround, float scaleX, float scaleY, const std::string& textureFile)
{
	// Load texture and store it
	obstacleTextures.emplace_back();
	if (!obstacleTextures.back().loadFromFile(textureFile))
		std::cerr << "Failed to load texture: " << textureFile << std::endl;
	obstacleTextures.back().setRepeated(false);

	// keep track of filename for substring searches
	obstacleTextureFiles.push_back(textureFile);

	sf::Vector2u texSize = obstacleTextures.back().getSize();

	// -------- BOX2D BODY --------
	b2BodyDef bodyDef;
	bodyDef.position.Set(x * INV_PPM, y * INV_PPM);
	bodyDef.type = b2_staticBody;

	b2Body* body = physicsWorld.CreateBody(&bodyDef);

	b2PolygonShape box;
	box.SetAsBox(scaleX / 2.f * INV_PPM, scaleY / 2.f * INV_PPM); // Box2D half-size

	b2FixtureDef fixture;
	fixture.shape = &box;
	fixture.density = onlyGround ? 0.f : 2.f;
	fixture.filter.categoryBits = CATEGORY_OBSTACLE;
	fixture.filter.maskBits = onlyGround ? (CATEGORY_GROUND | CATEGORY_PLAYER) : CATEGORY_GROUND;
	fixture.friction = 0.0f;


	body->CreateFixture(&fixture);

	// -------- SFML SHAPE --------
	sf::RectangleShape shape(sf::Vector2f(scaleX, scaleY));
	shape.setOrigin(scaleX / 2.f, scaleY / 2.f);
	shape.setPosition(x, y);
	shape.setTexture(&obstacleTextures.back());
	shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));

	// Store obstacle with texture index
	obstacles.emplace_back(body, shape, onlyGround, obstacleTextures.size() - 1);

	// Capture initial Box2D state and filters for reset
	auto& o = obstacles.back();
	o.startPosB2 = body->GetPosition();
	o.startAngle = body->GetAngle();
	o.startType = b2_staticBody; // created static above
	o.initialCategoryBits = fixture.filter.categoryBits;
	o.initialMaskBits = fixture.filter.maskBits;

	// Update level extents (in pixels)
	float left = x - scaleX * 0.5f;
	float right = x + scaleX * 0.5f;
	if (left < levelMinX) levelMinX = left;
	if (right > levelMaxX) levelMaxX = right;

}
void World::resetObstacle(Obstacle& o)
{
	if (!o.body) return;

	// Restore body transform, type and velocities
	o.body->SetType(o.startType);
	o.body->SetTransform(o.startPosB2, o.startAngle);
	o.body->SetLinearVelocity(b2Vec2_zero);
	o.body->SetAngularVelocity(0.f);

	// Restore filter for all fixtures
	for (b2Fixture* f = o.body->GetFixtureList(); f; f = f->GetNext()) {
		b2Filter flt = f->GetFilterData();
		flt.categoryBits = o.initialCategoryBits;
		flt.maskBits = o.initialMaskBits;
		f->SetFilterData(flt);
	}

	// Sync SFML representation and color
	o.shape.setPosition(o.startPosB2.x * PPM, o.startPosB2.y * PPM);
	o.shape.setRotation(o.startAngle * 180.f / 3.14159f);
	o.shape.setFillColor(sf::Color::White);

	// If this obstacle was the man-fall and we had swapped its texture, restore original texture
	if (o.textureIndex == 10)
	{
		if (o.textureIndex >= 0 && o.textureIndex < obstacleTextures.size()) {
			sf::Texture& tex = obstacleTextures[o.textureIndex];
			o.shape.setTexture(&tex);
			sf::Vector2u texSize = tex.getSize();
			o.shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));
		}
	}
}


void World::ResetWorld()
{
	mIsColliding = false;
	lastCollidedObstacleIndex = -1;
	mGameOverTriggered = false;

	for (auto& o : obstacles)
		resetObstacle(o);

	// 🔁 reset sewer animation state
	m_sewersPlaying = false;
	m_sewersLastFrame = -1;
	m_sewersAnim.Reset();
	m_sewersSprite.setPosition(m_sewersBasePos);

	// 🐦 reset bird
	m_birdSprite.setPosition(m_birdStartPos);
	m_birdGoingRight = true;
	m_birdAnim.SetFacingRight(true);
	m_birdAnim.Reset();

	m_poopDropped = false;

	// Restore any player fixtures we modified when sewer cap triggered
	for (auto& pair : m_modifiedPlayerFixtures)
	{
		b2Fixture* f = pair.first;
		uint16 originalMask = pair.second;
		if (f)
		{
			b2Filter flt = f->GetFilterData();
			flt.maskBits = originalMask;
			f->SetFilterData(flt);
		}
	}
	m_modifiedPlayerFixtures.clear();

	// Clear pending sewer game-over timer
	m_sewerGameOverPending = false;
	m_sewerGameOverTimer = 0.f;

	// Reset man-fell landing state
	m_manFellLanded = false;
}
World::Obstacle* World::getObstacleByTexture(size_t textureIndex)
{
	for (auto& o : obstacles)
	{
		if (o.textureIndex == textureIndex)
			return &o;
	}
	return nullptr;
}

// ======================================================================
// WORLD UPDATE
// ======================================================================
void World::update(float dt, const sf::Vector2f& camPos)
{
	physicsWorld.Step(dt, 8, 3);

	// Tick delayed sewer game-over timer
	if (m_sewerGameOverPending)
	{
		m_sewerGameOverTimer -= dt;
		if (m_sewerGameOverTimer <= 0.f)
		{
			m_sewerGameOverPending = false;
			mGameOverTriggered = true;
		}
	}

	updateParallax(camPos);

	// Sync obstacles with Box2D (keep this)
	for (auto& obj : obstacles)
	{
		b2Vec2 pos = obj.body->GetPosition();
		float angle = obj.body->GetAngle();

		obj.shape.setPosition(pos.x * PPM, pos.y * PPM);
		obj.shape.setRotation(angle * 180.f / 3.14159f);
	}


	// ✅ Animate sewer cap + move to the right on each frame change
	if (m_sewersPlaying)
	{
		m_sewersAnim.Update(dt);

		// Read current frame index from the Animation class
		int currentFrame = static_cast<int>(m_sewersAnim.CurrentFrameIndex());

		// If the frame changed since last update → move sprite
		if (currentFrame != m_sewersLastFrame)
		{
			m_sewersLastFrame = currentFrame;
			//How much to move to the right per frame (tweak this)
			float stepX = 60.f;
			float stepY = 0.f;
			m_sewersSprite.move(stepX, stepY);
		}
	}

	// 🐦 Update bird animation + left/right movement
	m_birdAnim.Update(dt); // flap

	sf::Vector2f pos = m_birdSprite.getPosition();

	// Direction: +1 = right, -1 = left
	float dir = m_birdGoingRight ? 1.f : -1.f;
	pos.x += dir * m_birdSpeed * dt;

	// Check limits and flip direction
	if (pos.x > m_birdMaxX)
	{
		pos.x = m_birdMaxX;
		m_birdGoingRight = false;
		m_birdAnim.SetFacingRight(false); // flip sprite horizontally
	}
	else if (pos.x < m_birdMinX)
	{
		pos.x = m_birdMinX;
		m_birdGoingRight = true;
		m_birdAnim.SetFacingRight(true); // face right again
	}

	m_birdSprite.setPosition(pos);


}

bool World::consumeGameOverTrigger()
{
	bool triggered = mGameOverTriggered;
	mGameOverTriggered = false; // auto-clear on read
	return triggered;
}
static bool IsTouchingGround(b2Body* body, uint16 groundCategoryBits)
{
	if (!body) return false;
	for (b2ContactEdge* ce = body->GetContactList(); ce; ce = ce->next)
	{
		b2Contact* c = ce->contact;
		if (!c || !c->IsTouching()) continue;

		b2Fixture* fa = c->GetFixtureA();
		b2Fixture* fb = c->GetFixtureB();
		if (!fa || !fb) continue;

		const uint16 ca = fa->GetFilterData().categoryBits;
		const uint16 cb = fb->GetFilterData().categoryBits;

		// If either contact fixture is ground, we consider the body touching ground
		if ((ca & groundCategoryBits) != 0 || (cb & groundCategoryBits) != 0)
			return true;
	}
	return false;
}
void World::checkCollision(const sf::RectangleShape& playerShape, bool playerCalm)
{
	sf::FloatRect playerBounds = playerShape.getGlobalBounds();
	mIsColliding = false;
	lastCollidedObstacleIndex = -1;

	bool fall6ActiveThisFrame = false;

	for (size_t i = 0; i < obstacles.size(); ++i)
	{
		auto& obj = obstacles[i];
		sf::FloatRect obsBounds = obj.shape.getGlobalBounds();

		if (playerBounds.intersects(obsBounds))
		{
			mIsColliding = true;
			if (lastCollidedObstacleIndex == -1)
				lastCollidedObstacleIndex = static_cast<int>(i);

			// 🔥 SEWER CAP COLLISION
			if (obj.textureIndex == 3)
			{
				obj.shape.setFillColor(sf::Color::Yellow); // keep your color

				// ▶ start the animation once
				if (!m_sewersPlaying)
				{
					m_sewersPlaying = true;
					m_sewersLastFrame = -1; // so first frame movement works
					m_sewersAnim.Reset(); // reset time & frame index to0
					m_sewersSprite.setPosition(m_sewersBasePos); // start from base
				}

				// If we haven't already applied the mask change for player fixtures, do it now and start timer
				if (!m_sewerGameOverPending)
				{
					// Make the PLAYER lose collision with ground by clearing CATEGORY_GROUND
					// from any fixture whose category indicates it's the player. Store original mask to restore later.
					for (b2Body* b = physicsWorld.GetBodyList(); b; b = b->GetNext()) {
						for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()) {
							b2Filter flt = f->GetFilterData();
							if ((flt.categoryBits & CATEGORY_PLAYER) != 0) {
								// store original mask
								m_modifiedPlayerFixtures.emplace_back(f, flt.maskBits);
								// remove ground bit from mask
								flt.maskBits = static_cast<uint16>(flt.maskBits & ~CATEGORY_GROUND);
								f->SetFilterData(flt);
							}
						}
					}

					// Start2-second countdown before signalling game over
					m_sewerGameOverPending = true;
					m_sewerGameOverTimer = 2.0f; // seconds
				}

				// Do NOT immediately set mGameOverTriggered; it will be set after timer elapses
			}
			else if (obj.textureIndex == 7)
			{
				obj.body->SetType(b2_dynamicBody);
				obj.shape.setFillColor(sf::Color::Blue);
			}
			else if (obj.textureIndex == 8)
			{
				// Mark that trigger8 is active this frame (can keep if you use it elsewhere)
				fall6ActiveThisFrame = true;

				// Only drop the poop ONCE per reset
				if (!m_poopDropped)
				{
					Obstacle* fallingObj = getObstacleByTexture(6); // "the shit" obstacle
					if (fallingObj && fallingObj->body)
					{
						//1) Place poop at the bird's current position (spawn exactly at bird)
						sf::Vector2f birdPos = m_birdSprite.getPosition();

						// If you want a small offset below the bird, change offsetYPx (positive = lower on screen)
						const float offsetYpx = 0.f;
						b2Vec2 newPos(
							birdPos.x * INV_PPM,
							(birdPos.y + offsetYpx) * INV_PPM
						);
						fallingObj->body->SetTransform(newPos, fallingObj->startAngle);
						fallingObj->body->SetLinearVelocity(b2Vec2_zero);
						fallingObj->body->SetAngularVelocity(0.f);
						fallingObj->startPosB2 = fallingObj->body->GetPosition();
						fallingObj->shape.setPosition(fallingObj->startPosB2.x * PPM, fallingObj->startPosB2.y * PPM);
						//2) Make sure it collides with ground
						if (b2Fixture* f = fallingObj->body->GetFixtureList())
						{
							b2Filter filter = f->GetFilterData();
							filter.maskBits |= CATEGORY_GROUND;
							f->SetFilterData(filter);
						}

						//3) Turn into dynamic so it FALLS
						fallingObj->body->SetType(b2_dynamicBody);
						fallingObj->body->SetAwake(true);

						//4) Remember that we already dropped it
						m_poopDropped = true;
					}
				}
			}

			else if (obj.textureIndex == 6)
			{
				// Player hits index6 -> trigger game over (Game handles reset)
				obj.shape.setFillColor(sf::Color::Red);
				mGameOverTriggered = true;
			}
			else if (obj.textureIndex == 11)
			{
				Obstacle* fallingObj = getObstacleByTexture(10);
				if (fallingObj && fallingObj->body)
				{
					if (b2Fixture* f = fallingObj->body->GetFixtureList())
					{
						b2Filter filter = f->GetFilterData();
						filter.maskBits |= CATEGORY_GROUND;
						f->SetFilterData(filter);
					}
					fallingObj->body->SetType(b2_dynamicBody);
					fallingObj->body->SetAwake(true);
				}
			}

			else if (obj.textureIndex == 10)
			{
				// If the man obstacle is currently dynamic (falling) and not yet marked as landed,
				// colliding with the player should trigger game over.
				if (obj.body && obj.body->GetType() == b2_dynamicBody && !m_manFellLanded)
				{
					mGameOverTriggered = true;
				}
				// Otherwise, do nothing. When the man lands we swap texture and remove player collision.
			}

			// DOGGIE angry swap: index9 -> if player is colliding and playerCalm (walking/idle) then use angry texture
			if (obj.textureIndex == 9)
			{
				if (playerCalm && m_doggieAngryTexture.getSize().x > 0)
				{
					obj.shape.setTexture(&m_doggieAngryTexture);
					sf::Vector2u ts = m_doggieAngryTexture.getSize();
					obj.shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(ts.x), static_cast<int>(ts.y)));

					// If the dog becomes angry while colliding with the player -> trigger game over
					mGameOverTriggered = true;
				}
				//else
				//{
				//	// restore original texture if available
				//	if (obj.textureIndex >= 0 && obj.textureIndex < obstacleTextures.size()) {
				//		sf::Texture& tex = obstacleTextures[obj.textureIndex];
				//		obj.shape.setTexture(&tex);
				//		sf::Vector2u texSize = tex.getSize();
				//		obj.shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));
				//	}
				//}
			}
		}
		else
		{
			// Restore visuals for non-colliding obstacles
			if (!(obj.textureIndex == 10 && m_manFellLanded)) // keep landed man frame if landed
			{
				if (obj.textureIndex >= 0 && obj.textureIndex < obstacleTextures.size())
				{
					sf::Texture& tex = obstacleTextures[obj.textureIndex];
					obj.shape.setTexture(&tex);
					sf::Vector2u texSize = tex.getSize();
					obj.shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));
				}
			}
			obj.shape.setFillColor(sf::Color::White);
		}
	}

	// Reset index6 ONLY if it is touching ground, regardless of trigger8 state
	if (Obstacle* fallingObj = getObstacleByTexture(6))
	{
		if (fallingObj->body && fallingObj->body->GetType() == b2_dynamicBody)
		{
			if (IsTouchingGround(fallingObj->body, CATEGORY_GROUND))
			{
				resetObstacle(*fallingObj);
				m_poopDropped = false; // allow future drops again
			}
		}
	}

	// -- Handle man-fall landing detection (index10)
	if (Obstacle* man = getObstacleByTexture(10))
	{
		if (man->body && man->body->GetType() == b2_dynamicBody && !m_manFellLanded)
		{
			if (IsTouchingGround(man->body, CATEGORY_GROUND))
			{
				// Swap to frame2 if we have that texture
				if (m_manFellFrame2.getSize().x > 0)
				{
					man->shape.setTexture(&m_manFellFrame2);
					sf::Vector2u ts = m_manFellFrame2.getSize();
					man->shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(ts.x), static_cast<int>(ts.y)));
				}

				// Disable collision with player for this obstacle's fixtures
				for (b2Fixture* f = man->body->GetFixtureList(); f; f = f->GetNext()) {
					b2Filter flt = f->GetFilterData();
					flt.maskBits = static_cast<uint16>(flt.maskBits & ~CATEGORY_PLAYER);
					f->SetFilterData(flt);
				}

				m_manFellLanded = true; // ensure we only do this once until reset
			}
		}
	}
}

int World::findObstacleByTextureSubstring(const std::string& substr) const
{
	for (size_t i = 0; i < obstacleTextureFiles.size(); ++i) {
		if (obstacleTextureFiles[i].find(substr) != std::string::npos) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

b2Vec2 World::getObstacleBodyPosition(int index) const
{
	if (index < 0 || index >= static_cast<int>(obstacles.size())) return b2Vec2(0.f, 0.f);
	b2Body* b = obstacles[index].body;
	if (!b) return b2Vec2(0.f, 0.f);
	return b->GetPosition();
}

int World::getLastCollidedObstacleIndex() const
{
	return lastCollidedObstacleIndex;
}

void World::draw(sf::RenderWindow& window)
{
	// Draw background layers
	drawParallaxBackground(window);

	// Draw obstacles behind player if needed
	for (auto& obj : obstacles)
	{

		if (obj.textureIndex == 3) // sewer cap
			continue;
		if (obj.textureIndex == 7) // bird
			continue;
		if (obj.textureIndex == 6 && !m_poopDropped)
			continue; // 💩 don't draw poop before it drops



		window.draw(obj.shape);

	}
	// Draw the sewer cap sprite (static at first, animated after collision)
	window.draw(m_sewersSprite);
	// 🐦 Bird sprite
	window.draw(m_birdSprite);





	// Player will be drawn in Game::render after this

	// Foreground layers will be drawn after the player
	// drawParallaxForeground(window); // <-- called in Game::render AFTER player
}

// ======================================================================
// PARALLAX INITIALIZATION (14 layers, back → front)
// ======================================================================
void World::initParallax()
{
	parallaxLayers.clear();
	parallaxLayers.resize(14);

	// -----------------------------
	// CENTRALIZED PARALLAX CONFIG
	// -----------------------------
	struct LayerConfig {
		float speedX;
		float speedY;
		float baseYOffset;
		float scale;
	};

	// BACK → FRONT (14 layers)
	const LayerConfig config[14] =
	{
		//speedx/y y axis pos, scale
		{0.0f,0.00f,0.0f,1.00f}, // layer1 (very far)
		{0.0f,0.0f, +0.0f,1.00f}, // layer2
		{-0.09f,0.0f, +0.0f,1.00f}, // layer3

		{-0.09f,0.0f, +0.0f,1.00f}, // layer4
		{-0.09f,0.0f, +0.0f,1.00f}, // layer5
		{-0.08f,0.0f,0.0f,1.00f}, // layer6

		{-0.07f,0.0f, -0.0f,1.00f}, // layer7
		{-0.06f,0.0f, -0.0f,1.00f}, // layer8

		{-0.05f,0.0f,0.f,1.00f}, // layer9 (GROUND layer)

		{0.0f,0.0f, -0.0f,1.00f}, // layer10 (foreground)
		{0.0f,0.0f, -0.0f,1.00f}, // layer11
		{0.0f,0.0f, -0.0f,1.00f}, // layer12 (closest)
		{0.0f,0.0f,300.0f,1.0f}, // layer13 (closest)

		{0.0f,0.0f,300.0f,1.0f}, // layer14 (All Props)
	};

	for (int i = 0; i < 14; i++)
	{
		std::string path = "Assets/Parallax/" + std::to_string(i + 1) + ".png";

		if (!parallaxLayers[i].texture.loadFromFile(path))
			std::cerr << "FAILED TO LOAD PARALLAX: " << path << "\n";

		parallaxLayers[i].texture.setRepeated(true);

		parallaxLayers[i].sprite.setTexture(parallaxLayers[i].texture);
		parallaxLayers[i].sprite.setTextureRect(sf::IntRect(0, 0, 1920, 1080));

		// Apply user-friendly controls
		parallaxLayers[i].speedX = config[i].speedX;
		parallaxLayers[i].speedY = config[i].speedY;
		parallaxLayers[i].baseYOffset = config[i].baseYOffset;
		parallaxLayers[i].scale = config[i].scale;

		parallaxLayers[i].sprite.setScale(config[i].scale, config[i].scale);
	}
}

// ======================================================================
// UPDATE PARALLAX (CAMERA-DRIVEN)
// ======================================================================
static sf::Clock mCloudClock; // For independent cloud movement (file-scope static)
static float cloudSpeed = -30.f; // pixels per second

void World::updateParallax(const sf::Vector2f& camPos)
{
	float dtc = mCloudClock.restart().asSeconds(); // time since last frame

	for (size_t i = 0; i < parallaxLayers.size(); ++i)
	{
		auto& layer = parallaxLayers[i];

		float px, py;

		// cloud layers at indices1 and12 (per the second version)
		if (i == 1 || i == 12)
		{
			// independent drift
			if (i == 1)
				layer.baseXOffset += cloudSpeed * dtc;
			else
				layer.baseXOffset += (cloudSpeed + 15.f) * dtc;

			// wrap offset to avoid floating point overflow
			float texW = layer.texture.getSize().x * layer.scale;
			if (layer.baseXOffset <= -texW) layer.baseXOffset += texW;
			else if (layer.baseXOffset >= texW) layer.baseXOffset -= texW;

			px = layer.baseXOffset;
			py = -camPos.y * layer.speedY + parallaxYOffset + layer.baseYOffset;
		}
		else
		{
			px = -camPos.x * layer.speedX + layer.baseXOffset;
			py = -camPos.y * layer.speedY + parallaxYOffset + layer.baseYOffset;
		}

		layer.sprite.setPosition(px, py);
	}

	// FIRST FRAME: fix alignment
	if (!parallaxAligned)
	{
		parallaxAligned = true;

		float currentY = parallaxLayers[0].sprite.getPosition().y;
		parallaxYOffset = -currentY;

		for (auto& l : parallaxLayers)
		{
			sf::Vector2f p = l.sprite.getPosition();
			l.sprite.setPosition(p.x, p.y + parallaxYOffset);
		}
	}
}

// ======================================================================
// DRAW PARALLAX (HORIZONTAL WRAP ONLY)
// ======================================================================

// Draw background layers (0 →11)
void World::drawParallaxBackground(sf::RenderWindow& window)
{
	for (size_t i = 0; i < 12 && i < parallaxLayers.size(); ++i) // layers0–11
		drawLayer(window, parallaxLayers[i]);
}

// Draw foreground layers (12 → end)
void World::drawParallaxForeground(sf::RenderWindow& window)
{
	for (size_t i = 12; i < parallaxLayers.size(); ++i)
		drawLayer(window, parallaxLayers[i]);
}

// Helper to draw a single layer (wraps horizontally)
void World::drawLayer(sf::RenderWindow& window, ParallaxLayer& layer)
{
	sf::View view = window.getView();
	float viewW = view.getSize().x;
	float viewLeft = view.getCenter().x - viewW * 0.5f;

	float texW = layer.texture.getSize().x * layer.scale;
	if (texW <= 0.f) return; // safety

	float baseX = std::fmod(layer.sprite.getPosition().x, texW);
	if (baseX > 0) baseX -= texW;
	float y = layer.sprite.getPosition().y;

	int firstTile = static_cast<int>(std::floor((viewLeft - baseX) / texW)) - 1;
	int needed = static_cast<int>(std::ceil(viewW / texW)) + 3;

	for (int i = 0; i < needed; ++i)
	{
		sf::Sprite copy = layer.sprite;
		copy.setPosition(baseX + texW * (firstTile + i), y);
		window.draw(copy);
	}
}