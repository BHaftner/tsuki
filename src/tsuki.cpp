#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
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
#include "httplib.h"
#include "json.hpp"
#include <libnova/julian_day.h>
#include <libnova/ln_types.h>
#include <libnova/lunar.h>
#include <libnova/rise_set.h>
#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using json = nlohmann::json;

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

    std::optional<sf::Sprite>  m_sprite;
    std::vector<sf::Texture>   m_textures;
    size_t                     m_currentFrame = 0;
    sf::Time                   m_frameDuration;
    sf::Clock                  m_frameClock;
};


namespace AppConfig {
    constexpr int FRAME_WIDTH = 400;
    constexpr int FRAME_HEIGHT = 600;
    constexpr int DRAG_AREA_HEIGHT = 60;
    constexpr int EXIT_BUTTON_X = 328;
    constexpr int EXIT_BUTTON_Y = 0;
    constexpr int STAR_AREA_X = 12;
    constexpr int STAR_AREA_Y = 77;
    constexpr int STAR_AREA_WIDTH = 376;
    constexpr int STAR_AREA_HEIGHT = 344;
    constexpr int INFO_PANEL_X = 12;
    constexpr int INFO_PANEL_Y = 433;
    constexpr int INFO_PANEL_WIDTH = 375;
    constexpr int INFO_PANEL_HEIGHT = 155;
}

std::string militaryToStandard(const ln_zonedate& time) {
    int hour = time.hours;
    std::string ampm = (hour >= 12) ? "pm" : "am";

    if (hour == 0) {
        hour = 12;
    } else if (hour > 12) {
        hour -= 12;
    }

    std::stringstream ss;
    ss << hour << ":" << std::setw(2) << std::setfill('0') << time.minutes << ampm;
    return ss.str();
}

class MoonInfo {
public:
    std::string phase;
    std::string illumination;
    std::string riseTimeString;
    std::string setTimeString;

    MoonInfo() {
        const double julianDay = ln_get_julian_from_sys();
        const auto userCoords = fetchCoordinates();

        calculatePhaseAndIllumination(julianDay);
        calculateRiseAndSetTimes(julianDay, userCoords);
    }

private:
    [[nodiscard]] std::pair<double, double> fetchCoordinates() {
        httplib::Client cli("http://ip-api.com");
        auto res = cli.Get("/json/");
        if (res && res->status == 200) {
            try {
                json data = json::parse(res->body);
                return {data["lon"].get<double>(), data["lat"].get<double>()};
            } catch (const json::exception& e) {
                std::cerr << "JSON parsing failed: " << e.what() << std::endl;
            }
        }
        return {0.0, 0.0};
    }

    void calculatePhaseAndIllumination(double julianDay) {
        const double illumValue = ln_get_lunar_disk(julianDay);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (illumValue * 100.0);
        illumination = ss.str();

        constexpr double LUNAR_PHASE_TIME_DELTA = 0.0020833333;
        const bool isWaxing = ln_get_lunar_disk(julianDay + LUNAR_PHASE_TIME_DELTA) > illumValue;

        constexpr double NEW_MOON_MAX = 0.02;
        constexpr double CRESCENT_MAX = 0.49;
        constexpr double QUARTER_MAX = 0.51;
        constexpr double GIBBOUS_MAX = 0.97;

        if (illumValue < NEW_MOON_MAX)       phase = "New";
        else if (illumValue < CRESCENT_MAX)  phase = isWaxing ? "Waxing Crescent" : "Waning Crescent";
        else if (illumValue < QUARTER_MAX)   phase = isWaxing ? "First Quarter" : "Last Quarter";
        else if (illumValue < GIBBOUS_MAX)   phase = isWaxing ? "Waxing Gibbous" : "Waning Gibbous";
        else                                 phase = "Full";
    }

    void calculateRiseAndSetTimes(double julianDay, const std::pair<double, double>& userCoords) {
        ln_lnlat_posn observerCoords = {userCoords.first, userCoords.second};
        ln_rst_time rst;

        if (ln_get_lunar_rst(julianDay, &observerCoords, &rst) == 0) {
            ln_zonedate localRise{}, localSet{};
            ln_get_local_date(rst.rise, &localRise);
            ln_get_local_date(rst.set, &localSet);

            riseTimeString = militaryToStandard(localRise);
            setTimeString = militaryToStandard(localSet);
        } else {
            riseTimeString = "N/A";
            setTimeString = "N/A";
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({AppConfig::FRAME_WIDTH, AppConfig::FRAME_HEIGHT}), "MoonInformation", sf::Style::None);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);

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
        "  Illumination: " + moonInfo.illumination + "%\n" +
        "Phase: " + moonInfo.phase + "\n" +
        "   Moonrise: " + moonInfo.riseTimeString + "\n" +
        "   Moonset: " + moonInfo.setTimeString);
    text.setCharacterSize(35);
    text.setFillColor(sf::Color::Yellow);
    text.setPosition({AppConfig::INFO_PANEL_X + 65, AppConfig::INFO_PANEL_Y + 5});

    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        bool mouseOverExit = exitSprite.getGlobalBounds().contains(mouseWorld);
        exitSprite.setTexture(mouseOverExit ? exitHoverTexture : exitTexture);
        if (mouseOverExit && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            window.close();
        }
    
        ImGui::SFML::Update(window, deltaClock.restart());
        starAnimation.update(); 

        window.clear();
        
        window.draw(backgroundSprite);
        window.draw(exitSprite);
        window.draw(starAnimation);

        window.draw(moonSprite);
        window.draw(text);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}