#include "Animation.h"

Animation::Animation()
    : m_currentFrameIndex(0),
    m_accum(0.f),
    m_sprite(nullptr),
    m_facingRight(true)
{
}

bool Animation::AddClip(const std::string& name, const std::vector<std::string>& framePaths, float frameTimeSeconds, bool loop)
{
    Clip clip;
    clip.frameTimeSeconds = frameTimeSeconds;
    clip.loop = loop;

    clip.frames.reserve(framePaths.size());
    for (const auto& path : framePaths) {
        sf::Texture tex;
        if (!tex.loadFromFile(path)) {
            // If a frame fails to load, the entire clip is considered failed
            return false;
        }
        tex.setSmooth(true);
        clip.frames.emplace_back(std::move(tex));
    }

    m_clips[name] = std::move(clip);

    // If no current clip, set this one as default
    if (m_currentClipName.empty()) {
        SetClip(name, true);
    }

    return true;
}

bool Animation::SetClip(const std::string& name, bool resetFrameIndex)
{
    auto it = m_clips.find(name);
    if (it == m_clips.end()) return false;

    m_currentClipName = name;
    if (resetFrameIndex) {
        m_currentFrameIndex = 0;
        m_accum = 0.f;
        applyFrame();
    }
    else {
        applyFrame();
    }
    return true;
}

void Animation::Update(float dt)
{
    if (m_currentClipName.empty()) return;
    auto it = m_clips.find(m_currentClipName);
    if (it == m_clips.end()) return;

    Clip& clip = it->second;
    if (clip.frames.empty()) return;

    m_accum += dt;
    while (m_accum >= clip.frameTimeSeconds) {
        m_accum -= clip.frameTimeSeconds;
        if (m_currentFrameIndex + 1 < clip.frames.size()) {
            m_currentFrameIndex++;
        }
        else {
            if (clip.loop) {
                m_currentFrameIndex = 0;
            }
            else {
                // stay on last frame if not looping
            }
        }
        applyFrame();
    }
}

void Animation::BindSprite(sf::Sprite* sprite)
{
    m_sprite = sprite;
    applyFrame();
    // Apply facing scale
    if (m_sprite) m_sprite->setScale(m_facingRight ? 1.f : -1.f, 1.f);
}

void Animation::Reset()
{
    m_accum = 0.f;
    m_currentFrameIndex = 0;
    applyFrame();
}

void Animation::SetFacingRight(bool right)
{
    if (m_facingRight == right) return;
    m_facingRight = right;
    if (m_sprite) {
        sf::Vector2f s = m_sprite->getScale();
        m_sprite->setScale(right ? std::abs(s.x) : -std::abs(s.x), s.y);
    }
}

std::size_t Animation::CurrentFrameCount() const
{
    auto it = m_clips.find(m_currentClipName);
    if (it == m_clips.end()) return 0;
    return it->second.frames.size();
}

void Animation::applyFrame()
{
    if (!m_sprite) return;
    auto it = m_clips.find(m_currentClipName);
    if (it == m_clips.end()) return;
    Clip& clip = it->second;
    if (clip.frames.empty()) return;

    m_sprite->setTexture(clip.frames[m_currentFrameIndex], true);

    // Keep origin centered to make horizontal flip stable
    const sf::Vector2u size = clip.frames[m_currentFrameIndex].getSize();
    m_sprite->setOrigin(static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f);

    // Re-apply facing scale in case texture size change affects appearance
    sf::Vector2f s = m_sprite->getScale();
    m_sprite->setScale(m_facingRight ? std::abs(s.x) : -std::abs(s.x), s.y);
}