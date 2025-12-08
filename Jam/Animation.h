#ifndef ANIMATION_H
#define ANIMATION_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>

class Animation {
public:
    struct Clip {
        std::vector<sf::Texture> frames;
        float frameTimeSeconds = 0.1f; // default per-frame time
        bool loop = true;
    };

    Animation();

    // Create a clip and load frames from file paths
    bool AddClip(const std::string& name, const std::vector<std::string>& framePaths, float frameTimeSeconds, bool loop);

    // Switch current clip
    bool SetClip(const std::string& name, bool resetFrameIndex = true);

    // Advance animation time and update sprite texture
    void Update(float dt);

    // Bind a sprite to this animation for texture swapping
    void BindSprite(sf::Sprite* sprite);

    // Reset current clip state
    void Reset();

    // Facing helpers (does not change origin, only scale)
    void SetFacingRight(bool right);
    bool IsFacingRight() const { return m_facingRight; }

    // Accessors
    const std::string& CurrentClip() const { return m_currentClipName; }
    std::size_t CurrentFrameIndex() const { return m_currentFrameIndex; }
    std::size_t CurrentFrameCount() const;

private:
    void applyFrame();

private:
    std::unordered_map<std::string, Clip> m_clips;
    std::string m_currentClipName;
    std::size_t m_currentFrameIndex;
    float m_accum; // seconds
    sf::Sprite* m_sprite;
    bool m_facingRight;
};

#endif // ANIMATION_H