#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include "Animation.h"

enum class PlayerAudioState { Neutral, Crazy };

class Player {
public:
    // Construct player and create physics body + fixtures in the provided world
    Player(b2World* world, float startX = 640.f, float startY = 200.f);
    ~Player();

    // update logic (physics already stepped by Game/Level)
    void Update(float dt, bool grounded);

    // update visual sprite from physics body
    void SyncGraphics();

    // draw the player sprite
    void Draw(sf::RenderWindow& window);

    // physics accessors
    b2Body* GetBody() const { return m_body; }
    b2Fixture* GetFootFixture() const { return m_footFixture; }
    b2Vec2 GetPosition() const { return m_body ? m_body->GetPosition() : b2Vec2_zero; }
    b2Vec2 GetLinearVelocity() const { return m_body ? m_body->GetLinearVelocity() : b2Vec2_zero; }
    void SetLinearVelocity(const b2Vec2& v) { if (m_body) m_body->SetLinearVelocity(v); }

    // visual helpers (color tint on sprite) - disabled to use sprite only
    void SetColor(const sf::Color& /*c*/) { /* No-op: tinting disabled */ }
    void SetOrigin(float ox, float oy) { m_sprite.setOrigin(ox, oy); }

    // Audio mood/state for driving music
    void SetAudioState(PlayerAudioState s) { m_audioState = s; }
    PlayerAudioState GetAudioState() const { return m_audioState; }

    // Movement state hint (set by input layer)
    void SetWalking(bool walking) { m_isWalking = walking; }

private:
    void applyCollisionFromSprite();

private:
    b2World* m_world;
    b2Body* m_body;
    b2Fixture* m_footFixture;

    // Visuals
    sf::Sprite m_sprite;
    Animation m_anim;

    // State
    bool m_facingRight;
    bool m_isWalking = false;
    PlayerAudioState m_audioState = PlayerAudioState::Neutral;
};