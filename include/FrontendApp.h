#ifndef FRONTENDAPP_H
#define FRONTENDAPP_H

#include "Game.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

class FrontendApp
{
public:
    FrontendApp();

    bool initialize();
    void run();

private:
    struct RenderProfile
    {
        float scaleMultiplier = 1.f;
        float verticalBias = 0.f;
        float padding = 0.f;
        sf::Color glowColor = sf::Color::Transparent;
        float bobAmplitude = 0.f;
        float bobSpeed = 1.f;
        float swayAmplitude = 0.f;
        float rotationAmplitude = 0.f;
        float scalePulse = 0.f;
    };

    struct AnimationClip
    {
        std::vector<sf::Texture> frames;
        float frameDuration = 0.16f;
        bool loop = true;
    };

    struct BattleAnimationState
    {
        std::string idleKey;
        std::string actionKey;
        std::string hurtKey;
        std::string overrideKey;
        sf::Clock overrideClock;
        float overrideDurationSeconds = 0.f;
    };

    struct BattleFxState
    {
        std::string clipKey;
        sf::FloatRect area;
        sf::Clock clock;
        float durationSeconds = 0.f;
    };

    enum class BattlePresentationPhase
    {
        Idle,
        PlayerActionText,
        MercyChange,
        MonsterHpDrop,
        MonsterActionText,
        PlayerHpDrop
    };

    enum class Screen
    {
        LanguageSelect,
        HeroSelect,
        Menu,
        PlayerStats,
        Bestiary,
        Items,
        MonsterSelect,
        Battle
    };

    enum class BattleOverlay
    {
        None,
        FightList,
        ActList,
        ItemList
    };

    Game m_game;
    sf::RenderWindow m_window;
    sf::Font m_font;
    sf::View m_uiView;
    std::map<std::string, sf::Texture> m_textures;
    std::map<std::string, AnimationClip> m_animationClips;
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::map<std::string, std::unique_ptr<sf::Sound>> m_sounds;
    std::map<std::string, std::string> m_musicTracks;
    std::unique_ptr<sf::Music> m_musicPlayer;
    std::string m_currentMusicKey;
    sf::Clock m_animationClock;
    sf::Clock m_screenTransitionClock;
    sf::Clock m_battleFlashClock;
    sf::Clock m_battlePresentationClock;
    sf::Color m_battleFlashColor;
    Screen m_currentScreen;
    BattleOverlay m_battleOverlay;
    std::size_t m_selectedMenuIndex;
    std::size_t m_selectedLanguageIndex;
    std::size_t m_selectedHeroAppearanceIndex;
    std::size_t m_selectedMonsterIndex;
    std::size_t m_selectedBattleActionIndex;
    std::size_t m_selectedOverlayIndex;
    std::size_t m_selectedBestiaryIndex;
    std::size_t m_selectedItemIndex;
    std::string m_languageCode;
    std::string m_statusText;
    bool m_fullscreen;
    BattleAnimationState m_playerBattleAnimation;
    BattleAnimationState m_monsterBattleAnimation;
    BattleFxState m_battleFxState;
    BattlePresentationPhase m_battlePresentationPhase;
    int m_displayedPlayerHp;
    int m_displayedMonsterHp;
    int m_displayedMercy;
    int m_startPlayerHp;
    int m_startMonsterHp;
    int m_startMercy;
    int m_targetPlayerHp;
    int m_targetMonsterHp;
    int m_targetMercy;
    bool m_hasPendingScreenChange;
    Screen m_pendingScreen;
    std::string m_pendingScreenStatus;
    bool m_hasCachedBattleView;
    FrontendBattleViewData m_cachedBattleView;
    std::string m_pendingPlayerAttackId;
    std::string m_pendingMonsterFxClip;
    std::string m_pendingMonsterSoundKey;
    sf::Color m_pendingMonsterFlashColor;

    bool loadFont();
    void loadOptionalTextures();
    void loadOptionalAnimations();
    void loadAnimationDirectory(const std::string& clipKey,
                                const std::string& directoryPath,
                                float frameDuration,
                                bool loop = true);
    void loadOptionalSounds();
    void loadOptionalMusic();
    void playSoundIfAvailable(const std::string& soundKey, float volume = 65.f);
    void playMusicTrackIfAvailable(const std::string& musicKey, float volume = 0.f);
    float getRecommendedMusicVolume(const std::string& musicKey) const;
    void updateMusicForCurrentContext();
    std::string getMusicKeyForCurrentContext() const;
    void recreateWindow();
    void updateUIView();
    sf::Vector2f mapMouseToUi(const sf::Vector2i& pixelPosition) const;
    void processEvents();
    void render();

    void renderMenu();
    void renderLanguageSelect();
    void renderHeroSelect();
    void renderPlayerStats();
    void renderBestiary();
    void renderItems();
    void renderMonsterSelect();
    void renderBattle();
    void renderOverlay(const std::vector<FrontendActionButtonViewData>& options, const std::string& title);
    void renderTransitionOverlay();
    void renderBattleEnvironment(const std::string& regionName,
                                 const std::string& elementType,
                                 const std::string& category,
                                 const std::string& monsterSlug);

