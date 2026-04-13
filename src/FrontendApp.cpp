#include "FrontendApp.h"

#include <SFML/Audio.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>

using namespace std;

namespace
{
sf::FloatRect rectf(float x, float y, float w, float h)
{
    return sf::FloatRect({x, y}, {w, h});
}
}

FrontendApp::FrontendApp()
    : m_game(),
      m_window(),
      m_font(),
      m_animationClock(),
      m_screenTransitionClock(),
      m_battleFlashClock(),
      m_battlePresentationClock(),
      m_battleFlashColor(sf::Color::White),
      m_currentScreen(Screen::LanguageSelect),
      m_battleOverlay(BattleOverlay::None),
      m_selectedMenuIndex(1),
      m_selectedLanguageIndex(0),
      m_selectedHeroAppearanceIndex(0),
      m_selectedMonsterIndex(0),
      m_selectedBattleActionIndex(0),
      m_selectedOverlayIndex(0),
      m_selectedBestiaryIndex(0),
      m_selectedItemIndex(0),
      m_languageCode("en"),
      m_statusText("Select Start Battle to enter the graphical flow."),
      m_fullscreen(false),
      m_battlePresentationPhase(BattlePresentationPhase::Idle),
      m_displayedPlayerHp(0),
      m_displayedMonsterHp(0),
      m_displayedMercy(0),
      m_startPlayerHp(0),
      m_startMonsterHp(0),
      m_startMercy(0),
      m_targetPlayerHp(0),
      m_targetMonsterHp(0),
      m_targetMercy(0),
      m_hasPendingScreenChange(false),
      m_pendingScreen(Screen::MonsterSelect),
      m_pendingScreenStatus(),
      m_hasCachedBattleView(false),
      m_cachedBattleView(),
      m_pendingPlayerAttackId(),
      m_pendingMonsterFxClip(),
      m_pendingMonsterFlashColor(sf::Color(255, 132, 92))
{
}

bool FrontendApp::initialize()
{
    if (!m_game.initializeForFrontend("Traveler"))
    {
        return false;
    }

    recreateWindow();
    if (!loadFont())
    {
        return false;
    }

    loadOptionalTextures();
    loadOptionalAnimations();
    loadOptionalSounds();
    return true;
}

void FrontendApp::run()
{
    while (m_window.isOpen())
    {
        processEvents();
        render();
    }
}

bool FrontendApp::loadFont()
{
    const array<string, 4> fontCandidates = {
        "assets/fonts/PressStart2P-Regular.ttf",
        "assets/fonts/VT323-Regular.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (const string& candidate : fontCandidates)
    {
        if (m_font.openFromFile(candidate))
        {
            return true;
        }
    }

    return false;
}

void FrontendApp::loadOptionalTextures()
{
    namespace fs = std::filesystem;

    const array<pair<string, string>, 12> fixedTextures = {{
        {"bg_sunken_mire", "assets/backgrounds/sunken_mire.png"},
        {"bg_glass_dunes", "assets/backgrounds/glass_dunes.png"},
        {"bg_signal_wastes", "assets/backgrounds/signal_wastes.png"},
        {"bg_ancient_vault", "assets/backgrounds/ancient_vault.png"},
        {"portrait_player_default", "assets/portraits/player_default.png"},
        {"portrait_player_wanderer", "assets/portraits/player_wanderer.png"},
        {"portrait_player_vanguard", "assets/portraits/player_vanguard.png"},
        {"portrait_player_mystic", "assets/portraits/player_mystic.png"},
        {"sprite_player_default", "assets/sprites/player_default.png"},
        {"sprite_player_wanderer", "assets/sprites/player_wanderer.png"},
        {"sprite_player_vanguard", "assets/sprites/player_vanguard.png"},
        {"sprite_player_mystic", "assets/sprites/player_mystic.png"}
    }};

    for (const auto& [key, path] : fixedTextures)
    {
        if (!fs::exists(path))
        {
            continue;
        }

        sf::Texture texture;
        if (texture.loadFromFile(path))
        {
            m_textures[key] = std::move(texture);
        }
    }

    const array<pair<string, string>, 3> dynamicDirectories = {{
        {"portrait_", "assets/portraits"},
        {"sprite_", "assets/sprites"},
        {"ui_", "assets/ui"}
    }};

    for (const auto& [prefix, directory] : dynamicDirectories)
    {
        if (!fs::exists(directory))
        {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(directory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (entry.path().extension() != ".png")
            {
                continue;
            }

            sf::Texture texture;
            if (texture.loadFromFile(entry.path().string()))
            {
                m_textures[prefix + toAssetSlug(entry.path().stem().string())] = std::move(texture);
            }
        }
    }
}

void FrontendApp::loadOptionalAnimations()
{
    loadAnimationDirectory("player_wanderer_idle", "assets/animations/player_wanderer/idle", 0.18f);
    loadAnimationDirectory("player_wanderer_attack", "assets/animations/player_wanderer/attack", 0.09f, false);
    loadAnimationDirectory("player_vanguard_idle", "assets/animations/player_vanguard/idle", 0.16f);
    loadAnimationDirectory("player_vanguard_attack", "assets/animations/player_vanguard/attack", 0.08f, false);
    loadAnimationDirectory("player_mystic_idle", "assets/animations/player_mystic/idle", 0.12f);

    loadAnimationDirectory("queenbyte_idle", "assets/animations/queenbyte/idle", 0.16f);
    loadAnimationDirectory("queenbyte_attack", "assets/animations/queenbyte/attack", 0.08f, false);
    loadAnimationDirectory("queenbyte_hurt", "assets/animations/queenbyte/hurt", 0.11f, false);

    loadAnimationDirectory("archivore_idle", "assets/animations/archivore/idle", 0.17f);
    loadAnimationDirectory("archivore_attack", "assets/animations/archivore/attack", 0.09f, false);
    loadAnimationDirectory("archivore_hurt", "assets/animations/archivore/hurt", 0.12f, false);

    loadAnimationDirectory("bloomcobra_idle", "assets/animations/bloomcobra/idle", 0.14f);
    loadAnimationDirectory("bloomcobra_attack", "assets/animations/bloomcobra/attack", 0.12f, false);
    loadAnimationDirectory("bloomcobra_hurt", "assets/animations/bloomcobra/hurt", 0.12f, false);

    loadAnimationDirectory("fx_fire", "assets/animations/fx_fire", 0.07f, false);
    loadAnimationDirectory("fx_arcane", "assets/animations/fx_arcane", 0.05f, false);
    loadAnimationDirectory("fx_blade", "assets/animations/fx_blade", 0.06f, false);
    loadAnimationDirectory("fx_water", "assets/animations/fx_water", 0.08f, false);
    loadAnimationDirectory("fx_hit", "assets/animations/fx_hit", 0.07f, false);
}

void FrontendApp::loadAnimationDirectory(const string& clipKey, const string& directoryPath, float frameDuration, bool loop)
{
    namespace fs = std::filesystem;

    if (!fs::exists(directoryPath))
    {
        return;
    }

    vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".png")
        {
            continue;
        }

        paths.push_back(entry.path());
    }

    sort(paths.begin(), paths.end());
    if (paths.empty())
    {
        return;
    }

    AnimationClip clip;
    clip.frameDuration = frameDuration;
    clip.loop = loop;

    for (const fs::path& path : paths)
    {
        sf::Texture texture;
        if (texture.loadFromFile(path.string()))
        {
            clip.frames.push_back(std::move(texture));
        }
    }

    if (!clip.frames.empty())
    {
        m_animationClips[clipKey] = std::move(clip);
    }
}

void FrontendApp::loadOptionalSounds()
{
    namespace fs = std::filesystem;

    const array<pair<string, string>, 10> soundCandidates = {{
        {"ui_move", "assets/sfx/ui_move.wav"},
        {"ui_confirm", "assets/sfx/ui_confirm.wav"},
        {"battle_hit", "assets/sfx/battle_hit.wav"},
        {"battle_act", "assets/sfx/battle_act.wav"},
        {"battle_item", "assets/sfx/battle_item.wav"},
        {"battle_mercy", "assets/sfx/battle_mercy.wav"},
        {"battle_cast", "assets/sfx/battle_cast.wav"},
        {"battle_heal", "assets/sfx/battle_heal.wav"},
        {"battle_victory", "assets/sfx/battle_victory.wav"},
        {"battle_defeat", "assets/sfx/battle_defeat.wav"}
    }};

    for (const auto& [key, path] : soundCandidates)
    {
        if (!fs::exists(path))
        {
            continue;
        }

        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(path))
        {
            continue;
        }

        m_soundBuffers[key] = std::move(buffer);
        m_sounds[key] = std::make_unique<sf::Sound>(m_soundBuffers.at(key));
    }
}

void FrontendApp::playSoundIfAvailable(const string& soundKey, float volume)
{
    const auto soundIt = m_sounds.find(soundKey);
    if (soundIt == m_sounds.end() || !soundIt->second)
    {
        return;
    }

    soundIt->second->stop();
    soundIt->second->setVolume(volume);
    soundIt->second->play();
}

void FrontendApp::recreateWindow()
{
    const sf::VideoMode mode = m_fullscreen ? sf::VideoMode::getDesktopMode() : sf::VideoMode({1280u, 720u});
    const sf::State state = m_fullscreen ? sf::State::Fullscreen : (sf::State::Windowed);
    m_window.create(mode, "ALTERDUNE Frontend", state);
    m_window.setFramerateLimit(60);
}

void FrontendApp::processEvents()
{
    while (const optional<sf::Event> event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
            continue;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::F11)
            {
                m_fullscreen = !m_fullscreen;
                recreateWindow();
                continue;
            }
        }

        switch (m_currentScreen)
        {
        case Screen::LanguageSelect: handleLanguageSelectEvent(*event); break;
        case Screen::HeroSelect: handleHeroSelectEvent(*event); break;
        case Screen::Menu: handleMenuEvent(*event); break;
        case Screen::PlayerStats: handlePlayerStatsEvent(*event); break;
        case Screen::Bestiary: handleBestiaryEvent(*event); break;
        case Screen::Items: handleItemsEvent(*event); break;
        case Screen::MonsterSelect: handleMonsterSelectEvent(*event); break;
        case Screen::Battle: handleBattleEvent(*event); break;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mousePressed->button != sf::Mouse::Button::Left)
            {
                continue;
            }

            const sf::Vector2f point(static_cast<float>(mousePressed->position.x),
                                     static_cast<float>(mousePressed->position.y));
            switch (m_currentScreen)
            {
            case Screen::LanguageSelect: handleLanguageSelectClick(point); break;
            case Screen::HeroSelect: handleHeroSelectClick(point); break;
            case Screen::Menu: handleMenuClick(point); break;
            case Screen::PlayerStats: handlePlayerStatsClick(point); break;
            case Screen::Bestiary: handleBestiaryClick(point); break;
            case Screen::Items: handleItemsClick(point); break;
            case Screen::MonsterSelect: handleMonsterSelectClick(point); break;
            case Screen::Battle: handleBattleClick(point); break;
            }
        }
    }
}

void FrontendApp::render()
{
    switch (m_currentScreen)
    {
    case Screen::LanguageSelect: renderLanguageSelect(); break;
    case Screen::HeroSelect: renderHeroSelect(); break;
    case Screen::Menu: renderMenu(); break;
    case Screen::PlayerStats: renderPlayerStats(); break;
    case Screen::Bestiary: renderBestiary(); break;
    case Screen::Items: renderItems(); break;
    case Screen::MonsterSelect: renderMonsterSelect(); break;
    case Screen::Battle: renderBattle(); break;
    }

    renderTransitionOverlay();
    m_window.display();
}

void FrontendApp::drawPanel(const sf::FloatRect& rect, const sf::Color& fill, float outlineThickness)
{
    sf::RectangleShape panel(rect.size);
    panel.setPosition(rect.position);
    panel.setFillColor(fill);
    panel.setOutlineColor(sf::Color(235, 222, 196));
    panel.setOutlineThickness(outlineThickness);
    m_window.draw(panel);
}

void FrontendApp::renderTransitionOverlay()
{
    const float elapsed = m_screenTransitionClock.getElapsedTime().asSeconds();
    if (elapsed >= 0.35f)
    {
        return;
    }

    const float t = 1.f - (elapsed / 0.35f);
    sf::RectangleShape fade({1280.f, 720.f});
    fade.setFillColor(sf::Color(10, 10, 14, static_cast<uint8_t>(160.f * std::clamp(t, 0.f, 1.f))));
    m_window.draw(fade);

    for (int i = 0; i < 5; ++i)
    {
        sf::RectangleShape streak({1280.f, 14.f + i * 3.f});
        streak.setPosition({0.f, 120.f + i * 110.f + t * 48.f});
        streak.setFillColor(sf::Color(255, 220, 153, static_cast<uint8_t>(26.f * std::clamp(t, 0.f, 1.f))));
        m_window.draw(streak);
    }
}

void FrontendApp::drawBackgroundGradient(const sf::Color& topColor, const sf::Color& bottomColor)
{
    sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
    quad[0].position = {0.f, 0.f};
    quad[1].position = {1280.f, 0.f};
    quad[2].position = {0.f, 720.f};
    quad[3].position = {1280.f, 720.f};
    quad[0].color = topColor;
    quad[1].color = topColor;
    quad[2].color = bottomColor;
    quad[3].color = bottomColor;
    m_window.draw(quad);
}

void FrontendApp::drawBackgroundTexture(const string& textureKey, const sf::Color& fallbackTop, const sf::Color& fallbackBottom)
{
    if (drawTextureIfAvailable(textureKey, rectf(0.f, 0.f, 1280.f, 720.f), sf::Color(255, 255, 255, 225)))
    {
        return;
    }

    drawBackgroundGradient(fallbackTop, fallbackBottom);
}

void FrontendApp::drawBattleSilhouette(const sf::FloatRect& area,
                                       bool monsterSide,
                                       const string& variantKey,
                                       const sf::Color& fillColor,
                                       const sf::Color& shadowColor)
{
    const size_t seed = hash<string>{}(variantKey + (monsterSide ? "_monster" : "_player"));
    const float bob = std::sin(m_animationClock.getElapsedTime().asSeconds() * 3.2f + (monsterSide ? 1.3f : 0.f)) * 6.f;
    const float bodyWidthFactor = 0.22f + static_cast<float>(seed % 9) * 0.012f;
    const float bodyHeightFactor = 0.30f + static_cast<float>((seed / 7) % 7) * 0.015f;
    const float headRadiusFactor = 0.08f + static_cast<float>((seed / 13) % 4) * 0.01f;

    sf::CircleShape shadow(area.size.x * 0.22f);
    shadow.setScale({1.7f, 0.45f});
    shadow.setFillColor(shadowColor);
    shadow.setPosition({area.position.x + area.size.x * 0.18f, area.position.y + area.size.y * 0.72f});
    m_window.draw(shadow);

    sf::RectangleShape body({area.size.x * bodyWidthFactor, area.size.y * bodyHeightFactor});
    body.setFillColor(fillColor);
    body.setPosition({area.position.x + area.size.x * (monsterSide ? 0.34f : 0.24f), area.position.y + area.size.y * 0.28f + bob});
    m_window.draw(body);

    sf::CircleShape head(area.size.x * headRadiusFactor, 6 + static_cast<size_t>((seed / 17) % 4));
    head.setFillColor(fillColor);
    head.setPosition({area.position.x + area.size.x * (monsterSide ? 0.39f : 0.29f), area.position.y + area.size.y * 0.16f + bob});
    m_window.draw(head);

    sf::RectangleShape arm({area.size.x * 0.08f, area.size.y * 0.18f});
    arm.setFillColor(fillColor);
    arm.setRotation(sf::degrees(monsterSide ? -25.f : 25.f));
    arm.setPosition({area.position.x + area.size.x * (monsterSide ? 0.31f : 0.47f), area.position.y + area.size.y * 0.34f + bob});
    m_window.draw(arm);

    sf::RectangleShape leg({area.size.x * 0.06f, area.size.y * 0.18f});
    leg.setFillColor(fillColor);
    leg.setRotation(sf::degrees(monsterSide ? 10.f : -10.f));
    leg.setPosition({area.position.x + area.size.x * (monsterSide ? 0.38f : 0.31f), area.position.y + area.size.y * 0.57f + bob});
    m_window.draw(leg);

    leg.setRotation(sf::degrees(monsterSide ? -10.f : 10.f));
    leg.setPosition({area.position.x + area.size.x * (monsterSide ? 0.46f : 0.39f), area.position.y + area.size.y * 0.57f + bob});
    m_window.draw(leg);

    if ((seed % 3) == 0)
    {
        sf::RectangleShape horn({area.size.x * 0.05f, area.size.y * 0.12f});
        horn.setFillColor(fillColor);
        horn.setRotation(sf::degrees(monsterSide ? 30.f : -30.f));
        horn.setPosition({area.position.x + area.size.x * (monsterSide ? 0.48f : 0.28f), area.position.y + area.size.y * 0.08f + bob});
        m_window.draw(horn);
    }

    if ((seed % 5) <= 1)
    {
        sf::RectangleShape tail({area.size.x * 0.12f, area.size.y * 0.05f});
        tail.setFillColor(fillColor);
        tail.setRotation(sf::degrees(monsterSide ? 18.f : -18.f));
        tail.setPosition({area.position.x + area.size.x * (monsterSide ? 0.24f : 0.46f), area.position.y + area.size.y * 0.48f + bob});
        m_window.draw(tail);
    }
}

