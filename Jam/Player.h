#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <memory>
#include "Units.h"
#include "Animation.h"

class Player {
public:
    // Construct player and create physics body + fixtures in the provided world
    Player(b2World* world, float startX = 640.f, float startY = 200.f);
    ~Player();

    // update logic (physics already stepped by Game)
    void Update(float dt, bool grounded);

    // update visual sprite from physics body
    void SyncGraphics();

    // draw the player sprite
    void Draw(sf::RenderWindow& window);

    // physics accessors
    b2Body* GetBody() const { return m_body; }
    b2Fixture* GetFootFixture() const { return m_footFixture; }
    b2Vec2 GetPosition() const { return m_body ? m_body->GetPosition() : b2Vec2(0, 0); }
    b2Vec2 GetLinearVelocity() const { return m_body ? m_body->GetLinearVelocity() : b2Vec2(0, 0); }
    void SetLinearVelocity(const b2Vec2& v) { if (m_body) m_body->SetLinearVelocity(v); }

    // visual helpers (color tint on sprite)
    void SetColor(const sf::Color& c) { m_sprite.setColor(c); }
    void SetOrigin(float ox, float oy) { m_sprite.setOrigin(ox, oy); }

private:
    b2World* m_world;
    b2Body* m_body;
    b2Fixture* m_footFixture;

    // Visuals
    sf::Sprite m_sprite;
    Animation m_anim;

    // State
    bool m_facingRight;
};

#endif // PLAYER_H