    void drawPanel(const sf::FloatRect& rect, const sf::Color& fill, float outlineThickness = 3.f);
    void drawBackgroundGradient(const sf::Color& topColor, const sf::Color& bottomColor);
    void drawBackgroundTexture(const std::string& textureKey, const sf::Color& fallbackTop, const sf::Color& fallbackBottom);
    void drawBattleSilhouette(const sf::FloatRect& area,
                              bool monsterSide,
                              const std::string& variantKey,
                              const sf::Color& fillColor,
                              const sf::Color& shadowColor);
    void drawPortrait(const sf::FloatRect& area,
                      const std::string& seedText,
                      bool playerPortrait,
                      const sf::Color& accentColor = sf::Color(235, 222, 196));
    void drawDecorationRow(float y, const sf::Color& color, float spacing, float size);
    bool drawTextureIfAvailable(const std::string& textureKey, const sf::FloatRect& area, const sf::Color& tint = sf::Color::White);
    bool drawContainedTextureIfAvailable(const std::string& textureKey,
                                         const sf::FloatRect& area,
                                         const sf::Color& tint = sf::Color::White,
                                         float padding = 0.f,
                                         bool alignBottom = false);
    bool drawAnimationClipIfAvailable(const std::string& clipKey,
                                      const sf::FloatRect& area,
                                      const sf::Color& tint = sf::Color::White,
                                      float elapsedSeconds = -1.f,
                                      bool flipHorizontally = false);
    bool drawContainedTextureWithProfile(const std::string& textureKey,
                                         const sf::FloatRect& area,
                                         const RenderProfile& profile,
                                         const sf::Color& tint = sf::Color::White,
                                         bool alignBottom = false);
    bool drawAnimationClipWithProfile(const std::string& clipKey,
                                      const sf::FloatRect& area,
                                      const RenderProfile& profile,
                                      const sf::Color& tint = sf::Color::White,
                                      float elapsedSeconds = -1.f,
                                      bool flipHorizontally = false);
    bool drawCharacterShowcase(const std::string& baseSlug,
                               const sf::FloatRect& area,
                               bool playerSide,
                               const sf::Color& fallbackFill,
                               const sf::Color& fallbackShadow);
    bool drawBattlerAnimation(BattleAnimationState& state,
                              const sf::FloatRect& area,
                              const sf::Color& tint = sf::Color::White,
                              bool flipHorizontally = false);
    void drawText(const std::string& text,
                  const sf::Vector2f& position,
                  unsigned int size,
                  const sf::Color& color,
                  bool centered = false);
    void drawWrappedText(const std::string& text,
                         const sf::Vector2f& position,
                         unsigned int size,
                         const sf::Color& color,
                         float maxWidth,
                         float lineSpacing = 1.2f);
    void drawElementBadge(const std::string& elementType,
                          const sf::Vector2f& center,
                          float radius);
    void drawStatBar(const FrontendStatBarViewData& bar,
                     const sf::Vector2f& position,
                     const sf::Vector2f& size,
                     const sf::Color& fillColor);

    void handleMenuEvent(const sf::Event& event);
    void handleLanguageSelectEvent(const sf::Event& event);
    void handleHeroSelectEvent(const sf::Event& event);
    void handlePlayerStatsEvent(const sf::Event& event);
    void handleBestiaryEvent(const sf::Event& event);
    void handleItemsEvent(const sf::Event& event);
    void handleMonsterSelectEvent(const sf::Event& event);
    void handleBattleEvent(const sf::Event& event);

    void handleMenuClick(const sf::Vector2f& point);
    void handleLanguageSelectClick(const sf::Vector2f& point);
    void handleHeroSelectClick(const sf::Vector2f& point);
    void handlePlayerStatsClick(const sf::Vector2f& point);
    void handleBestiaryClick(const sf::Vector2f& point);
    void handleItemsClick(const sf::Vector2f& point);
    void handleMonsterSelectClick(const sf::Vector2f& point);
    void handleBattleClick(const sf::Vector2f& point);

    void activateMenuChoice();
    void confirmSelectedLanguage();
    void startSelectedMonsterBattle();
    void changeScreen(Screen newScreen, const std::string& statusText = "");
    void triggerBattleFlash(const sf::Color& color);
    void queueBattlerAnimation(BattleAnimationState& state, const std::string& clipKey, float durationSeconds);
    void queueBattleFx(const std::string& clipKey, const sf::FloatRect& area, float durationSeconds);
    void renderBattleFx();
    void primeBattlePresentation(const FrontendBattleViewData& viewData);
    void beginBattlePresentation(const FrontendBattleViewData& beforeView, const FrontendBattleViewData& afterView);
    void updateBattlePresentation();
    void updateBattleAnimationBindings(const std::string& playerAppearance, const std::string& monsterName);
    std::string getFxClipForAttackId(const std::string& attackId) const;
    std::string getSoundKeyForAttackId(const std::string& attackId) const;
    std::string getOutcomeSoundKey(const std::string& statusText) const;
    std::string getMonsterResponseSoundKey(const FrontendBattleViewData& beforeView,
                                           const FrontendBattleViewData& afterView) const;
    std::string tr(const std::string& englishText, const std::string& frenchText) const;
    std::string localizeDynamicText(const std::string& text) const;
    std::string trAppearanceLabel(const std::string& appearanceId) const;
    std::string localizeMenuLabel(const FrontendActionButtonViewData& button) const;
    std::string localizeBattleActionLabel(const FrontendActionButtonViewData& button) const;
    RenderProfile getRenderProfile(const std::string& baseSlug) const;

    static sf::FloatRect makeRect(float x, float y, float width, float height);
    static bool contains(const sf::FloatRect& rect, const sf::Vector2f& point);
    static std::string getCanonicalRegionKey(const std::string& regionName);
    static sf::Color getRegionColor(const std::string& regionName);
    static sf::Color getElementColor(const std::string& elementType);
    static std::string toAssetSlug(const std::string& text);
};

#endif