void FrontendApp::drawPortrait(const sf::FloatRect& area,
                               const string& seedText,
                               bool playerPortrait,
                               const sf::Color& accentColor)
{
    string portraitKey = "portrait_" + toAssetSlug(seedText);
    drawPanel(area, sf::Color(30, 30, 34), 4.f);
    drawPanel(rectf(area.position.x + 4.f, area.position.y + 4.f, area.size.x - 8.f, 10.f),
              sf::Color(accentColor.r, accentColor.g, accentColor.b, 210),
              0.f);
    drawPanel(rectf(area.position.x + 8.f, area.position.y + 8.f, area.size.x - 16.f, area.size.y - 16.f),
              playerPortrait ? sf::Color(45, 56, 66) : sf::Color(52, 45, 40),
              2.f);

    const RenderProfile profile = getRenderProfile(toAssetSlug(seedText));
    const sf::FloatRect contentArea = rectf(area.position.x + 10.f, area.position.y + 18.f, area.size.x - 20.f, area.size.y - 28.f);

    if (playerPortrait && !drawContainedTextureWithProfile(portraitKey, contentArea, profile, sf::Color::White, true))
    {
        portraitKey = "portrait_player_default";
    }
    if (!playerPortrait && drawContainedTextureWithProfile(portraitKey, contentArea, profile, sf::Color::White, true))
    {
        return;
    }
    if (drawContainedTextureWithProfile(portraitKey, contentArea, profile, sf::Color::White, true))
    {
        return;
    }

    const size_t seed = hash<string>{}(seedText + (playerPortrait ? "_player" : "_monster"));
    const sf::Color frame = playerPortrait ? sf::Color(124, 201, 222) : accentColor;
    const sf::Color fillA(static_cast<uint8_t>(80 + (seed % 120)), static_cast<uint8_t>(70 + ((seed / 7) % 100)), static_cast<uint8_t>(90 + ((seed / 19) % 110)));
    const sf::Color fillB(static_cast<uint8_t>(110 + ((seed / 5) % 100)), static_cast<uint8_t>(90 + ((seed / 13) % 110)), static_cast<uint8_t>(80 + ((seed / 17) % 100)));

    sf::RectangleShape backdrop({area.size.x - 24.f, area.size.y - 24.f});
    backdrop.setPosition({area.position.x + 12.f, area.position.y + 12.f});
    backdrop.setFillColor(sf::Color(fillA.r, fillA.g, fillA.b, 210));
    m_window.draw(backdrop);

    sf::CircleShape head(area.size.x * 0.16f, 7);
    head.setFillColor(fillB);
    head.setPosition({area.position.x + area.size.x * 0.33f, area.position.y + area.size.y * 0.16f});
    m_window.draw(head);

    sf::RectangleShape torso({area.size.x * 0.22f, area.size.y * 0.26f});
    torso.setFillColor(fillB);
    torso.setPosition({area.position.x + area.size.x * 0.35f, area.position.y + area.size.y * 0.42f});
    m_window.draw(torso);

    sf::RectangleShape accent({area.size.x * 0.08f, area.size.y * 0.08f});
    accent.setFillColor(frame);
    accent.setPosition({area.position.x + area.size.x * (playerPortrait ? 0.18f : 0.68f), area.position.y + area.size.y * 0.18f});
    m_window.draw(accent);

    sf::RectangleShape accent2({area.size.x * 0.12f, area.size.y * 0.05f});
    accent2.setFillColor(frame);
    accent2.setPosition({area.position.x + area.size.x * 0.22f, area.position.y + area.size.y * 0.76f});
    m_window.draw(accent2);
}

void FrontendApp::drawDecorationRow(float y, const sf::Color& color, float spacing, float size)
{
    for (float x = 48.f; x < 1232.f; x += spacing)
    {
        sf::RectangleShape pixel({size, size});
        pixel.setFillColor(color);
        pixel.setPosition({x, y});
        m_window.draw(pixel);
    }
}

bool FrontendApp::drawTextureIfAvailable(const string& textureKey, const sf::FloatRect& area, const sf::Color& tint)
{
    const auto it = m_textures.find(textureKey);
    if (it == m_textures.end())
    {
        return false;
    }

    sf::Sprite sprite(it->second);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
    {
        return false;
    }

    sprite.setColor(tint);
    sprite.setPosition(area.position);
    sprite.setScale({area.size.x / bounds.size.x, area.size.y / bounds.size.y});
    m_window.draw(sprite);
    return true;
}

bool FrontendApp::drawContainedTextureIfAvailable(const string& textureKey,
                                                  const sf::FloatRect& area,
                                                  const sf::Color& tint,
                                                  float padding,
                                                  bool alignBottom)
{
    const auto it = m_textures.find(textureKey);
    if (it == m_textures.end())
    {
        return false;
    }

    sf::Sprite sprite(it->second);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
    {
        return false;
    }

    const float innerWidth = std::max(1.f, area.size.x - padding * 2.f);
    const float innerHeight = std::max(1.f, area.size.y - padding * 2.f);
    const float scaleFactor = std::min(innerWidth / bounds.size.x, innerHeight / bounds.size.y);
    const float drawWidth = bounds.size.x * scaleFactor;
    const float drawHeight = bounds.size.y * scaleFactor;
    const float drawX = area.position.x + (area.size.x - drawWidth) * 0.5f;
    const float drawY = alignBottom
        ? (area.position.y + area.size.y - drawHeight - padding)
        : (area.position.y + (area.size.y - drawHeight) * 0.5f);

    sprite.setColor(tint);
    sprite.setScale({scaleFactor, scaleFactor});
    sprite.setPosition({drawX, drawY});
    m_window.draw(sprite);
    return true;
}

bool FrontendApp::drawAnimationClipIfAvailable(const string& clipKey,
                                               const sf::FloatRect& area,
                                               const sf::Color& tint,
                                               float elapsedSeconds,
                                               bool flipHorizontally)
{
    const auto it = m_animationClips.find(clipKey);
    if (it == m_animationClips.end() || it->second.frames.empty())
    {
        return false;
    }

    const AnimationClip& clip = it->second;
    const float localElapsed = elapsedSeconds >= 0.f ? elapsedSeconds : m_animationClock.getElapsedTime().asSeconds();

    size_t frameIndex = 0;
    if (clip.frames.size() > 1 && clip.frameDuration > 0.f)
    {
        const size_t rawIndex = static_cast<size_t>(localElapsed / clip.frameDuration);
        frameIndex = clip.loop ? (rawIndex % clip.frames.size()) : std::min(rawIndex, clip.frames.size() - 1);
    }

    sf::Sprite sprite(clip.frames[frameIndex]);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
    {
        return false;
    }

    const float scaleFactor = std::min(area.size.x / bounds.size.x, area.size.y / bounds.size.y);
    const float drawWidth = bounds.size.x * scaleFactor;
    const float drawHeight = bounds.size.y * scaleFactor;
    const float drawY = area.position.y + area.size.y - drawHeight;

    sprite.setColor(tint);

    sf::Vector2f scale(scaleFactor, scaleFactor);
    if (flipHorizontally)
    {
        sprite.setPosition({area.position.x + (area.size.x + drawWidth) * 0.5f, drawY});
        scale.x *= -1.f;
    }
    else
    {
        sprite.setPosition({area.position.x + (area.size.x - drawWidth) * 0.5f, drawY});
    }
    sprite.setScale(scale);
    m_window.draw(sprite);
    return true;
}

bool FrontendApp::drawContainedTextureWithProfile(const string& textureKey,
                                                  const sf::FloatRect& area,
                                                  const RenderProfile& profile,
                                                  const sf::Color& tint,
                                                  bool alignBottom)
{
    const auto it = m_textures.find(textureKey);
    if (it == m_textures.end())
    {
        return false;
    }

    sf::Sprite sprite(it->second);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
    {
        return false;
    }

    const float innerWidth = std::max(1.f, area.size.x - profile.padding * 2.f);
    const float innerHeight = std::max(1.f, area.size.y - profile.padding * 2.f);
    const float baseScale = std::min(innerWidth / bounds.size.x, innerHeight / bounds.size.y);
    const float scaleFactor = baseScale * std::max(0.25f, profile.scaleMultiplier);
    const float drawWidth = bounds.size.x * scaleFactor;
    const float drawHeight = bounds.size.y * scaleFactor;
    const float drawX = area.position.x + (area.size.x - drawWidth) * 0.5f;
    const float baseY = alignBottom
        ? (area.position.y + area.size.y - drawHeight - profile.padding)
        : (area.position.y + (area.size.y - drawHeight) * 0.5f);
    const float drawY = baseY + area.size.y * profile.verticalBias;

    if (profile.glowColor.a > 0)
    {
        sf::CircleShape glow(std::max(drawWidth, drawHeight) * 0.34f);
        glow.setScale({1.6f, 1.04f});
        glow.setFillColor(profile.glowColor);
        glow.setPosition({drawX + drawWidth * 0.14f, drawY + drawHeight * 0.18f});
        m_window.draw(glow);
    }

    sprite.setColor(tint);
    sprite.setScale({scaleFactor, scaleFactor});
    sprite.setPosition({drawX, drawY});
    m_window.draw(sprite);
    return true;
}

bool FrontendApp::drawAnimationClipWithProfile(const string& clipKey,
                                               const sf::FloatRect& area,
                                               const RenderProfile& profile,
                                               const sf::Color& tint,
                                               float elapsedSeconds,
                                               bool flipHorizontally)
{
    const auto it = m_animationClips.find(clipKey);
    if (it == m_animationClips.end() || it->second.frames.empty())
    {
        return false;
    }

    const AnimationClip& clip = it->second;
    const float localElapsed = elapsedSeconds >= 0.f ? elapsedSeconds : m_animationClock.getElapsedTime().asSeconds();

    size_t frameIndex = 0;
    if (clip.frames.size() > 1 && clip.frameDuration > 0.f)
    {
        const size_t rawIndex = static_cast<size_t>(localElapsed / clip.frameDuration);
        frameIndex = clip.loop ? (rawIndex % clip.frames.size()) : std::min(rawIndex, clip.frames.size() - 1);
    }

    sf::Sprite sprite(clip.frames[frameIndex]);
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
    {
        return false;
    }

    const float baseScale = std::min(area.size.x / bounds.size.x, area.size.y / bounds.size.y);
    const float scaleFactor = baseScale * std::max(0.25f, profile.scaleMultiplier);
    const float drawWidth = bounds.size.x * scaleFactor;
    const float drawHeight = bounds.size.y * scaleFactor;
    const float drawY = area.position.y + area.size.y - drawHeight + area.size.y * profile.verticalBias;

    if (profile.glowColor.a > 0)
    {
        sf::CircleShape glow(std::max(drawWidth, drawHeight) * 0.32f);
        glow.setScale({1.55f, 1.f});
        glow.setFillColor(profile.glowColor);
        glow.setPosition({area.position.x + area.size.x * 0.5f - glow.getRadius(), drawY + drawHeight * 0.15f});
        m_window.draw(glow);
    }

    sprite.setColor(tint);
    sf::Vector2f scale(scaleFactor, scaleFactor);
    if (flipHorizontally)
    {
        sprite.setPosition({area.position.x + (area.size.x + drawWidth) * 0.5f, drawY});
        scale.x *= -1.f;
    }
    else
    {
        sprite.setPosition({area.position.x + (area.size.x - drawWidth) * 0.5f, drawY});
    }

    sprite.setScale(scale);
    m_window.draw(sprite);
    return true;
}

bool FrontendApp::drawCharacterShowcase(const string& baseSlug,
                                        const sf::FloatRect& area,
                                        bool playerSide,
                                        const sf::Color& fallbackFill,
                                        const sf::Color& fallbackShadow)
{
    const RenderProfile profile = getRenderProfile(baseSlug);
    if (drawAnimationClipWithProfile(baseSlug + "_idle", area, profile, sf::Color::White, -1.f, false))
    {
        return true;
    }

    if (drawContainedTextureWithProfile("sprite_" + baseSlug, area, profile, sf::Color::White, true))
    {
        return true;
    }

    drawBattleSilhouette(area, !playerSide, baseSlug, fallbackFill, fallbackShadow);
    return false;
}

bool FrontendApp::drawBattlerAnimation(BattleAnimationState& state,
                                       const sf::FloatRect& area,
                                       const sf::Color& tint,
                                       bool flipHorizontally)
{
    const bool idleSuffix = state.idleKey.size() >= 5 && state.idleKey.substr(state.idleKey.size() - 5) == "_idle";
    const string profileKey = idleSuffix ? state.idleKey.substr(0, state.idleKey.size() - 5) : state.idleKey;
    const RenderProfile profile = getRenderProfile(profileKey);

    if (!state.overrideKey.empty() && state.overrideClock.getElapsedTime().asSeconds() < state.overrideDurationSeconds)
    {
        return drawAnimationClipWithProfile(state.overrideKey,
                                            area,
                                            profile,
                                            tint,
                                            state.overrideClock.getElapsedTime().asSeconds(),
                                            flipHorizontally);
    }

    state.overrideKey.clear();

    if (!state.idleKey.empty())
    {
        return drawAnimationClipWithProfile(state.idleKey, area, profile, tint, -1.f, flipHorizontally);
    }

    return false;
}

