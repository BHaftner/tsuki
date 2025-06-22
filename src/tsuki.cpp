#include <iostream>
#include <string>
#include <utility>
#include <optional>
#include <vector>
#include <map>

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/Window/VideoMode.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <MoonInfo.cpp>

class FrameAnimator : public sf::Drawable {
public:
    explicit FrameAnimator(sf::Time frameDuration) : m_frameDuration(frameDuration) {}

    bool loadFromFiles(const std::vector<std::string>& filenames) {
        m_textures.reserve(filenames.size());
        for (const auto& filename : filenames) {
            sf::Texture texture;
            if (!texture.loadFromFile(filename)) {
                std::cerr << "Failed to load animation frame: " << filename << std::endl;
                m_textures.clear();
                return false;
            }
            m_textures.push_back(std::move(texture));
        }

        if (!m_textures.empty()) {
            m_sprite.emplace(m_textures[0]);
            m_frameClock.restart();
            return true;
        }
        return false;
    }

    void setPosition(float x, float y) {
        if (m_sprite) {
            m_sprite->setPosition({x, y});
        }
    }

    void setSize(float width, float height) {
        if (!m_sprite || m_textures.empty()) return;
        
        const sf::Vector2u originalSize = m_textures[0].getSize();
        if (originalSize.x == 0 || originalSize.y == 0) return;
        
        float scaleX = width / static_cast<float>(originalSize.x);
        float scaleY = height / static_cast<float>(originalSize.y);
        m_sprite->setScale({scaleX, scaleY});
    }

    void update() {
        if (!m_sprite || m_textures.size() <= 1) return;

        if (m_frameClock.getElapsedTime() >= m_frameDuration) {
            m_currentFrame = (m_currentFrame + 1) % m_textures.size();
            m_sprite->setTexture(m_textures[m_currentFrame]);
            m_frameClock.restart();
        }
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        if (m_sprite) {
            target.draw(*m_sprite, states);
        }
    }

    std::optional<sf::Sprite>   m_sprite;
    std::vector<sf::Texture>    m_textures;
    size_t                      m_currentFrame = 0;
    sf::Time                    m_frameDuration;
    sf::Clock                   m_frameClock;
};