void FrontendApp::renderBattleEnvironment(const string& regionName,
                                          const string& elementType,
                                          const string& category,
                                          const string& monsterSlug)
{
    const sf::Color regionBase = getRegionColor(regionName);
    const sf::Color elementTint = getElementColor(elementType);
    drawBackgroundTexture("bg_" + toAssetSlug(regionName),
                          sf::Color(regionBase.r / 2, regionBase.g / 2, regionBase.b / 2),
                          regionBase);

    sf::RectangleShape ambient({1280.f, 720.f});
    ambient.setFillColor(sf::Color(elementTint.r, elementTint.g, elementTint.b, category == "BOSS" ? 76 : 46));
    m_window.draw(ambient);

    sf::VertexArray dunes(sf::PrimitiveType::TriangleStrip, 6);
    dunes[0].position = {0.f, 470.f};
    dunes[1].position = {0.f, 720.f};
    dunes[2].position = {360.f, 420.f};
    dunes[3].position = {360.f, 720.f};
    dunes[4].position = {820.f, 500.f};
    dunes[5].position = {820.f, 720.f};
    for (size_t i = 0; i < 6; ++i)
    {
        dunes[i].color = sf::Color(regionBase.r / 2, regionBase.g / 2, regionBase.b / 2, 210);
    }
    m_window.draw(dunes);

    sf::VertexArray farHorizon(sf::PrimitiveType::TriangleStrip, 6);
    farHorizon[0].position = {460.f, 430.f};
    farHorizon[1].position = {460.f, 720.f};
    farHorizon[2].position = {880.f, 360.f};
    farHorizon[3].position = {880.f, 720.f};
    farHorizon[4].position = {1280.f, 460.f};
    farHorizon[5].position = {1280.f, 720.f};
    for (size_t i = 0; i < 6; ++i)
    {
        farHorizon[i].color = sf::Color(elementTint.r / 2, elementTint.g / 2, elementTint.b / 2, category == "BOSS" ? 190 : 130);
    }
    m_window.draw(farHorizon);

    for (int i = 0; i < (category == "BOSS" ? 30 : 16); ++i)
    {
        const float t = m_animationClock.getElapsedTime().asSeconds() * (category == "BOSS" ? 0.9f : 0.5f);
        const float x = 40.f + static_cast<float>((i * 79) % 1180);
        const float y = 70.f + std::sin(t + i * 0.55f) * 24.f + static_cast<float>((i * 23) % 220);
        sf::CircleShape mote(category == "BOSS" ? 4.f + static_cast<float>(i % 3) : 2.f + static_cast<float>(i % 2));
        mote.setFillColor(sf::Color(elementTint.r, elementTint.g, elementTint.b, category == "BOSS" ? 160 : 110));
        mote.setPosition({x, y});
        m_window.draw(mote);
    }

    if (category == "BOSS")
    {
        for (int i = 0; i < 7; ++i)
        {
            sf::RectangleShape pillar({18.f + i * 6.f, 150.f + i * 24.f});
            pillar.setFillColor(sf::Color(24 + i * 8, 24 + i * 8, 30 + i * 10, 110));
            pillar.setPosition({60.f + i * 170.f, 720.f - pillar.getSize().y});
            m_window.draw(pillar);
        }

        sf::CircleShape halo(110.f);
        halo.setScale({1.8f, 0.8f});
        halo.setPosition({760.f, 70.f});
        halo.setFillColor(sf::Color(elementTint.r, elementTint.g, elementTint.b, 70));
        m_window.draw(halo);

        if (elementType == "Fire")
        {
            for (int i = 0; i < 14; ++i)
            {
                sf::CircleShape ember(5.f + static_cast<float>(i % 3));
                ember.setFillColor(sf::Color(255, 164, 74, 90));
                ember.setPosition({120.f + i * 78.f, 120.f + std::sin(m_animationClock.getElapsedTime().asSeconds() * 2.2f + i) * 42.f});
                m_window.draw(ember);
            }
        }
        else if (elementType == "Water")
        {
            for (int i = 0; i < 5; ++i)
            {
                sf::CircleShape wave(55.f + i * 18.f);
                wave.setScale({2.4f, 0.26f});
                wave.setFillColor(sf::Color(96, 176, 255, 26));
                wave.setPosition({720.f - i * 80.f, 520.f + std::sin(m_animationClock.getElapsedTime().asSeconds() * 1.2f + i) * 10.f});
                m_window.draw(wave);
            }
        }
        else if (elementType == "Shadow")
        {
            for (int i = 0; i < 10; ++i)
            {
                sf::RectangleShape veil({250.f, 10.f});
                veil.setRotation(sf::degrees(-32.f + i * 4.f));
                veil.setPosition({640.f + i * 30.f, 60.f + i * 44.f});
                veil.setFillColor(sf::Color(115, 92, 180, 36));
                m_window.draw(veil);
            }
        }
        else if (elementType == "Light")
        {
            for (int i = 0; i < 8; ++i)
            {
                sf::RectangleShape beam({22.f, 380.f});
                beam.setFillColor(sf::Color(255, 244, 176, 34));
                beam.setPosition({720.f + i * 54.f, 0.f});
                m_window.draw(beam);
            }
        }
        else if (elementType == "Nature")
        {
            for (int i = 0; i < 8; ++i)
            {
                sf::RectangleShape vine({8.f, 160.f + i * 18.f});
                vine.setRotation(sf::degrees(-8.f + i * 4.f));
                vine.setFillColor(sf::Color(82, 148, 86, 70));
                vine.setPosition({80.f + i * 150.f, 460.f});
                m_window.draw(vine);
            }
        }

        if (monsterSlug == "queenbyte")
        {
            for (int i = 0; i < 9; ++i)
            {
                sf::RectangleShape scan({280.f, 3.f});
                scan.setPosition({760.f - i * 26.f, 136.f + i * 26.f});
                scan.setFillColor(sf::Color(120, 236, 255, 42));
                m_window.draw(scan);
            }

            for (int i = 0; i < 4; ++i)
            {
                sf::CircleShape node(16.f + i * 8.f);
                node.setFillColor(sf::Color::Transparent);
                node.setOutlineThickness(2.f);
                node.setOutlineColor(sf::Color(146, 220, 255, 64));
                node.setPosition({890.f + i * 48.f, 84.f + i * 16.f});
                m_window.draw(node);
            }
        }
        else if (monsterSlug == "archivore")
        {
            for (int i = 0; i < 5; ++i)
            {
                sf::RectangleShape shelf({220.f, 18.f});
                shelf.setPosition({820.f - i * 58.f, 82.f + i * 52.f});
                shelf.setFillColor(sf::Color(82, 58, 48, 86));
                m_window.draw(shelf);
            }

            for (int i = 0; i < 6; ++i)
            {
                sf::CircleShape sigil(22.f + i * 8.f);
                sigil.setFillColor(sf::Color::Transparent);
                sigil.setOutlineThickness(2.f);
                sigil.setOutlineColor(sf::Color(180, 118, 220, 40));
                sigil.setPosition({836.f - i * 18.f, 60.f + i * 12.f});
                m_window.draw(sigil);
            }
        }
    }

    if (regionName == "Sunken Mire")
    {
        for (int i = 0; i < 8; ++i)
        {
            sf::CircleShape ripple(24.f + i * 4.f);
            ripple.setScale({1.8f, 0.35f});
            ripple.setPosition({70.f + i * 150.f, 560.f + std::sin(m_animationClock.getElapsedTime().asSeconds() + i) * 8.f});
            ripple.setFillColor(sf::Color(120, 150, 132, 35));
            m_window.draw(ripple);
        }
    }
    else if (regionName == "Glass Dunes")
    {
        for (int i = 0; i < 18; ++i)
        {
            sf::RectangleShape shard({6.f, 14.f + static_cast<float>(i % 4) * 4.f});
            shard.setRotation(sf::degrees(12.f + i * 9.f));
            shard.setPosition({50.f + i * 68.f, 510.f + static_cast<float>((i % 3) * 24)});
            shard.setFillColor(sf::Color(190, 226, 232, 55));
            m_window.draw(shard);
        }
    }
    else if (regionName == "Signal Wastes")
    {
        for (int i = 0; i < 9; ++i)
        {
            sf::RectangleShape beam({180.f, 4.f});
            beam.setPosition({-20.f + i * 150.f, 110.f + std::sin(m_animationClock.getElapsedTime().asSeconds() * 1.8f + i) * 30.f});
            beam.setFillColor(sf::Color(elementTint.r, elementTint.g, elementTint.b, 70));
            m_window.draw(beam);
        }
    }
    else if (regionName == "Ancient Vault")
    {
        for (int i = 0; i < 6; ++i)
        {
            sf::CircleShape sigil(26.f + i * 8.f);
            sigil.setFillColor(sf::Color::Transparent);
            sigil.setOutlineThickness(2.f);
            sigil.setOutlineColor(sf::Color(elementTint.r, elementTint.g, elementTint.b, 50));
            sigil.setPosition({930.f - i * 42.f, 80.f + i * 18.f});
            m_window.draw(sigil);
        }
    }
}

void FrontendApp::drawText(const string& text, const sf::Vector2f& position, unsigned int size, const sf::Color& color, bool centered)
{
    sf::Text drawable(m_font, text, size);
    drawable.setFillColor(color);
    if (centered)
    {
        const sf::FloatRect localBounds = drawable.getLocalBounds();
        drawable.setOrigin({localBounds.position.x + localBounds.size.x / 2.f, 0.f});
    }
    drawable.setPosition(position);
    m_window.draw(drawable);
}

void FrontendApp::drawWrappedText(const string& text,
                                  const sf::Vector2f& position,
                                  unsigned int size,
                                  const sf::Color& color,
                                  float maxWidth,
                                  float lineSpacing)
{
    istringstream stream(text);
    string word;
    string currentLine;
    float y = position.y;

    while (stream >> word)
    {
        const string candidate = currentLine.empty() ? word : (currentLine + " " + word);
        sf::Text measure(m_font, candidate, size);
        if (!currentLine.empty() && measure.getLocalBounds().size.x > maxWidth)
        {
            drawText(currentLine, {position.x, y}, size, color);
            y += static_cast<float>(size) * lineSpacing;
            currentLine = word;
        }
        else
        {
            currentLine = candidate;
        }
    }

    if (!currentLine.empty())
    {
        drawText(currentLine, {position.x, y}, size, color);
    }
}

void FrontendApp::drawStatBar(const FrontendStatBarViewData& bar, const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& fillColor)
{
    drawText(bar.label, {position.x, position.y - 22.f}, 16, sf::Color::White);

    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color(30, 30, 30));
    m_window.draw(background);

    const float ratio = bar.maxValue <= 0 ? 0.f : static_cast<float>(bar.currentValue) / static_cast<float>(bar.maxValue);
    sf::RectangleShape fill({size.x * clamp(ratio, 0.f, 1.f), size.y});
    fill.setPosition(position);
    fill.setFillColor(fillColor);
    m_window.draw(fill);

    drawText(to_string(bar.currentValue) + " / " + to_string(bar.maxValue), {position.x + size.x + 12.f, position.y - 4.f}, 16, sf::Color::White);
}

void FrontendApp::renderMenu()
{
    const FrontendMenuViewData menuData = m_game.buildFrontendMenuViewData();
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);
    drawBackgroundTexture("bg_sunken_mire", sf::Color(21, 32, 44), sf::Color(67, 52, 38));

    sf::CircleShape moon(38.f);
    moon.setFillColor(sf::Color(244, 234, 194, 210));
    moon.setPosition({1020.f, 58.f});
    m_window.draw(moon);
    drawDecorationRow(162.f, sf::Color(248, 210, 132, 120), 24.f, 4.f);

    drawPanel(rectf(60.f, 40.f, 1160.f, 110.f), sf::Color(37, 47, 58));
    drawPanel(rectf(60.f, 180.f, 500.f, 460.f), sf::Color(47, 59, 54));
    drawPanel(rectf(590.f, 180.f, 630.f, 460.f), sf::Color(51, 44, 37));
    drawPanel(rectf(60.f, 660.f, 1160.f, 36.f), sf::Color(30, 30, 34));

    drawText("ALTERDUNE", {640.f, 58.f}, 40, sf::Color(245, 236, 198), true);
    drawText(tr("Frontend bonus", "Bonus frontend"), {640.f, 104.f}, 20, sf::Color(255, 214, 139), true);
    drawText(menuData.playerName, {90.f, 200.f}, 20, sf::Color::White);
    drawText(menuData.progressText, {90.f, 236.f}, 18, sf::Color(220, 235, 230));
    drawText(menuData.unlockedLandsText, {90.f, 268.f}, 17, sf::Color(255, 220, 153));

    for (size_t i = 0; i < menuData.buttons.size(); ++i)
    {
        const float y = 320.f + static_cast<float>(i) * 54.f;
        const bool selected = i == m_selectedMenuIndex;
        const sf::Color selectedColor(242, static_cast<uint8_t>(178 + 22 * pulse), 94);
        drawPanel(rectf(90.f, y, 320.f, 40.f), selected ? selectedColor : sf::Color(81, 97, 110), selected ? 5.f : 3.f);
        drawText(localizeMenuLabel(menuData.buttons[i]), {250.f, y + 7.f}, 18, selected ? sf::Color(30, 24, 15) : sf::Color::White, true);
    }

    drawText(tr("Controls", "Controles"), {620.f, 218.f}, 24, sf::Color(255, 214, 139));
    drawText(tr("Mouse click or use arrows + Enter.", "Clic souris ou fleches + Entree."), {620.f, 260.f}, 18, sf::Color::White);
    drawText(tr("Escape closes a panel or returns to a previous screen.", "Echap ferme un panneau ou fait revenir."), {620.f, 296.f}, 18, sf::Color::White);
    drawText(tr("Same battle logic, CSV data, mercy system and progression as console.", "Meme logique de combat, CSV, mercy et progression que la console."), {620.f, 348.f}, 18, sf::Color(210, 230, 240));
    drawText(tr("Bestiary and items update live after battles.", "Le bestiaire et les items se mettent a jour apres les combats."), {620.f, 382.f}, 18, sf::Color(210, 230, 240));
    drawText(tr("Retro goal: old-school handheld RPG vibe.", "Objectif retro : sensation RPG portable old-school."), {620.f, 434.f}, 18, sf::Color(255, 220, 153));
    drawText(m_statusText, {80.f, 666.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderLanguageSelect()
{
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);
    const array<pair<string, string>, 2> languages = {{{"en", "English"}, {"fr", "Francais"}}};

    drawBackgroundTexture("bg_sunken_mire", sf::Color(18, 28, 42), sf::Color(54, 46, 38));
    drawPanel(rectf(120.f, 70.f, 1040.f, 120.f), sf::Color(34, 43, 56));
    drawPanel(rectf(120.f, 230.f, 1040.f, 360.f), sf::Color(39, 47, 58));
    drawPanel(rectf(120.f, 620.f, 1040.f, 46.f), sf::Color(28, 28, 32));

    drawText(tr("Choose Language", "Choisir la langue"), {640.f, 100.f}, 34, sf::Color(245, 236, 198), true);
    drawText(tr("Arrows + Enter or mouse click", "Fleches + Entree ou clic souris"), {640.f, 146.f}, 18, sf::Color(210, 228, 240), true);

    for (size_t i = 0; i < languages.size(); ++i)
    {
        const float x = 220.f + static_cast<float>(i) * 430.f;
        const bool selected = i == m_selectedLanguageIndex;
        drawPanel(rectf(x, 310.f, 310.f, 180.f),
                  selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(70, 82, 94),
                  selected ? 5.f : 3.f);
        drawText(languages[i].second, {x + 155.f, 360.f}, 28, selected ? sf::Color(30, 24, 15) : sf::Color::White, true);
        drawText(languages[i].first == "en" ? "English UI" : "Interface francaise",
                 {x + 155.f, 408.f},
                 18,
                 selected ? sf::Color(30, 24, 15) : sf::Color(220, 232, 240),
                 true);
    }

    drawText(tr("This changes the frontend presentation first.", "Cela change d'abord la presentation du frontend."),
             {160.f, 634.f},
             15,
             sf::Color(235, 235, 235));
}

void FrontendApp::renderHeroSelect()
{
    const vector<string> appearanceIds = Game::getAvailableHeroAppearanceIds();
    m_selectedHeroAppearanceIndex = min(m_selectedHeroAppearanceIndex, appearanceIds.size() - 1);
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);

    drawBackgroundTexture("bg_sunken_mire", sf::Color(18, 28, 42), sf::Color(54, 46, 38));
    drawPanel(rectf(70.f, 40.f, 1140.f, 100.f), sf::Color(34, 43, 56));
    drawPanel(rectf(70.f, 170.f, 1140.f, 470.f), sf::Color(39, 47, 58));
    drawPanel(rectf(70.f, 654.f, 1140.f, 42.f), sf::Color(28, 28, 32));

    drawText(tr("Choose Your Hero", "Choisis ton heros"), {640.f, 58.f}, 34, sf::Color(245, 236, 198), true);
    drawText(tr("Arrows + Enter or mouse click", "Fleches + Entree ou clic souris"), {640.f, 102.f}, 18, sf::Color(210, 228, 240), true);

    for (size_t i = 0; i < appearanceIds.size(); ++i)
    {
        const float x = 118.f + static_cast<float>(i) * 350.f;
        const bool selected = i == m_selectedHeroAppearanceIndex;
        const string& appearanceId = appearanceIds[i];
        drawPanel(rectf(x, 218.f, 250.f, 320.f),
                  selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(70, 82, 94),
                  selected ? 5.f : 3.f);
        drawPanel(rectf(x + 18.f, 238.f, 214.f, 150.f), selected ? sf::Color(46, 56, 62) : sf::Color(30, 36, 42), 3.f);
        drawPanel(rectf(x + 30.f, 250.f, 190.f, 126.f), sf::Color(18, 22, 28, 150), 2.f);
        drawCharacterShowcase("player_" + appearanceId,
                              rectf(x + 28.f, 246.f, 194.f, 132.f),
                              true,
                              sf::Color(128, 205, 226),
                              sf::Color(14, 18, 24, 130));
        drawText(trAppearanceLabel(appearanceId), {x + 125.f, 404.f}, 22, selected ? sf::Color(30, 24, 15) : sf::Color::White, true);

        string subtitle = tr("Balanced explorer", "Explorateur equilibre");
        string styleText = tr("Versatile desert traveler", "Voyageur polyvalent du desert");
        if (appearanceId == "vanguard")
        {
            subtitle = tr("Bold frontline style", "Style de premiere ligne");
            styleText = tr("Heavy silhouette, direct stance, combat-ready gear", "Silhouette solide, posture frontale, equipement de combat");
        }
        else if (appearanceId == "mystic")
        {
            subtitle = tr("Arcane desert look", "Allure arcane du desert");
            styleText = tr("Sharper profile, arcane techwear, agile posture", "Profil fin, tenue arcano-tech, posture agile");
        }
        drawText(subtitle, {x + 125.f, 438.f}, 16, selected ? sf::Color(30, 24, 15) : sf::Color(220, 232, 240), true);
        drawText(styleText, {x + 26.f, 472.f}, 14, selected ? sf::Color(30, 24, 15) : sf::Color(245, 230, 196));
        drawText(tr("Visible avatar in battle and menus", "Avatar visible en combat et dans les menus"), {x + 26.f, 502.f}, 13, selected ? sf::Color(30, 24, 15) : sf::Color(215, 225, 235));
    }

    drawText(tr("Your current name is ", "Ton nom actuel est ") + m_game.buildFrontendMenuViewData().playerName + ".", {96.f, 664.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderPlayerStats()
{
    const FrontendPlayerStatsViewData stats = m_game.buildFrontendPlayerStatsViewData();
    drawBackgroundTexture("bg_sunken_mire", sf::Color(26, 35, 52), sf::Color(53, 68, 84));
    drawPanel(rectf(40.f, 30.f, 1200.f, 80.f), sf::Color(36, 44, 56));
    drawPanel(rectf(40.f, 126.f, 330.f, 560.f), sf::Color(43, 55, 67));
    drawPanel(rectf(392.f, 126.f, 848.f, 560.f), sf::Color(30, 36, 44));

    drawText(tr("Player Stats", "Statistiques"), {640.f, 52.f}, 30, sf::Color(245, 236, 198), true);
    drawText(tr("ESC to return", "ECHAP pour revenir"), {640.f, 84.f}, 16, sf::Color(205, 225, 240), true);

    drawPortrait(rectf(86.f, 168.f, 236.f, 236.f), "player_" + m_game.getPlayerAppearance(), true, sf::Color(124, 201, 222));
    drawText(stats.playerName, {204.f, 428.f}, 24, sf::Color(255, 224, 168), true);
    drawText(stats.routeText, {72.f, 476.f}, 16, sf::Color(220, 232, 240));
    drawText(stats.appearanceText, {72.f, 508.f}, 16, sf::Color(255, 220, 153));

    drawText(tr("Core Stats", "Stats principales"), {428.f, 154.f}, 24, sf::Color(255, 214, 139));
    drawStatBar(stats.hpBar, {428.f, 210.f}, {260.f, 22.f}, sf::Color(118, 215, 126));
    drawText("ATK: " + to_string(stats.atk), {428.f, 270.f}, 20, sf::Color::White);
    drawText("DEF: " + to_string(stats.def), {428.f, 306.f}, 20, sf::Color::White);

    drawText(tr("Progress", "Progression"), {760.f, 154.f}, 24, sf::Color(255, 214, 139));
    drawText("Victories: " + to_string(stats.victories) + "/10", {760.f, 210.f}, 20, sf::Color::White);
    drawText("Kills: " + to_string(stats.kills), {760.f, 246.f}, 20, sf::Color::White);
    drawText("Spares: " + to_string(stats.spares), {760.f, 282.f}, 20, sf::Color::White);

    drawPanel(rectf(428.f, 360.f, 760.f, 126.f), sf::Color(44, 56, 66));
    drawText(tr("Inventory", "Inventaire"), {452.f, 384.f}, 22, sf::Color(255, 214, 139));
    drawText("Slots used: " + to_string(stats.inventorySlots), {452.f, 422.f}, 18, sf::Color::White);
    drawText("Total healing stock: " + to_string(stats.totalHealingStock), {452.f, 454.f}, 18, sf::Color::White);

    drawPanel(rectf(428.f, 514.f, 760.f, 112.f), sf::Color(38, 47, 57));
    drawText(tr("Milestones", "Paliers"), {452.f, 538.f}, 22, sf::Color(255, 214, 139));
    drawText(stats.milestoneText, {452.f, 576.f}, 17, sf::Color(220, 232, 240));
    drawText(m_statusText, {72.f, 654.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderBestiary()
{
    const vector<FrontendBestiaryEntryViewData> entries = m_game.buildFrontendBestiaryViewData();
    m_selectedBestiaryIndex = entries.empty() ? 0 : min(m_selectedBestiaryIndex, entries.size() - 1);
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);

    drawBackgroundTexture("bg_signal_wastes", sf::Color(26, 32, 42), sf::Color(48, 60, 77));
    drawPanel(rectf(40.f, 30.f, 1200.f, 80.f), sf::Color(39, 45, 54));
    drawPanel(rectf(40.f, 126.f, 520.f, 560.f), sf::Color(46, 56, 67));
    drawPanel(rectf(580.f, 126.f, 660.f, 560.f), sf::Color(34, 39, 45));

    drawText(tr("Bestiary", "Bestiaire"), {640.f, 52.f}, 30, sf::Color(245, 236, 198), true);
    drawText(tr("ESC to return", "ECHAP pour revenir"), {640.f, 84.f}, 16, sf::Color(205, 225, 240), true);

    if (entries.empty())
    {
        drawText(tr("No entry unlocked yet. Win a battle first.", "Aucune entree debloquee. Gagne d'abord un combat."), {640.f, 320.f}, 24, sf::Color::White, true);
        return;
    }

    for (size_t i = 0; i < entries.size(); ++i)
    {
        const float y = 145.f + static_cast<float>(i) * 48.f;
        const bool selected = i == m_selectedBestiaryIndex;
        drawPanel(rectf(64.f, y, 472.f, 34.f),
                  selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(81, 97, 110),
                  selected ? 5.f : 3.f);
        drawText(entries[i].name, {84.f, y + 6.f}, 17, selected ? sf::Color(30, 24, 15) : sf::Color::White);
        drawText(entries[i].category, {320.f, y + 6.f}, 15, selected ? sf::Color(30, 24, 15) : sf::Color(220, 232, 240));
    }

    const auto& entry = entries[m_selectedBestiaryIndex];
    drawPortrait(rectf(934.f, 150.f, 180.f, 180.f), entry.name, false, getElementColor(entry.elementType));
    drawText(entry.name, {610.f, 150.f}, 30, sf::Color(255, 224, 168));
    drawText(entry.category + " | " + entry.elementType + " | " + entry.land, {610.f, 192.f}, 18, sf::Color(220, 232, 240));
    drawText(entry.physique, {610.f, 224.f}, 17, sf::Color(255, 220, 153));
    drawText(tr("Result: ", "Resultat : ") + entry.result, {610.f, 254.f}, 18, sf::Color(255, 214, 139));
    drawText("HP " + to_string(entry.hp) + "   ATK " + to_string(entry.atk) + "   DEF " + to_string(entry.def), {610.f, 290.f}, 18, sf::Color::White);
    drawWrappedText(entry.description, {610.f, 340.f}, 18, sf::Color(235, 235, 235), 290.f);
    drawText(m_statusText, {610.f, 654.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderItems()
{
    const vector<FrontendInventoryItemViewData> items = m_game.buildFrontendInventoryViewData();
    m_selectedItemIndex = items.empty() ? 0 : min(m_selectedItemIndex, items.size() - 1);
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);

    drawBackgroundTexture("bg_glass_dunes", sf::Color(55, 36, 30), sf::Color(92, 64, 48));
    drawPanel(rectf(40.f, 30.f, 1200.f, 80.f), sf::Color(59, 45, 39));
    drawPanel(rectf(40.f, 126.f, 540.f, 560.f), sf::Color(69, 54, 46));
    drawPanel(rectf(600.f, 126.f, 640.f, 560.f), sf::Color(47, 38, 34));

    drawText(tr("Items", "Objets"), {640.f, 52.f}, 30, sf::Color(245, 236, 198), true);
    drawText(tr("ESC to return", "ECHAP pour revenir"), {640.f, 84.f}, 16, sf::Color(205, 225, 240), true);

    if (items.empty())
    {
        drawText(tr("Inventory is empty.", "L'inventaire est vide."), {640.f, 320.f}, 24, sf::Color::White, true);
        return;
    }

    for (size_t i = 0; i < items.size(); ++i)
    {
        const float y = 145.f + static_cast<float>(i) * 52.f;
        const bool selected = i == m_selectedItemIndex;
        drawPanel(rectf(64.f, y, 492.f, 38.f),
                  selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(94, 74, 61),
                  selected ? 5.f : 3.f);
        drawText(items[i].name, {86.f, y + 8.f}, 17, selected ? sf::Color(30, 24, 15) : sf::Color::White);
        drawText("x" + to_string(items[i].quantity), {500.f, y + 8.f}, 17, selected ? sf::Color(30, 24, 15) : sf::Color(255, 230, 190));
    }

    const auto& item = items[m_selectedItemIndex];
    drawPortrait(rectf(954.f, 150.f, 150.f, 150.f), item.name, false, sf::Color(255, 214, 139));
    drawText(item.name, {624.f, 150.f}, 30, sf::Color(255, 224, 168));
    drawText(item.type + " | Value " + to_string(item.value), {624.f, 192.f}, 18, sf::Color(220, 232, 240));
    drawText("Quantity: " + to_string(item.quantity), {624.f, 226.f}, 18, sf::Color::White);
    drawText(tr("Tactical effect:", "Effet tactique :"), {624.f, 286.f}, 20, sf::Color(255, 214, 139));
    drawText(item.tacticalEffect, {624.f, 320.f}, 18, sf::Color(235, 235, 235));
    drawText(m_statusText, {624.f, 654.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderMonsterSelect()
{
    const vector<FrontendMonsterCardViewData> cards = m_game.buildFrontendMonsterSelectionViewData();
    m_selectedMonsterIndex = cards.empty() ? 0 : min(m_selectedMonsterIndex, cards.size() - 1);
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);

    drawBackgroundTexture("bg_glass_dunes", sf::Color(24, 35, 48), sf::Color(58, 77, 81));
    drawPanel(rectf(40.f, 30.f, 1200.f, 80.f), sf::Color(34, 40, 50));
    drawPanel(rectf(40.f, 126.f, 780.f, 560.f), sf::Color(42, 50, 58));
    drawPanel(rectf(842.f, 126.f, 398.f, 560.f), sf::Color(33, 39, 46));

    drawText(tr("Choose Opponent", "Choisir l'adversaire"), {640.f, 52.f}, 30, sf::Color(245, 236, 198), true);
    drawText(tr("ESC to return", "ECHAP pour revenir"), {640.f, 84.f}, 16, sf::Color(205, 225, 240), true);

    for (size_t i = 0; i < cards.size(); ++i)
    {
        const auto& card = cards[i];
        const float y = 146.f + static_cast<float>(i) * 52.f;
        const bool selected = i == m_selectedMonsterIndex;
        const sf::Color fill = !card.unlocked ? sf::Color(67, 67, 72) : (selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(79, 95, 110));
        drawPanel(rectf(70.f, y, 720.f, 40.f), fill, selected ? 5.f : 3.f);
        drawText(card.unlocked ? card.name : "???", {92.f, y + 8.f}, 17, selected && card.unlocked ? sf::Color(30, 24, 15) : sf::Color::White);
        drawText(card.unlocked ? (card.category + " | " + card.elementType) : tr("LOCKED", "VERROUILLE"), {280.f, y + 8.f}, 15, selected && card.unlocked ? sf::Color(30, 24, 15) : sf::Color(220, 232, 240));
        drawText(card.land, {465.f, y + 8.f}, 15, selected && card.unlocked ? sf::Color(30, 24, 15) : sf::Color(255, 220, 153));
        drawText(card.threat, {650.f, y + 8.f}, 15, selected && card.unlocked ? sf::Color(30, 24, 15) : sf::Color::White);
        drawText(card.unlocked ? ("HP " + to_string(card.hp) + "  ATK " + to_string(card.atk) + "  DEF " + to_string(card.def))
                               : tr("Win more battles to unlock this land", "Gagne plus de combats pour debloquer cette zone"),
                 {92.f, y + 28.f},
                 15,
                 card.unlocked ? (selected ? sf::Color(30, 24, 15) : sf::Color::White) : sf::Color(220, 220, 220));
    }

    if (!cards.empty())
    {
        const auto& focus = cards[m_selectedMonsterIndex];
        const sf::Color focusColor = getElementColor(focus.elementType);
        drawText(focus.unlocked ? focus.name : "???", {1040.f, 150.f}, 28, sf::Color(255, 224, 168), true);
        drawText(focus.unlocked ? (focus.category + " | " + focus.elementType) : tr("Locked encounter", "Rencontre verrouillee"),
                 {1040.f, 188.f}, 16, sf::Color(210, 228, 240), true);
        drawPanel(rectf(890.f, 214.f, 300.f, 190.f),
                  sf::Color(25, 31, 38),
                  4.f);
        drawPanel(rectf(896.f, 220.f, 288.f, 10.f),
                  sf::Color(focusColor.r, focusColor.g, focusColor.b, 210),
                  0.f);
        drawPanel(rectf(906.f, 236.f, 268.f, 152.f), sf::Color(15, 18, 24, 160), 2.f);
        if (focus.unlocked)
        {
            drawCharacterShowcase(toAssetSlug(focus.name),
                                  rectf(916.f, 238.f, 248.f, 144.f),
                                  false,
                                  sf::Color(232, 198, 124),
                                  sf::Color(14, 18, 24, 130));
        }
        else
        {
            drawBattleSilhouette(rectf(920.f, 232.f, 240.f, 150.f), true, "locked_monster", sf::Color(120, 124, 132), sf::Color(14, 18, 24, 130));
        }

        drawText(tr("Region", "Region"), {878.f, 428.f}, 17, sf::Color(255, 214, 139));
        drawText(focus.land, {1000.f, 428.f}, 17, sf::Color::White);
        drawText(tr("Physique", "Physique"), {878.f, 462.f}, 17, sf::Color(255, 214, 139));
        drawText(focus.unlocked ? focus.physique : tr("Unknown", "Inconnu"), {878.f, 488.f}, 15, sf::Color(220, 232, 240));
        drawText(tr("Description", "Description"), {878.f, 532.f}, 17, sf::Color(255, 214, 139));
        drawWrappedText(focus.unlocked ? focus.description : tr("Still hidden in a farther land.", "Encore cache dans une contree lointaine."),
                        {878.f, 560.f}, 15, sf::Color(235, 235, 235), 320.f);
    }

    drawText(m_statusText, {70.f, 654.f}, 15, sf::Color(235, 235, 235));
}

void FrontendApp::renderBattle()
{
    if (!m_game.hasActiveFrontendBattle() && !m_hasCachedBattleView)
    {
        m_currentScreen = Screen::MonsterSelect;
        renderMonsterSelect();
        return;
    }

    const FrontendBattleViewData viewData = m_game.hasActiveFrontendBattle()
        ? m_game.buildActiveFrontendBattleViewData()
        : m_cachedBattleView;
    m_cachedBattleView = viewData;
    m_hasCachedBattleView = true;
    updateBattleAnimationBindings(m_game.getPlayerAppearance(), viewData.monsterName);
    updateBattlePresentation();
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.8f);
    const sf::Color monsterAccent = getElementColor(viewData.monsterElementType);
    const float phaseWave = 0.5f + 0.5f * std::sin(m_battlePresentationClock.getElapsedTime().asSeconds() * 10.f);
    const float playerDash = (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText
                              || m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
        ? (22.f + 12.f * phaseWave)
        : 0.f;
    const float monsterLunge = (m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText
                                || m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
        ? (18.f + 10.f * phaseWave)
        : 0.f;
    const float monsterShake = (m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
        ? ((phaseWave - 0.5f) * 18.f)
        : 0.f;
    const float playerShake = (m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
        ? ((phaseWave - 0.5f) * 14.f)
        : 0.f;
    const sf::FloatRect playerBattlerRect = rectf(120.f + playerDash + playerShake, 358.f, 260.f, 150.f);
    const sf::FloatRect monsterBattlerRect = rectf(738.f - monsterLunge + monsterShake, 166.f, 180.f, 170.f);
    renderBattleEnvironment(viewData.regionName,
                            viewData.monsterElementType,
                            viewData.monsterCategory,
                            toAssetSlug(viewData.monsterName));

    if (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText
        || m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
    {
        sf::Color attackTint = sf::Color(255, 236, 196, 180);
        if (m_pendingPlayerAttackId == "ember")
        {
            attackTint = sf::Color(255, 154, 82, 190);
        }
        else if (m_pendingPlayerAttackId == "pulse")
        {
            attackTint = sf::Color(194, 148, 255, 190);
        }
        else if (m_pendingPlayerAttackId == "tide")
        {
            attackTint = sf::Color(110, 192, 255, 190);
        }
        else if (m_pendingPlayerAttackId == "blade")
        {
            attackTint = sf::Color(255, 244, 204, 190);
        }

        for (int i = 0; i < 4; ++i)
        {
            sf::RectangleShape trail({110.f - i * 18.f, 5.f});
            trail.setFillColor(sf::Color(attackTint.r, attackTint.g, attackTint.b, static_cast<uint8_t>(110 - i * 18)));
            trail.setRotation(sf::degrees(-8.f));
            trail.setPosition({360.f - i * 28.f + playerDash * 0.6f, 442.f - i * 10.f});
            m_window.draw(trail);
        }

        if (!m_pendingPlayerAttackId.empty())
        {
            if (m_pendingPlayerAttackId == "blade")
            {
                sf::RectangleShape slash({120.f, 10.f});
                slash.setFillColor(sf::Color(255, 245, 220, 190));
                slash.setRotation(sf::degrees(-26.f));
                slash.setPosition({476.f + playerDash * 1.2f + phaseWave * 112.f, 332.f - phaseWave * 20.f});
                m_window.draw(slash);
            }
            else if (m_pendingPlayerAttackId == "pulse")
            {
                for (int i = 0; i < 3; ++i)
                {
                    sf::CircleShape orb(8.f + phaseWave * 4.f + i * 3.f);
                    orb.setFillColor(sf::Color(attackTint.r, attackTint.g, attackTint.b, static_cast<uint8_t>(190 - i * 40)));
                    orb.setPosition({430.f + playerDash * 1.1f + phaseWave * (120.f + i * 18.f), 388.f - phaseWave * (70.f - i * 10.f)});
                    m_window.draw(orb);
                }
            }
            else if (m_pendingPlayerAttackId == "ember")
            {
                sf::CircleShape fireball(12.f + phaseWave * 5.f);
                fireball.setFillColor(sf::Color(255, 154, 82, 200));
                fireball.setPosition({438.f + playerDash * 1.1f + phaseWave * 156.f, 394.f - phaseWave * 84.f});
                m_window.draw(fireball);

                sf::CircleShape core(6.f + phaseWave * 3.f);
                core.setFillColor(sf::Color(255, 240, 186, 220));
                core.setPosition({446.f + playerDash * 1.1f + phaseWave * 156.f, 402.f - phaseWave * 84.f});
                m_window.draw(core);
            }
            else if (m_pendingPlayerAttackId == "tide")
            {
                for (int i = 0; i < 3; ++i)
                {
                    sf::CircleShape drop(10.f + i * 3.f + phaseWave * 2.f);
                    drop.setScale({1.3f, 0.8f});
                    drop.setFillColor(sf::Color(110, 192, 255, static_cast<uint8_t>(180 - i * 35)));
                    drop.setPosition({434.f + playerDash * 1.1f + phaseWave * (130.f + i * 14.f), 400.f - phaseWave * (74.f - i * 8.f)});
                    m_window.draw(drop);
                }
            }
            else
            {
                sf::CircleShape projectile(10.f + phaseWave * 6.f);
                projectile.setFillColor(sf::Color(attackTint.r, attackTint.g, attackTint.b, 180));
                projectile.setPosition({430.f + playerDash * 1.2f + phaseWave * 160.f, 388.f - phaseWave * 70.f});
                m_window.draw(projectile);
            }
        }
    }

    if (m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText
        || m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
    {
        const sf::Color attackTint = getElementColor(viewData.monsterElementType);
        for (int i = 0; i < 4; ++i)
        {
            sf::RectangleShape trail({96.f - i * 16.f, 6.f});
            trail.setFillColor(sf::Color(attackTint.r, attackTint.g, attackTint.b, static_cast<uint8_t>(92 - i * 15)));
            trail.setRotation(sf::degrees(188.f + i * 2.f));
            trail.setPosition({740.f + i * 24.f - monsterLunge * 0.5f, 300.f + i * 12.f});
            m_window.draw(trail);
        }

        if (viewData.monsterCategory == "BOSS")
        {
            for (int i = 0; i < 5; ++i)
            {
                sf::CircleShape ring(20.f + i * 16.f + phaseWave * 8.f);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineThickness(2.f);
                ring.setOutlineColor(sf::Color(attackTint.r, attackTint.g, attackTint.b, static_cast<uint8_t>(88 - i * 12)));
                ring.setPosition({746.f - i * 8.f, 176.f - i * 6.f});
                m_window.draw(ring);
            }

            if (viewData.monsterElementType == "Fire")
            {
                for (int i = 0; i < 4; ++i)
                {
                    sf::CircleShape ember(10.f + i * 5.f + phaseWave * 4.f);
                    ember.setFillColor(sf::Color(255, 136, 74, static_cast<uint8_t>(130 - i * 20)));
                    ember.setPosition({684.f - i * 18.f, 204.f + i * 20.f});
                    m_window.draw(ember);
                }
            }
            else if (viewData.monsterElementType == "Water")
            {
                for (int i = 0; i < 4; ++i)
                {
                    sf::CircleShape wave(22.f + i * 12.f);
                    wave.setScale({1.8f, 0.34f});
                    wave.setFillColor(sf::Color(96, 176, 255, static_cast<uint8_t>(60 - i * 10)));
                    wave.setPosition({704.f - i * 10.f, 286.f + i * 8.f});
                    m_window.draw(wave);
                }
            }
            else if (viewData.monsterElementType == "Shadow")
            {
                for (int i = 0; i < 6; ++i)
                {
                    sf::RectangleShape veil({220.f - i * 20.f, 8.f});
                    veil.setRotation(sf::degrees(-18.f + i * 4.f));
                    veil.setFillColor(sf::Color(150, 110, 220, static_cast<uint8_t>(70 - i * 8)));
                    veil.setPosition({680.f + i * 10.f, 192.f + i * 18.f});
                    m_window.draw(veil);
                }
            }
        }
    }

    sf::CircleShape playerPlatform(94.f);
    playerPlatform.setScale({1.7f, 0.34f});
    playerPlatform.setPosition({118.f, 448.f});
    playerPlatform.setFillColor(sf::Color(24, 24, 24, 110));
    m_window.draw(playerPlatform);

    sf::CircleShape monsterPlatform(90.f);
    monsterPlatform.setScale({1.6f, 0.3f});
    monsterPlatform.setPosition({794.f, 318.f});
    monsterPlatform.setFillColor(sf::Color(24, 24, 24, 110));
    m_window.draw(monsterPlatform);

    drawPanel(rectf(36.f, 28.f, 1208.f, 78.f), sf::Color(28, 28, 34));
    drawPanel(rectf(706.f, 118.f, 518.f, 274.f), sf::Color(22, 28, 36, viewData.monsterCategory == "BOSS" ? 235 : 220));
    drawPanel(rectf(714.f, 126.f, 502.f, 12.f), sf::Color(monsterAccent.r, monsterAccent.g, monsterAccent.b, 210), 0.f);
    drawPanel(rectf(724.f, 132.f, 210.f, 240.f), sf::Color(27, 33, 41));
    drawPanel(rectf(734.f, 144.f, 190.f, 216.f), sf::Color(14, 18, 24, 155), 2.f);
    drawPanel(rectf(948.f, 132.f, 258.f, 240.f), sf::Color(35, 43, 55));
    drawPanel(rectf(92.f, 364.f, 430.f, 160.f), sf::Color(46, 55, 61));
    drawPanel(rectf(36.f, 538.f, 1208.f, 96.f), sf::Color(30, 30, 32));
    drawPanel(rectf(36.f, 646.f, 1208.f, 52.f), sf::Color(22, 22, 24));
    drawDecorationRow(114.f, sf::Color(252, 225, 171, 110), 20.f, 3.f);

    drawText(tr("Region: ", "Region : ") + viewData.regionName, {62.f, 50.f}, 18, sf::Color(255, 220, 153));
    drawText(viewData.playerName + " vs " + viewData.monsterName, {62.f, 76.f}, 22, sf::Color::White);
    drawText(viewData.monsterName, {960.f, 148.f}, 28, sf::Color(255, 235, 180));
    drawText(viewData.monsterCategory + " | " + viewData.monsterElementType, {960.f, 184.f}, 17, sf::Color(188, 220, 255));
    drawText(tr("Physique: ", "Physique : ") + viewData.monsterPhysique, {960.f, 214.f}, 15, sf::Color(255, 220, 153));
    drawText(tr("Hint: ", "Indice : ") + viewData.hintText, {960.f, 244.f}, 15, sf::Color::White);
    drawText(tr("Mood: ", "Humeur : ") + viewData.moodText, {960.f, 274.f}, 15, sf::Color::White);
    drawText(tr("Description", "Description"), {960.f, 308.f}, 16, sf::Color(255, 214, 139));
    drawWrappedText(viewData.monsterDescription, {960.f, 334.f}, 14, sf::Color(235, 235, 235), 230.f);
    drawText(viewData.playerName, {116.f, 382.f}, 24, sf::Color(214, 245, 255));
    drawText(viewData.activeBonusesText, {116.f, 416.f}, 16, sf::Color(255, 220, 153));

    if (!drawBattlerAnimation(m_playerBattleAnimation, playerBattlerRect, sf::Color::White, false)
        && !drawContainedTextureWithProfile("sprite_player_" + toAssetSlug(m_game.getPlayerAppearance()),
                                            playerBattlerRect,
                                            getRenderProfile("player_" + toAssetSlug(m_game.getPlayerAppearance())),
                                            sf::Color::White,
                                            true)
        && !drawContainedTextureWithProfile("sprite_player_default",
                                            playerBattlerRect,
                                            getRenderProfile("player_default"),
                                            sf::Color::White,
                                            true))
    {
        drawBattleSilhouette(playerBattlerRect, false, "player_" + m_game.getPlayerAppearance(), sf::Color(128, 205, 226), sf::Color(14, 18, 24, 130));
    }
    if (!drawBattlerAnimation(m_monsterBattleAnimation, monsterBattlerRect, sf::Color::White, false)
        && !drawContainedTextureWithProfile("sprite_" + toAssetSlug(viewData.monsterName),
                                            monsterBattlerRect,
                                            getRenderProfile(toAssetSlug(viewData.monsterName)),
                                            sf::Color::White,
                                            true))
    {
        drawBattleSilhouette(monsterBattlerRect, true, viewData.monsterName, sf::Color(232, 198, 124), sf::Color(14, 18, 24, 130));
    }
    renderBattleFx();
    drawPortrait(rectf(404.f, 374.f, 92.f, 92.f), "player_" + m_game.getPlayerAppearance(), true, sf::Color(124, 201, 222));
    drawPortrait(rectf(768.f, 144.f, 82.f, 82.f), viewData.monsterName, false, getElementColor(viewData.monsterElementType));

    const sf::Color auraColor = getElementColor(viewData.monsterElementType);
    for (int i = 0; i < 10; ++i)
    {
        const float orbit = static_cast<float>(i) * 0.6f + m_animationClock.getElapsedTime().asSeconds() * 2.1f;
        sf::CircleShape spark(3.f + static_cast<float>(i % 3));
        spark.setFillColor(auraColor);
        spark.setPosition({820.f + std::cos(orbit) * (62.f + i * 5.f), 238.f + std::sin(orbit * 1.2f) * (40.f + i * 3.f)});
        m_window.draw(spark);
    }

    FrontendStatBarViewData animatedPlayerHpBar = viewData.playerHpBar;
    FrontendStatBarViewData animatedMonsterHpBar = viewData.monsterHpBar;
    FrontendStatBarViewData animatedMercyBar = viewData.mercyBar;
    animatedPlayerHpBar.currentValue = m_displayedPlayerHp;
    animatedMonsterHpBar.currentValue = m_displayedMonsterHp;
    animatedMercyBar.currentValue = m_displayedMercy;
    drawStatBar(animatedPlayerHpBar, {116.f, 454.f}, {250.f, 20.f}, sf::Color(118, 215, 126));
    drawStatBar(animatedMonsterHpBar, {748.f, 294.f}, {250.f, 20.f}, sf::Color(233, 118, 90));
    drawStatBar(animatedMercyBar, {748.f, 328.f}, {250.f, 20.f}, sf::Color(255, 210, 82));

    drawText(viewData.openingText, {60.f, 556.f}, 18, sf::Color(242, 242, 242));
    string phaseText = viewData.battleLogText;
    if (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText
        || m_battlePresentationPhase == BattlePresentationPhase::MercyChange
        || m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
    {
        phaseText = viewData.playerActionText.empty() ? viewData.battleLogText : viewData.playerActionText;
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText
             || m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
    {
        phaseText = viewData.monsterActionText.empty() ? viewData.battleLogText : viewData.monsterActionText;
    }
    drawText(phaseText, {60.f, 586.f}, 16, sf::Color(255, 220, 153));
    if (m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
    {
        drawText(tr("Your strike lands...", "Ton attaque touche..."), {60.f, 612.f}, 15, sf::Color(210, 228, 240));
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MercyChange)
    {
        drawText(tr("Mercy shifts...", "La mercy evolue..."), {60.f, 612.f}, 15, sf::Color(210, 228, 240));
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText)
    {
        drawText(tr("You act first.", "Tu agis en premier."), {60.f, 612.f}, 15, sf::Color(210, 228, 240));
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText)
    {
        drawText(tr("The foe prepares an answer...", "L'ennemi prepare sa reponse..."), {60.f, 612.f}, 15, sf::Color(210, 228, 240));
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
    {
        drawText(tr("The enemy answers back...", "L'ennemi riposte..."), {60.f, 612.f}, 15, sf::Color(210, 228, 240));
    }
    drawText(tr("Mouse or Left/Right + Enter", "Souris ou Gauche/Droite + Entree"), {1040.f, 556.f}, 15, sf::Color(215, 230, 240), true);

    if (m_hasPendingScreenChange && m_battlePresentationPhase == BattlePresentationPhase::Idle)
    {
        const bool victoryState = m_pendingScreenStatus.find("defeated") != string::npos
            || m_pendingScreenStatus.find("spare") != string::npos
            || m_pendingScreenStatus.find("spared") != string::npos;
        const bool defeatState = m_pendingScreenStatus.find("overwhelmed") != string::npos;
        const sf::Color bannerColor = defeatState ? sf::Color(164, 72, 72) : (victoryState ? sf::Color(86, 142, 98) : sf::Color(120, 112, 86));
        const string bannerTitle = defeatState
            ? tr("BATTLE LOST", "COMBAT PERDU")
            : (victoryState ? tr("BATTLE WON", "COMBAT GAGNE") : tr("BATTLE ENDED", "COMBAT TERMINE"));
        sf::RectangleShape veil({1280.f, 720.f});
        veil.setFillColor(sf::Color(10, 10, 14, 120));
        m_window.draw(veil);

        drawPanel(rectf(330.f, 208.f, 620.f, 220.f), sf::Color(28, 30, 34), 5.f);
        drawPanel(rectf(344.f, 222.f, 592.f, 22.f), bannerColor, 0.f);
        drawText(bannerTitle, {640.f, 268.f}, 36, sf::Color(255, 242, 210), true);
        drawWrappedText(m_pendingScreenStatus, {388.f, 318.f}, 18, sf::Color(244, 236, 216), 500.f, 1.3f);
        drawPanel(rectf(430.f, 378.f, 420.f, 34.f), sf::Color(46, 56, 66), 3.f);
        drawText(tr("Press Enter, Escape or click to continue", "Entree, Echap ou clic pour continuer"),
                 {640.f, 384.f},
                 16,
                 sf::Color(255, 236, 196),
                 true);
    }

    for (size_t i = 0; i < viewData.primaryActions.size(); ++i)
    {
        const float x = 56.f + static_cast<float>(i) * 300.f;
        const bool selected = i == m_selectedBattleActionIndex;
        const bool enabled = viewData.primaryActions[i].enabled;
        const sf::Color activeColor(242, static_cast<uint8_t>(178 + 22 * pulse), 94);
        const bool lockedByEndState = m_hasPendingScreenChange;
        drawPanel(rectf(x, 652.f, 280.f, 38.f),
                  (!enabled || lockedByEndState) ? sf::Color(78, 78, 82) : (selected ? activeColor : sf::Color(70, 82, 92)),
                  selected && !lockedByEndState ? 5.f : 3.f);
        drawText(localizeBattleActionLabel(viewData.primaryActions[i]),
                 {x + 140.f, 658.f},
                 18,
                 (!enabled || lockedByEndState) ? sf::Color(180, 180, 180) : (selected ? sf::Color(30, 24, 15) : sf::Color::White),
                 true);
    }

    const float flashElapsed = m_battleFlashClock.getElapsedTime().asSeconds();
    if (flashElapsed < 0.18f)
    {
        const float flashT = 1.f - (flashElapsed / 0.18f);
        sf::RectangleShape flash({1280.f, 720.f});
        flash.setFillColor(sf::Color(m_battleFlashColor.r,
                                     m_battleFlashColor.g,
                                     m_battleFlashColor.b,
                                     static_cast<uint8_t>(70.f * std::clamp(flashT, 0.f, 1.f))));
        m_window.draw(flash);
    }

    if (m_battleOverlay == BattleOverlay::ActList)
    {
        renderOverlay(m_game.buildFrontendActOptionViewData(), tr("Choose ACT", "Choisir ACT"));
    }
    else if (m_battleOverlay == BattleOverlay::ItemList)
    {
        renderOverlay(m_game.buildFrontendItemOptionViewData(), tr("Choose Item", "Choisir un objet"));
    }
    else if (m_battleOverlay == BattleOverlay::FightList)
    {
        renderOverlay(m_game.buildFrontendFightOptionViewData(), tr("Choose Attack", "Choisir une attaque"));
    }
}

void FrontendApp::renderOverlay(const vector<FrontendActionButtonViewData>& options, const string& title)
{
    const float pulse = 0.5f + 0.5f * std::sin(m_animationClock.getElapsedTime().asSeconds() * 4.f);
    drawPanel(rectf(250.f, 100.f, 780.f, 440.f), sf::Color(24, 26, 31));
    drawText(title, {640.f, 122.f}, 28, sf::Color(255, 220, 153), true);
    drawText(tr("Arrows + Enter or mouse click. ESC closes the panel.", "Fleches + Entree ou clic souris. ECHAP ferme le panneau."), {640.f, 160.f}, 16, sf::Color(210, 228, 240), true);

    if (options.empty())
    {
        drawText(tr("No option available.", "Aucune option disponible."), {640.f, 300.f}, 22, sf::Color::White, true);
        return;
    }

    m_selectedOverlayIndex = min(m_selectedOverlayIndex, options.size() - 1);
    for (size_t i = 0; i < options.size(); ++i)
    {
        const float y = 200.f + static_cast<float>(i) * 48.f;
        const bool selected = i == m_selectedOverlayIndex;
        const bool enabled = options[i].enabled;
        drawPanel(rectf(300.f, y, 680.f, 34.f),
                  !enabled ? sf::Color(76, 76, 80) : (selected ? sf::Color(242, static_cast<uint8_t>(178 + 22 * pulse), 94) : sf::Color(78, 92, 106)),
                  selected ? 5.f : 3.f);
        drawText(options[i].label, {640.f, y + 6.f}, 17, !enabled ? sf::Color(180, 180, 180) : (selected ? sf::Color(30, 24, 15) : sf::Color::White), true);
    }
}

void FrontendApp::handleLanguageSelectEvent(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedLanguageIndex = (m_selectedLanguageIndex == 0) ? 1 : 0;
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedLanguageIndex = (m_selectedLanguageIndex + 1) % 2;
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            confirmSelectedLanguage();
        }
    }
}

void FrontendApp::handleMenuEvent(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedMenuIndex = (m_selectedMenuIndex == 0) ? 4 : (m_selectedMenuIndex - 1);
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedMenuIndex = (m_selectedMenuIndex + 1) % 5;
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            activateMenuChoice();
        }
    }
}

void FrontendApp::handleHeroSelectEvent(const sf::Event& event)
{
    const vector<string> appearanceIds = Game::getAvailableHeroAppearanceIds();
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedHeroAppearanceIndex = (m_selectedHeroAppearanceIndex == 0) ? appearanceIds.size() - 1 : (m_selectedHeroAppearanceIndex - 1);
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedHeroAppearanceIndex = (m_selectedHeroAppearanceIndex + 1) % appearanceIds.size();
            playSoundIfAvailable("ui_move", 45.f);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            m_game.setPlayerAppearance(appearanceIds[m_selectedHeroAppearanceIndex]);
            changeScreen(Screen::Menu, "Hero style set to " + Game::getHeroAppearanceLabel(appearanceIds[m_selectedHeroAppearanceIndex]) + ".");
        }
    }
}

void FrontendApp::handleBestiaryEvent(const sf::Event& event)
{
    const vector<FrontendBestiaryEntryViewData> entries = m_game.buildFrontendBestiaryViewData();
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape)
        {
            changeScreen(Screen::Menu, "Returned to menu.");
        }
        else if (!entries.empty() && keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedBestiaryIndex = (m_selectedBestiaryIndex == 0) ? entries.size() - 1 : (m_selectedBestiaryIndex - 1);
        }
        else if (!entries.empty() && keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedBestiaryIndex = (m_selectedBestiaryIndex + 1) % entries.size();
        }
    }
}

void FrontendApp::handlePlayerStatsEvent(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            changeScreen(Screen::Menu, "Returned to menu.");
        }
    }
}

void FrontendApp::handleItemsEvent(const sf::Event& event)
{
    const vector<FrontendInventoryItemViewData> items = m_game.buildFrontendInventoryViewData();
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape)
        {
            changeScreen(Screen::Menu, "Returned to menu.");
        }
        else if (!items.empty() && keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedItemIndex = (m_selectedItemIndex == 0) ? items.size() - 1 : (m_selectedItemIndex - 1);
        }
        else if (!items.empty() && keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedItemIndex = (m_selectedItemIndex + 1) % items.size();
        }
    }
}

void FrontendApp::handleMonsterSelectEvent(const sf::Event& event)
{
    const vector<FrontendMonsterCardViewData> cards = m_game.buildFrontendMonsterSelectionViewData();
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyPressed->code == sf::Keyboard::Key::Escape)
        {
            changeScreen(Screen::Menu, "Returned to menu.");
        }
        else if (!cards.empty() && keyPressed->code == sf::Keyboard::Key::Up)
        {
            m_selectedMonsterIndex = (m_selectedMonsterIndex == 0) ? cards.size() - 1 : (m_selectedMonsterIndex - 1);
        }
        else if (!cards.empty() && keyPressed->code == sf::Keyboard::Key::Down)
        {
            m_selectedMonsterIndex = (m_selectedMonsterIndex + 1) % cards.size();
        }
        else if (!cards.empty() && (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space))
        {
            startSelectedMonsterBattle();
        }
    }
}

void FrontendApp::handleBattleEvent(const sf::Event& event)
{
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_hasPendingScreenChange && m_battlePresentationPhase == BattlePresentationPhase::Idle)
        {
            if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space || keyPressed->code == sf::Keyboard::Key::Escape)
            {
                if (m_game.hasActiveFrontendBattle())
                {
                    m_game.leaveFrontendBattle();
                }
                changeScreen(m_pendingScreen, m_pendingScreenStatus);
            }
            return;
        }

        if (m_battlePresentationPhase != BattlePresentationPhase::Idle
            && keyPressed->code != sf::Keyboard::Key::Escape)
        {
            return;
        }

        if (keyPressed->code == sf::Keyboard::Key::Escape)
        {
            if (m_battleOverlay != BattleOverlay::None)
            {
                m_battleOverlay = BattleOverlay::None;
                m_selectedOverlayIndex = 0;
            }
            else
            {
                m_game.leaveFrontendBattle();
                changeScreen(Screen::MonsterSelect, "Returned to monster selection.");
            }
            return;
        }

        if (m_battleOverlay != BattleOverlay::None)
        {
            vector<FrontendActionButtonViewData> options;
            if (m_battleOverlay == BattleOverlay::FightList)
            {
                options = m_game.buildFrontendFightOptionViewData();
            }
            else if (m_battleOverlay == BattleOverlay::ActList)
            {
                options = m_game.buildFrontendActOptionViewData();
            }
            else
            {
                options = m_game.buildFrontendItemOptionViewData();
            }
            if (options.empty())
            {
                m_battleOverlay = BattleOverlay::None;
                return;
            }

            if (keyPressed->code == sf::Keyboard::Key::Up)
            {
                m_selectedOverlayIndex = (m_selectedOverlayIndex == 0) ? options.size() - 1 : (m_selectedOverlayIndex - 1);
            }
            else if (keyPressed->code == sf::Keyboard::Key::Down)
            {
                m_selectedOverlayIndex = (m_selectedOverlayIndex + 1) % options.size();
            }
            else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
            {
                if (!options[m_selectedOverlayIndex].enabled)
                {
                    m_statusText = "This option is unavailable.";
                    return;
                }

                if (m_battleOverlay == BattleOverlay::FightList)
                {
                    const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                    const string selectedAttackId = options[m_selectedOverlayIndex].id;
                    m_pendingPlayerAttackId = selectedAttackId;
                    m_statusText = m_game.performFrontendFightByIndex(m_selectedOverlayIndex);
                    triggerBattleFlash(sf::Color(255, 132, 92));
                    queueBattlerAnimation(m_playerBattleAnimation, m_playerBattleAnimation.actionKey, 0.42f);
                    queueBattlerAnimation(m_monsterBattleAnimation, m_monsterBattleAnimation.hurtKey, 0.30f);
                    queueBattleFx(getFxClipForAttackId(selectedAttackId), rectf(770.f, 180.f, 220.f, 150.f), 0.44f);
                    if (m_game.hasActiveFrontendBattle())
                    {
                        beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                    }
                    else
                    {
                        FrontendBattleViewData afterView = beforeView;
                        afterView.playerActionText = m_statusText;
                        afterView.battleLogText = m_statusText;
                        afterView.monsterActionText.clear();
                        if (m_statusText.find("defeated") != string::npos)
                        {
                            afterView.monsterHpBar.currentValue = 0;
                        }
                        beginBattlePresentation(beforeView, afterView);
                        m_hasPendingScreenChange = true;
                        m_pendingScreen = Screen::MonsterSelect;
                        m_pendingScreenStatus = m_statusText;
                        playSoundIfAvailable("battle_victory", 72.f);
                    }
                }
                else if (m_battleOverlay == BattleOverlay::ActList)
                {
                    const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                    m_pendingPlayerAttackId = "act";
                    m_statusText = m_game.performFrontendActByIndex(m_selectedOverlayIndex);
                    triggerBattleFlash(sf::Color(118, 203, 255));
                    queueBattlerAnimation(m_playerBattleAnimation, m_playerBattleAnimation.actionKey, 0.28f);
                    if (m_game.hasActiveFrontendBattle())
                    {
                        beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                    }
                    else
                    {
                        FrontendBattleViewData afterView = beforeView;
                        afterView.playerActionText = m_statusText;
                        afterView.battleLogText = m_statusText;
                        afterView.monsterActionText.clear();
                        if (m_statusText.find("spare") != string::npos || m_statusText.find("spared") != string::npos)
                        {
                            afterView.mercyBar.currentValue = afterView.mercyBar.maxValue;
                        }
                        if (m_statusText.find("overwhelmed") != string::npos)
                        {
                            afterView.playerHpBar.currentValue = 0;
                        }
                        beginBattlePresentation(beforeView, afterView);
                        m_hasPendingScreenChange = true;
                        m_pendingScreen = Screen::MonsterSelect;
                        m_pendingScreenStatus = m_statusText;
                        playSoundIfAvailable(m_statusText.find("overwhelmed") != string::npos ? "battle_defeat" : "battle_victory", 72.f);
                    }
                }
                else
                {
                    const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                    m_pendingPlayerAttackId = "item";
                    m_statusText = m_game.performFrontendItemByIndex(m_selectedOverlayIndex);
                    triggerBattleFlash(sf::Color(128, 232, 144));
                    playSoundIfAvailable("battle_heal", 68.f);
                    if (m_game.hasActiveFrontendBattle())
                    {
                        beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                    }
                    else
                    {
                        FrontendBattleViewData afterView = beforeView;
                        afterView.playerActionText = m_statusText;
                        afterView.battleLogText = m_statusText;
                        afterView.monsterActionText.clear();
                        if (m_statusText.find("overwhelmed") != string::npos)
                        {
                            afterView.playerHpBar.currentValue = 0;
                        }
                        beginBattlePresentation(beforeView, afterView);
                        m_hasPendingScreenChange = true;
                        m_pendingScreen = Screen::MonsterSelect;
                        m_pendingScreenStatus = m_statusText;
                        playSoundIfAvailable("battle_defeat", 72.f);
                    }
                }
                m_battleOverlay = BattleOverlay::None;
            }
            return;
        }

        if (keyPressed->code == sf::Keyboard::Key::Left)
        {
            m_selectedBattleActionIndex = (m_selectedBattleActionIndex == 0) ? 3 : (m_selectedBattleActionIndex - 1);
        }
        else if (keyPressed->code == sf::Keyboard::Key::Right)
        {
            m_selectedBattleActionIndex = (m_selectedBattleActionIndex + 1) % 4;
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space)
        {
            const FrontendBattleViewData viewData = m_game.buildActiveFrontendBattleViewData();
            const string& actionId = viewData.primaryActions[m_selectedBattleActionIndex].id;
            if (!viewData.primaryActions[m_selectedBattleActionIndex].enabled)
            {
                m_statusText = "This action is unavailable.";
            }
            else if (actionId == "fight")
            {
                m_battleOverlay = BattleOverlay::FightList;
                m_selectedOverlayIndex = 0;
            }
            else if (actionId == "act")
            {
                m_battleOverlay = BattleOverlay::ActList;
                m_selectedOverlayIndex = 0;
            }
            else if (actionId == "item")
            {
                m_battleOverlay = BattleOverlay::ItemList;
                m_selectedOverlayIndex = 0;
            }
            else
            {
                const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                m_pendingPlayerAttackId = actionId;
                m_statusText = m_game.performFrontendBattleAction(actionId);
                if (actionId == "fight")
                {
                    triggerBattleFlash(sf::Color(255, 132, 92));
                }
                else if (actionId == "mercy")
                {
                    triggerBattleFlash(sf::Color(255, 226, 110));
                }
                if (m_game.hasActiveFrontendBattle())
                {
                    beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                }
                else
                {
                    FrontendBattleViewData afterView = beforeView;
                    afterView.playerActionText = m_statusText;
                    afterView.battleLogText = m_statusText;
                    afterView.monsterActionText.clear();
                    if (m_statusText.find("spare") != string::npos || m_statusText.find("spared") != string::npos)
                    {
                        afterView.mercyBar.currentValue = afterView.mercyBar.maxValue;
                    }
                    if (m_statusText.find("overwhelmed") != string::npos)
                    {
                        afterView.playerHpBar.currentValue = 0;
                    }
                    beginBattlePresentation(beforeView, afterView);
                    m_hasPendingScreenChange = true;
                    m_pendingScreen = Screen::MonsterSelect;
                    m_pendingScreenStatus = m_statusText;
                    playSoundIfAvailable(m_statusText.find("overwhelmed") != string::npos ? "battle_defeat" : "battle_victory", 72.f);
                }
            }
        }
    }
}

void FrontendApp::handleMenuClick(const sf::Vector2f& point)
{
    for (size_t i = 0; i < 5; ++i)
    {
        const float y = 320.f + static_cast<float>(i) * 54.f;
        if (contains(rectf(90.f, y, 320.f, 40.f), point))
        {
            m_selectedMenuIndex = i;
            activateMenuChoice();
            return;
        }
    }
}

void FrontendApp::handleLanguageSelectClick(const sf::Vector2f& point)
{
    for (size_t i = 0; i < 2; ++i)
    {
        const float x = 220.f + static_cast<float>(i) * 430.f;
        if (contains(rectf(x, 310.f, 310.f, 180.f), point))
        {
            m_selectedLanguageIndex = i;
            confirmSelectedLanguage();
            return;
        }
    }
}

void FrontendApp::handleHeroSelectClick(const sf::Vector2f& point)
{
    const vector<string> appearanceIds = Game::getAvailableHeroAppearanceIds();
    for (size_t i = 0; i < appearanceIds.size(); ++i)
    {
        const float x = 118.f + static_cast<float>(i) * 350.f;
        if (contains(rectf(x, 218.f, 250.f, 320.f), point))
        {
            m_selectedHeroAppearanceIndex = i;
            m_game.setPlayerAppearance(appearanceIds[i]);
            changeScreen(Screen::Menu, "Hero style set to " + Game::getHeroAppearanceLabel(appearanceIds[i]) + ".");
            return;
        }
    }
}

void FrontendApp::handleBestiaryClick(const sf::Vector2f& point)
{
    const vector<FrontendBestiaryEntryViewData> entries = m_game.buildFrontendBestiaryViewData();
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const float y = 145.f + static_cast<float>(i) * 48.f;
        if (contains(rectf(64.f, y, 472.f, 34.f), point))
        {
            m_selectedBestiaryIndex = i;
            return;
        }
    }
}