namespace AppConfig {
    constexpr int FRAME_WIDTH = 400;
    constexpr int FRAME_HEIGHT = 600;
    constexpr int EXIT_BUTTON_X = 340;
    constexpr int EXIT_BUTTON_Y = 12;
    constexpr int STAR_AREA_X = 12;
    constexpr int STAR_AREA_Y = 77;
    constexpr int STAR_AREA_WIDTH = 376;
    constexpr int STAR_AREA_HEIGHT = 344;
    constexpr int INFO_PANEL_X = 12;
    constexpr int INFO_PANEL_Y = 433;
    //constexpr int INFO_PANEL_WIDTH = 375;
    //constexpr int INFO_PANEL_HEIGHT = 155;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({AppConfig::FRAME_WIDTH, AppConfig::FRAME_HEIGHT}), "Tsuki", sf::Style::None);
    window.setFramerateLimit(60);

    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    int windowX = (desktopMode.size.x - AppConfig::FRAME_WIDTH) / 2;
    int windowY = (desktopMode.size.y - AppConfig::FRAME_HEIGHT) / 2;
    window.setPosition({windowX, windowY});

    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("assets/background.png")) {
        std::cerr << "Error: Could not load background image from assets/background.png" << std::endl;
        return -1;
    }
    sf::Sprite backgroundSprite(backgroundTexture);

    sf::Texture exitTexture, exitHoverTexture;
    if (!exitTexture.loadFromFile("assets/exit.png") ||
        !exitHoverTexture.loadFromFile("assets/exit_hover.png")) {
        std::cerr << "Error: Could not load exit button images\n";
        return -1;
    }

    sf::Sprite exitSprite(exitTexture);
    exitSprite.setPosition({AppConfig::EXIT_BUTTON_X, AppConfig::EXIT_BUTTON_Y});

    FrameAnimator starAnimation(sf::seconds(0.5));
    std::vector<std::string> starFiles;
    for (int i = 1; i <= 6; ++i) {
        starFiles.push_back("assets/stars" + std::to_string(i) + ".png");
    }

    if (!starAnimation.loadFromFiles(starFiles)) {
        std::cerr << "Error: Could not load star animation frames." << std::endl;
        return -1;
    }
    starAnimation.setPosition(AppConfig::STAR_AREA_X, AppConfig::STAR_AREA_Y);
    starAnimation.setSize(AppConfig::STAR_AREA_WIDTH, AppConfig::STAR_AREA_HEIGHT);

    MoonInfo moonInfo;

    const std::map<std::string, std::string> phaseToFilename = {
        {"New",             "assets/new_moon.png"},
        {"Waxing Crescent", "assets/waxing_crescent.png"},
        {"First Quarter",   "assets/first_quarter.png"},
        {"Waxing Gibbous",  "assets/waxing_gibbous.png"},
        {"Full",            "assets/full_moon.png"},
        {"Waning Gibbous",  "assets/waning_gibbous.png"},
        {"Last Quarter",    "assets/last_quarter.png"},
        {"Waning Crescent", "assets/waning_crescent.png"}
    };

    sf::Texture moonTexture;

    auto it = phaseToFilename.find(moonInfo.phase);
    std::string moonImageFile;
    if (it != phaseToFilename.end()) {
        moonImageFile = it->second;
    } else {
        std::cerr << "Warning: Could not find image for phase: " << moonInfo.phase << ". Defaulting to new moon." << std::endl;
        moonImageFile = "assets/new_moon.png"; 
    }

    if (!moonTexture.loadFromFile(moonImageFile)) {
        std::cerr << "Error: Could not load moon image from " << moonImageFile << std::endl;
        return -1;
    }
    sf::Sprite moonSprite(moonTexture);
    moonSprite.setTexture(moonTexture);

    sf::FloatRect moonBounds = moonSprite.getLocalBounds();
    moonSprite.setOrigin({moonBounds.size.x / 2.f, moonBounds.size.y / 2.f});
    moonSprite.setPosition({
        AppConfig::STAR_AREA_X + (AppConfig::STAR_AREA_WIDTH / 2.f),
        AppConfig::STAR_AREA_Y + (AppConfig::STAR_AREA_HEIGHT / 2.f)
    });

    sf::Font font;
    if (!font.openFromFile("assets/PixelPurl.ttf")) {
        std::cout << "Failed to load font.";
    }
    
    sf::Text text(font);
    text.setString(
        "   Illumination: " + moonInfo.illumination + "%\n" +
        "Phase: " + moonInfo.phase + "\n" +
        "   Moonrise: " + moonInfo.riseTimeString + "\n" +
        "   Moonset: " + moonInfo.setTimeString);
    text.setCharacterSize(35);
    text.setFillColor(sf::Color::Yellow);
    text.setPosition({AppConfig::INFO_PANEL_X + 65, AppConfig::INFO_PANEL_Y + 5});

    sf::Vector2i dragOffset;
    bool draggingWindow = false;

    const sf::IntRect draggableArea({0, 0}, {AppConfig::FRAME_WIDTH, 60});


    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {

        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        else if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                if (draggableArea.contains({mouseButtonPressed->position.x, mouseButtonPressed->position.y})) {
                    draggingWindow = true;
                    dragOffset = sf::Mouse::getPosition() - window.getPosition();
                }
            }
        }
        else if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                draggingWindow = false;
            }
        }
    }

        sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        bool mouseOverExit = exitSprite.getGlobalBounds().contains(mouseWorld);
        exitSprite.setTexture(mouseOverExit ? exitHoverTexture : exitTexture);
        if (mouseOverExit && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            window.close();
        }

        if (draggingWindow) {
            sf::Vector2i newPosition = sf::Mouse::getPosition() - dragOffset;
            window.setPosition(newPosition);
        }
    
        starAnimation.update(); 

        window.clear();
        
        window.draw(backgroundSprite);
        window.draw(exitSprite);
        window.draw(starAnimation);

        window.draw(moonSprite);
        window.draw(text);

        window.display();
    }

    return 0;
}