void FrontendApp::handlePlayerStatsClick(const sf::Vector2f& point)
{
    (void)point;
}

void FrontendApp::handleItemsClick(const sf::Vector2f& point)
{
    const vector<FrontendInventoryItemViewData> items = m_game.buildFrontendInventoryViewData();
    for (size_t i = 0; i < items.size(); ++i)
    {
        const float y = 145.f + static_cast<float>(i) * 52.f;
        if (contains(rectf(64.f, y, 492.f, 38.f), point))
        {
            m_selectedItemIndex = i;
            return;
        }
    }
}

void FrontendApp::handleMonsterSelectClick(const sf::Vector2f& point)
{
    const vector<FrontendMonsterCardViewData> cards = m_game.buildFrontendMonsterSelectionViewData();
    for (size_t i = 0; i < cards.size(); ++i)
    {
        const float y = 146.f + static_cast<float>(i) * 52.f;
        if (contains(rectf(70.f, y, 1140.f, 40.f), point))
        {
            m_selectedMonsterIndex = i;
            startSelectedMonsterBattle();
            return;
        }
    }
}

void FrontendApp::handleBattleClick(const sf::Vector2f& point)
{
    if (m_hasPendingScreenChange && m_battlePresentationPhase == BattlePresentationPhase::Idle)
    {
        if (contains(rectf(330.f, 208.f, 620.f, 220.f), point))
        {
            if (m_game.hasActiveFrontendBattle())
            {
                m_game.leaveFrontendBattle();
            }
            changeScreen(m_pendingScreen, m_pendingScreenStatus);
        }
        return;
    }

    if (m_battlePresentationPhase != BattlePresentationPhase::Idle)
    {
        return;
    }

    if (m_battleOverlay != BattleOverlay::None)
    {
        vector<FrontendActionButtonViewData> options;
        if (m_battleOverlay == BattleOverlay::FightList)
        {
            options = m_game.buildFrontendFightOptionViewData();
        }
        else if (m_battleOverlay == BattleOverlay::ActList)
        {
            options = m_game.buildFrontendActOptionViewData();
        }
        else
        {
            options = m_game.buildFrontendItemOptionViewData();
        }
        for (size_t i = 0; i < options.size(); ++i)
        {
            const float y = 200.f + static_cast<float>(i) * 48.f;
            if (!contains(rectf(300.f, y, 680.f, 34.f), point))
            {
                continue;
            }

            m_selectedOverlayIndex = i;
            if (!options[i].enabled)
            {
                m_statusText = "This option is unavailable.";
                return;
            }

            if (m_battleOverlay == BattleOverlay::FightList)
            {
                const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                const string selectedAttackId = options[i].id;
                m_pendingPlayerAttackId = selectedAttackId;
                m_statusText = m_game.performFrontendFightByIndex(i);
                triggerBattleFlash(sf::Color(255, 132, 92));
                queueBattlerAnimation(m_playerBattleAnimation, m_playerBattleAnimation.actionKey, 0.42f);
                queueBattlerAnimation(m_monsterBattleAnimation, m_monsterBattleAnimation.hurtKey, 0.30f);
                queueBattleFx(getFxClipForAttackId(selectedAttackId), rectf(770.f, 180.f, 220.f, 150.f), 0.44f);
                if (m_game.hasActiveFrontendBattle())
                {
                    beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                }
                else
                {
                    FrontendBattleViewData afterView = beforeView;
                    afterView.playerActionText = m_statusText;
                    afterView.battleLogText = m_statusText;
                    afterView.monsterActionText.clear();
                    if (m_statusText.find("defeated") != string::npos)
                    {
                        afterView.monsterHpBar.currentValue = 0;
                    }
                    beginBattlePresentation(beforeView, afterView);
                    m_hasPendingScreenChange = true;
                    m_pendingScreen = Screen::MonsterSelect;
                    m_pendingScreenStatus = m_statusText;
                    playSoundIfAvailable("battle_victory", 72.f);
                }
            }
            else if (m_battleOverlay == BattleOverlay::ActList)
            {
                const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                m_pendingPlayerAttackId = "act";
                m_statusText = m_game.performFrontendActByIndex(i);
                triggerBattleFlash(sf::Color(118, 203, 255));
                queueBattlerAnimation(m_playerBattleAnimation, m_playerBattleAnimation.actionKey, 0.28f);
                if (m_game.hasActiveFrontendBattle())
                {
                    beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                }
                else
                {
                    FrontendBattleViewData afterView = beforeView;
                    afterView.playerActionText = m_statusText;
                    afterView.battleLogText = m_statusText;
                    afterView.monsterActionText.clear();
                    if (m_statusText.find("spare") != string::npos || m_statusText.find("spared") != string::npos)
                    {
                        afterView.mercyBar.currentValue = afterView.mercyBar.maxValue;
                    }
                    if (m_statusText.find("overwhelmed") != string::npos)
                    {
                        afterView.playerHpBar.currentValue = 0;
                    }
                    beginBattlePresentation(beforeView, afterView);
                    m_hasPendingScreenChange = true;
                    m_pendingScreen = Screen::MonsterSelect;
                    m_pendingScreenStatus = m_statusText;
                    playSoundIfAvailable(m_statusText.find("overwhelmed") != string::npos ? "battle_defeat" : "battle_victory", 72.f);
                }
            }
            else
            {
                const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
                m_pendingPlayerAttackId = "item";
                m_statusText = m_game.performFrontendItemByIndex(i);
                triggerBattleFlash(sf::Color(128, 232, 144));
                playSoundIfAvailable("battle_heal", 68.f);
                if (m_game.hasActiveFrontendBattle())
                {
                    beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
                }
                else
                {
                    FrontendBattleViewData afterView = beforeView;
                    afterView.playerActionText = m_statusText;
                    afterView.battleLogText = m_statusText;
                    afterView.monsterActionText.clear();
                    if (m_statusText.find("overwhelmed") != string::npos)
                    {
                        afterView.playerHpBar.currentValue = 0;
                    }
                    beginBattlePresentation(beforeView, afterView);
                    m_hasPendingScreenChange = true;
                    m_pendingScreen = Screen::MonsterSelect;
                    m_pendingScreenStatus = m_statusText;
                    playSoundIfAvailable("battle_defeat", 72.f);
                }
            }
            m_battleOverlay = BattleOverlay::None;
            return;
        }
        return;
    }

    const FrontendBattleViewData viewData = m_game.buildActiveFrontendBattleViewData();
    for (size_t i = 0; i < viewData.primaryActions.size(); ++i)
    {
        const float x = 56.f + static_cast<float>(i) * 300.f;
        if (!contains(rectf(x, 652.f, 280.f, 38.f), point))
        {
            continue;
        }

        m_selectedBattleActionIndex = i;
        const string& actionId = viewData.primaryActions[i].id;
        if (!viewData.primaryActions[i].enabled)
        {
            m_statusText = "This action is unavailable.";
        }
        else if (actionId == "fight")
        {
            m_battleOverlay = BattleOverlay::FightList;
            m_selectedOverlayIndex = 0;
        }
        else if (actionId == "act")
        {
            m_battleOverlay = BattleOverlay::ActList;
            m_selectedOverlayIndex = 0;
        }
        else if (actionId == "item")
        {
            m_battleOverlay = BattleOverlay::ItemList;
            m_selectedOverlayIndex = 0;
        }
        else
        {
            const FrontendBattleViewData beforeView = m_game.buildActiveFrontendBattleViewData();
            m_pendingPlayerAttackId = actionId;
            m_statusText = m_game.performFrontendBattleAction(actionId);
            if (actionId == "fight")
            {
                triggerBattleFlash(sf::Color(255, 132, 92));
            }
            else if (actionId == "mercy")
            {
                triggerBattleFlash(sf::Color(255, 226, 110));
            }
            if (m_game.hasActiveFrontendBattle())
            {
                beginBattlePresentation(beforeView, m_game.buildActiveFrontendBattleViewData());
            }
            else
            {
                FrontendBattleViewData afterView = beforeView;
                afterView.playerActionText = m_statusText;
                afterView.battleLogText = m_statusText;
                afterView.monsterActionText.clear();
                if (m_statusText.find("spare") != string::npos || m_statusText.find("spared") != string::npos)
                {
                    afterView.mercyBar.currentValue = afterView.mercyBar.maxValue;
                }
                if (m_statusText.find("overwhelmed") != string::npos)
                {
                    afterView.playerHpBar.currentValue = 0;
                }
                beginBattlePresentation(beforeView, afterView);
                m_hasPendingScreenChange = true;
                m_pendingScreen = Screen::MonsterSelect;
                m_pendingScreenStatus = m_statusText;
                playSoundIfAvailable(m_statusText.find("overwhelmed") != string::npos ? "battle_defeat" : "battle_victory", 72.f);
            }
        }
        return;
    }
}

void FrontendApp::activateMenuChoice()
{
    switch (m_selectedMenuIndex)
    {
    case 0:
        changeScreen(Screen::Bestiary, "Bestiary opened.");
        break;
    case 1:
        changeScreen(Screen::MonsterSelect, "Choose a monster to start a battle.");
        break;
    case 2:
        changeScreen(Screen::PlayerStats, "Player stats opened.");
        break;
    case 3:
        changeScreen(Screen::Items, "Items screen opened.");
        break;
    case 4:
        m_window.close();
        break;
    default:
        break;
    }
}

void FrontendApp::confirmSelectedLanguage()
{
    m_languageCode = (m_selectedLanguageIndex == 0) ? "en" : "fr";
    changeScreen(Screen::HeroSelect, tr("Language selected.", "Langue selectionnee."));
}

void FrontendApp::startSelectedMonsterBattle()
{
    const vector<FrontendMonsterCardViewData> cards = m_game.buildFrontendMonsterSelectionViewData();
    if (cards.empty())
    {
        return;
    }

    if (!cards[m_selectedMonsterIndex].unlocked)
    {
        m_statusText = "This foe is still hidden in a farther land.";
        return;
    }

    size_t unlockedDisplayIndex = 0;
    for (size_t i = 0; i < cards.size(); ++i)
    {
        if (!cards[i].unlocked)
        {
            continue;
        }

        if (i == m_selectedMonsterIndex)
        {
            if (m_game.startFrontendBattle(unlockedDisplayIndex))
            {
                changeScreen(Screen::Battle);
                m_battleOverlay = BattleOverlay::None;
                m_selectedBattleActionIndex = 0;
                m_selectedOverlayIndex = 0;
                primeBattlePresentation(m_game.buildActiveFrontendBattleViewData());
                m_statusText = "Battle started against " + cards[i].name + ".";
                triggerBattleFlash(sf::Color(255, 236, 171));
            }
            return;
        }

        ++unlockedDisplayIndex;
    }
}

void FrontendApp::changeScreen(Screen newScreen, const string& statusText)
{
    m_currentScreen = newScreen;
    m_screenTransitionClock.restart();
    m_playerBattleAnimation.overrideKey.clear();
    m_playerBattleAnimation.overrideDurationSeconds = 0.f;
    m_monsterBattleAnimation.overrideKey.clear();
    m_monsterBattleAnimation.overrideDurationSeconds = 0.f;
    m_battleFxState.clipKey.clear();
    m_battleFxState.durationSeconds = 0.f;
    m_battlePresentationPhase = BattlePresentationPhase::Idle;
    m_hasPendingScreenChange = false;
    m_pendingPlayerAttackId.clear();
    if (newScreen != Screen::Battle)
    {
        m_hasCachedBattleView = false;
    }
    playSoundIfAvailable("ui_confirm", 55.f);
    if (!statusText.empty())
    {
        m_statusText = statusText;
    }
}

void FrontendApp::triggerBattleFlash(const sf::Color& color)
{
    m_battleFlashColor = color;
    m_battleFlashClock.restart();

    if (color == sf::Color(255, 132, 92))
    {
        playSoundIfAvailable("battle_hit", 70.f);
    }
    else if (color == sf::Color(118, 203, 255))
    {
        playSoundIfAvailable("battle_act", 68.f);
    }
    else if (color == sf::Color(128, 232, 144))
    {
        playSoundIfAvailable("battle_item", 68.f);
    }
    else if (color == sf::Color(255, 226, 110))
    {
        playSoundIfAvailable("battle_mercy", 72.f);
    }
}

void FrontendApp::queueBattlerAnimation(BattleAnimationState& state, const string& clipKey, float durationSeconds)
{
    if (clipKey.empty() || m_animationClips.find(clipKey) == m_animationClips.end())
    {
        return;
    }

    state.overrideKey = clipKey;
    state.overrideDurationSeconds = durationSeconds;
    state.overrideClock.restart();
}

void FrontendApp::queueBattleFx(const string& clipKey, const sf::FloatRect& area, float durationSeconds)
{
    if (clipKey.empty() || m_animationClips.find(clipKey) == m_animationClips.end())
    {
        return;
    }

    m_battleFxState.clipKey = clipKey;
    m_battleFxState.area = area;
    m_battleFxState.durationSeconds = durationSeconds;
    m_battleFxState.clock.restart();
}

void FrontendApp::renderBattleFx()
{
    if (m_battleFxState.clipKey.empty())
    {
        return;
    }

    const float elapsed = m_battleFxState.clock.getElapsedTime().asSeconds();
    if (elapsed >= m_battleFxState.durationSeconds)
    {
        m_battleFxState.clipKey.clear();
        return;
    }

    const float t = 1.f - (elapsed / std::max(0.01f, m_battleFxState.durationSeconds));
    sf::CircleShape burst(std::max(m_battleFxState.area.size.x, m_battleFxState.area.size.y) * (0.18f + 0.12f * t));
    burst.setScale({1.9f, 1.15f});
    burst.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(56.f * std::clamp(t, 0.f, 1.f))));
    burst.setPosition({m_battleFxState.area.position.x + m_battleFxState.area.size.x * 0.5f - burst.getRadius(),
                       m_battleFxState.area.position.y + m_battleFxState.area.size.y * 0.5f - burst.getRadius() * 0.8f});
    m_window.draw(burst);

    drawAnimationClipIfAvailable(m_battleFxState.clipKey,
                                 m_battleFxState.area,
                                 sf::Color(255, 255, 255, 220),
                                 elapsed,
                                 false);
}

void FrontendApp::primeBattlePresentation(const FrontendBattleViewData& viewData)
{
    m_displayedPlayerHp = viewData.playerHpBar.currentValue;
    m_displayedMonsterHp = viewData.monsterHpBar.currentValue;
    m_displayedMercy = viewData.mercyBar.currentValue;
    m_startPlayerHp = m_displayedPlayerHp;
    m_startMonsterHp = m_displayedMonsterHp;
    m_startMercy = m_displayedMercy;
    m_targetPlayerHp = m_displayedPlayerHp;
    m_targetMonsterHp = m_displayedMonsterHp;
    m_targetMercy = m_displayedMercy;
    m_battlePresentationPhase = BattlePresentationPhase::Idle;
    m_hasPendingScreenChange = false;
    m_hasCachedBattleView = true;
    m_cachedBattleView = viewData;
    m_pendingPlayerAttackId.clear();
    m_pendingMonsterFxClip.clear();
}

void FrontendApp::beginBattlePresentation(const FrontendBattleViewData& beforeView, const FrontendBattleViewData& afterView)
{
    m_startPlayerHp = beforeView.playerHpBar.currentValue;
    m_startMonsterHp = beforeView.monsterHpBar.currentValue;
    m_startMercy = beforeView.mercyBar.currentValue;
    m_displayedPlayerHp = beforeView.playerHpBar.currentValue;
    m_displayedMonsterHp = beforeView.monsterHpBar.currentValue;
    m_displayedMercy = beforeView.mercyBar.currentValue;
    m_targetPlayerHp = afterView.playerHpBar.currentValue;
    m_targetMonsterHp = afterView.monsterHpBar.currentValue;
    m_targetMercy = afterView.mercyBar.currentValue;
    m_hasCachedBattleView = true;
    m_cachedBattleView = afterView;
    m_battlePresentationPhase = BattlePresentationPhase::PlayerActionText;
    m_pendingMonsterFxClip = getFxClipForAttackId(toAssetSlug(afterView.monsterElementType));
    m_pendingMonsterFlashColor = getElementColor(afterView.monsterElementType);

    m_battlePresentationClock.restart();
}

void FrontendApp::updateBattlePresentation()
{
    if (m_battlePresentationPhase == BattlePresentationPhase::Idle)
    {
        return;
    }

    const float durationSeconds = (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText
                                   || m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText)
        ? 0.62f
        : 0.42f;
    const float t = std::clamp(m_battlePresentationClock.getElapsedTime().asSeconds() / durationSeconds, 0.f, 1.f);

    if (m_battlePresentationPhase == BattlePresentationPhase::PlayerActionText)
    {
        if (t >= 1.f)
        {
            m_battlePresentationClock.restart();
            if (m_startMercy != m_targetMercy)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MercyChange;
            }
            else if (m_startMonsterHp != m_targetMonsterHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MonsterHpDrop;
            }
            else if (m_startPlayerHp != m_targetPlayerHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MonsterActionText;
            }
            else
            {
                m_battlePresentationPhase = BattlePresentationPhase::Idle;
            }
        }
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MercyChange)
    {
        const float value = static_cast<float>(m_startMercy)
            + static_cast<float>(m_targetMercy - m_startMercy) * t;
        m_displayedMercy = static_cast<int>(std::round(value));

        if (t >= 1.f)
        {
            m_displayedMercy = m_targetMercy;
            m_battlePresentationClock.restart();
            if (m_startMonsterHp != m_targetMonsterHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MonsterHpDrop;
            }
            else if (m_startPlayerHp != m_targetPlayerHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MonsterActionText;
            }
            else
            {
                m_battlePresentationPhase = BattlePresentationPhase::Idle;
            }
        }
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MonsterHpDrop)
    {
        const float value = static_cast<float>(m_startMonsterHp)
            + static_cast<float>(m_targetMonsterHp - m_startMonsterHp) * t;
        m_displayedMonsterHp = static_cast<int>(std::round(value));

        if (t >= 1.f)
        {
            m_displayedMonsterHp = m_targetMonsterHp;
            m_battlePresentationClock.restart();
            if (m_startPlayerHp != m_targetPlayerHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::MonsterActionText;
            }
            else
            {
                m_battlePresentationPhase = BattlePresentationPhase::Idle;
            }
        }
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::MonsterActionText)
    {
        if (!m_pendingMonsterFxClip.empty())
        {
            queueBattlerAnimation(m_monsterBattleAnimation, m_monsterBattleAnimation.actionKey, 0.40f);
            queueBattleFx(m_pendingMonsterFxClip.empty() ? "fx_hit" : m_pendingMonsterFxClip,
                          rectf(170.f, 344.f, 220.f, 160.f),
                          0.38f);
            triggerBattleFlash(m_pendingMonsterFlashColor);
            m_pendingMonsterFxClip.clear();
        }

        if (t >= 1.f)
        {
            m_battlePresentationClock.restart();
            if (m_startPlayerHp != m_targetPlayerHp)
            {
                m_battlePresentationPhase = BattlePresentationPhase::PlayerHpDrop;
            }
            else
            {
                m_battlePresentationPhase = BattlePresentationPhase::Idle;
            }
        }
    }
    else if (m_battlePresentationPhase == BattlePresentationPhase::PlayerHpDrop)
    {
        const float value = static_cast<float>(m_startPlayerHp)
            + static_cast<float>(m_targetPlayerHp - m_startPlayerHp) * t;
        m_displayedPlayerHp = static_cast<int>(std::round(value));

        if (t >= 1.f)
        {
            m_displayedPlayerHp = m_targetPlayerHp;
            m_battlePresentationPhase = BattlePresentationPhase::Idle;
        }
    }
}

void FrontendApp::updateBattleAnimationBindings(const string& playerAppearance, const string& monsterName)
{
    m_playerBattleAnimation.idleKey = "player_" + toAssetSlug(playerAppearance) + "_idle";
    m_playerBattleAnimation.actionKey = "player_" + toAssetSlug(playerAppearance) + "_attack";
    m_playerBattleAnimation.hurtKey.clear();

    const string monsterSlug = toAssetSlug(monsterName);
    m_monsterBattleAnimation.idleKey = monsterSlug + "_idle";
    m_monsterBattleAnimation.actionKey = monsterSlug + "_attack";
    m_monsterBattleAnimation.hurtKey = monsterSlug + "_hurt";
}

string FrontendApp::getFxClipForAttackId(const string& attackId) const
{
    if (attackId == "ember")
    {
        return "fx_fire";
    }
    if (attackId == "pulse")
    {
        return "fx_arcane";
    }
    if (attackId == "blade")
    {
        return "fx_blade";
    }
    if (attackId == "tide")
    {
        return "fx_water";
    }
    if (attackId == "act")
    {
        return "fx_arcane";
    }
    if (attackId == "item")
    {
        return "fx_hit";
    }
    if (attackId == "fire")
    {
        return "fx_fire";
    }
    if (attackId == "water")
    {
        return "fx_water";
    }
    if (attackId == "shadow" || attackId == "arcane")
    {
        return "fx_arcane";
    }
    if (attackId == "light" || attackId == "nature" || attackId == "earth" || attackId == "metal" || attackId == "storm")
    {
        return "fx_hit";
    }

    return "fx_hit";
}

string FrontendApp::tr(const string& englishText, const string& frenchText) const
{
    return m_languageCode == "fr" ? frenchText : englishText;
}

string FrontendApp::trAppearanceLabel(const string& appearanceId) const
{
    if (appearanceId == "wanderer")
    {
        return tr("Wanderer", "Voyageur");
    }
    if (appearanceId == "vanguard")
    {
        return tr("Vanguard", "Avant-garde");
    }
    if (appearanceId == "mystic")
    {
        return tr("Mystic", "Mystique");
    }

    return Game::getHeroAppearanceLabel(appearanceId);
}

string FrontendApp::localizeMenuLabel(const FrontendActionButtonViewData& button) const
{
    if (button.id == "bestiary")
    {
        return tr("Bestiary", "Bestiaire");
    }
    if (button.id == "start_battle")
    {
        return tr("Start Battle", "Lancer un combat");
    }
    if (button.id == "player_stats")
    {
        return tr("Player Stats", "Statistiques");
    }
    if (button.id == "items")
    {
        return tr("Items", "Objets");
    }
    if (button.id == "quit")
    {
        return tr("Quit", "Quitter");
    }

    return button.label;
}

string FrontendApp::localizeBattleActionLabel(const FrontendActionButtonViewData& button) const
{
    if (button.id == "fight")
    {
        return tr("FIGHT", "ATTAQUE");
    }
    if (button.id == "act")
    {
        return "ACT";
    }
    if (button.id == "item")
    {
        return tr("ITEM", "OBJET");
    }
    if (button.id == "mercy")
    {
        return tr("MERCY", "PITIE");
    }
    if (button.id == "back")
    {
        return tr("BACK", "RETOUR");
    }

    return button.label;
}

FrontendApp::RenderProfile FrontendApp::getRenderProfile(const string& baseSlug) const
{
    RenderProfile profile;
    profile.padding = 8.f;

    if (baseSlug == "player_wanderer")
    {
        return {1.12f, 0.05f, 6.f, sf::Color(124, 201, 222, 34)};
    }
    if (baseSlug == "player_vanguard")
    {
        return {1.06f, 0.07f, 6.f, sf::Color(255, 198, 118, 32)};
    }
    if (baseSlug == "player_mystic")
    {
        return {1.14f, 0.03f, 8.f, sf::Color(214, 138, 255, 34)};
    }
    if (baseSlug == "froggit")
    {
        return {1.22f, 0.06f, 4.f, sf::Color(116, 215, 120, 24)};
    }
    if (baseSlug == "mimicbox")
    {
        return {1.08f, 0.02f, 8.f, sf::Color(214, 138, 255, 24)};
    }
    if (baseSlug == "dustling")
    {
        return {1.18f, 0.01f, 6.f, sf::Color(150, 110, 220, 26)};
    }
    if (baseSlug == "glimmermoth")
    {
        return {1.14f, -0.01f, 6.f, sf::Color(255, 240, 150, 26)};
    }
    if (baseSlug == "pebblimp")
    {
        return {1.1f, 0.06f, 6.f, sf::Color(185, 145, 92, 24)};
    }
    if (baseSlug == "howlscreen")
    {
        return {1.22f, -0.03f, 6.f, sf::Color(255, 124, 72, 24)};
    }
    if (baseSlug == "bloomcobra")
    {
        return {1.22f, 0.1f, 0.f, sf::Color(116, 215, 120, 24)};
    }
    if (baseSlug == "queenbyte")
    {
        return {1.18f, 0.03f, 2.f, sf::Color(96, 176, 255, 28)};
    }
    if (baseSlug == "archivore")
    {
        return {1.15f, 0.04f, 4.f, sf::Color(150, 110, 220, 28)};
    }
    if (baseSlug == "player_default")
    {
        return {1.1f, 0.05f, 6.f, sf::Color(124, 201, 222, 28)};
    }

    return profile;
}

sf::FloatRect FrontendApp::makeRect(float x, float y, float width, float height)
{
    return rectf(x, y, width, height);
}

bool FrontendApp::contains(const sf::FloatRect& rect, const sf::Vector2f& point)
{
    return rect.contains(point);
}

sf::Color FrontendApp::getRegionColor(const string& regionName)
{
    if (regionName == "Sunken Mire")
    {
        return sf::Color(58, 91, 76);
    }
    if (regionName == "Glass Dunes")
    {
        return sf::Color(118, 148, 149);
    }
    if (regionName == "Signal Wastes")
    {
        return sf::Color(125, 72, 54);
    }
    if (regionName == "Ancient Vault")
    {
        return sf::Color(64, 62, 88);
    }

    return sf::Color(40, 40, 52);
}

sf::Color FrontendApp::getElementColor(const string& elementType)
{
    if (elementType == "Fire")
    {
        return sf::Color(255, 124, 72, 160);
    }
    if (elementType == "Water")
    {
        return sf::Color(96, 176, 255, 160);
    }
    if (elementType == "Shadow")
    {
        return sf::Color(150, 110, 220, 160);
    }
    if (elementType == "Light")
    {
        return sf::Color(255, 240, 150, 160);
    }
    if (elementType == "Nature")
    {
        return sf::Color(116, 215, 120, 160);
    }
    if (elementType == "Earth")
    {
        return sf::Color(185, 145, 92, 160);
    }
    if (elementType == "Metal")
    {
        return sf::Color(185, 198, 216, 160);
    }
    if (elementType == "Storm")
    {
        return sf::Color(132, 224, 255, 160);
    }
    if (elementType == "Arcane")
    {
        return sf::Color(214, 138, 255, 160);
    }

    return sf::Color(255, 220, 153, 140);
}

string FrontendApp::toAssetSlug(const string& text)
{
    string slug;
    slug.reserve(text.size());

    for (char ch : text)
    {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (std::isalnum(value))
        {
            slug.push_back(static_cast<char>(std::tolower(value)));
        }
        else if (ch == ' ' || ch == '-' || ch == '_')
        {
            if (slug.empty() || slug.back() == '_')
            {
                continue;
            }
            slug.push_back('_');
        }
    }

    if (!slug.empty() && slug.back() == '_')
    {
        slug.pop_back();
    }

    return slug;
}
