#include <iostream>
#include <string>
#include <algorithm>
#include <utility>
#include <optional>
#include <vector>
#include <map>

#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/System/String.hpp"
#include "simdjson.h"
#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Font.hpp"
#include "SFML/Graphics/Sprite.hpp"
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
    constexpr int LOCATION_BUTTONS_X = 280;
    constexpr int LOCATION_BUTTONS_Y = 12;
    constexpr int EXIT_BUTTON_X = 340;
    constexpr int EXIT_BUTTON_Y = 12;
    constexpr int STAR_AREA_X = 12;
    constexpr int STAR_AREA_Y = 77;
    constexpr int STAR_AREA_WIDTH = 376;
    constexpr int STAR_AREA_HEIGHT = 344;
    constexpr int INFO_PANEL_X = 12;
    constexpr int INFO_PANEL_Y = 433;
    constexpr int SEARCH_BAR_X = 80;
    constexpr int SEARCH_BAR_Y = 91;
}

struct City {
    std::string country;
    std::string admin;
    std::string name;
    double latitude;
    double longitude;
};

std::vector<City> loadWorld() {
    std::vector<City> allCities;

    simdjson::padded_string json_data;
    auto error = simdjson::padded_string::load("assets/cities_data.json").get(json_data);
    
    if (error) {
        std::cerr << "Error loading JSON file: " << error << std::endl;
        return allCities;
    }

    simdjson::dom::parser parser;

    simdjson::dom::element doc;
    error = parser.parse(json_data).get(doc);
    
    if (error) {
        std::cerr << "Error parsing JSON: " << error << std::endl;
        return allCities;
    }

    for (simdjson::dom::element city_element : doc.get_array()) {
        try {
            City city;
            city.country = std::string(city_element["ct"].get_string().value());
            city.admin = std::string(city_element["ad"].get_string().value());
            city.name = std::string(city_element["nm"].get_string().value());
            city.latitude = city_element["lt"].get_double().value();
            city.longitude = city_element["ln"].get_double().value();

            allCities.push_back(city);
        } catch (const simdjson::simdjson_error& e) {
            std::cerr << "Error processing city data: " << e.what() << std::endl;
            continue;
        }
    }
    return allCities;
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void updateSearchResults(const std::string& query, const std::vector<City>& allCities, std::vector<std::pair<sf::Text, City>>& results, const sf::Font& font) {
    results.clear();
    if (query.empty()) {
        return;
    }

    std::vector<std::string> searchTerms;
    std::stringstream ss(query);
    std::string term;
    while (std::getline(ss, term, ',')) {
        size_t first = term.find_first_not_of(' ');
        if (std::string::npos == first) {
            searchTerms.push_back("");
        } else {
            size_t last = term.find_last_not_of(' ');
            searchTerms.push_back(toLower(term.substr(first, (last - first + 1))));
        }
    }

    unsigned int maxResults = 15;
    float startY = AppConfig::SEARCH_BAR_Y + 67.5;
    float lineSpacing = 27.0f;

    for (const auto& city : allCities) {
        bool match = true;
        if (!searchTerms.empty() && !searchTerms[0].empty()) {
            std::string lowerCityName = toLower(city.name);
            if (lowerCityName.rfind(searchTerms[0], 0) != 0) {
                match = false;
            }
        }

        if (match && searchTerms.size() > 1 && !searchTerms[1].empty()) {
            std::string lowerAdmin = toLower(city.admin);
            if (lowerAdmin.rfind(searchTerms[1], 0) != 0) {
                match = false;
            }
        }

        if (match && searchTerms.size() > 2 && !searchTerms[2].empty()) {
            std::string lowerCountry = toLower(city.country);
            if (lowerCountry.rfind(searchTerms[2], 0) != 0) {
                match = false;
            }
        }

        if (match) {
            if (results.size() < maxResults) {
                sf::Text resultText(font);

                std::string cityInfo = city.name + ", " + city.admin + ", " + city.country;
                int length = cityInfo.length();
                if (length > 21) {
                    cityInfo.erase(21, length - 21);
                    cityInfo += "...";
                }

                resultText.setString(sf::String::fromUtf8(cityInfo.begin(), cityInfo.end()));
                resultText.setCharacterSize(25);
                resultText.setPosition({(float)AppConfig::SEARCH_BAR_X, startY + (results.size() * lineSpacing)});
                resultText.setFillColor(sf::Color::Yellow);
                results.push_back({resultText, city});
            } else {
                break;
            }
        }
    }
}
void updateMoonDisplay(MoonInfo& moonInfo, sf::Texture& moonTexture, sf::Sprite& moonSprite, sf::Text& infoText, const std::map<std::string, std::string>& phaseToFilename) {
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
        return;
    }
    moonSprite.setTexture(moonTexture);

    sf::FloatRect moonBounds = moonSprite.getLocalBounds();
    moonSprite.setOrigin({moonBounds.size.x / 2.f, moonBounds.size.y / 2.f});
    moonSprite.setPosition({
        AppConfig::STAR_AREA_X + (AppConfig::STAR_AREA_WIDTH / 2.f),
        AppConfig::STAR_AREA_Y + (AppConfig::STAR_AREA_HEIGHT / 2.f)
    });

    infoText.setString(
        "Illumination: " + moonInfo.illumination + "%\n" +
        "Phase: " + moonInfo.phase + "\n" +
        "Moonrise: " + moonInfo.riseTimeString + "\n" +
        "Moonset: " + moonInfo.setTimeString
    );
}

enum class AppState {
    MainView,
    SearchView
};

struct SearchResultItem {
    sf::Text text;
    City city;
    sf::RectangleShape highlightRect;
};

int main() {
    sf::RenderWindow window(sf::VideoMode({AppConfig::FRAME_WIDTH, AppConfig::FRAME_HEIGHT}), "Tsuki", sf::Style::None);
    window.setFramerateLimit(60);

    AppState state = AppState::MainView;

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

    sf::Texture searchTexture;
    if (!searchTexture.loadFromFile("assets/search.png")) {
        std::cerr << "Error: Could not load search image from assets/search.png" << std::endl;
        return -1;
    }
    
    sf::Sprite searchSprite(searchTexture);

    sf::Texture exitTexture, exitHoverTexture;
    if (!exitTexture.loadFromFile("assets/exit.png") ||
        !exitHoverTexture.loadFromFile("assets/exit_hover.png")) {
        std::cerr << "Error: Could not load exit button images\n";
        return -1;
    }

    sf::Sprite exitSprite(exitTexture);
    exitSprite.setPosition({AppConfig::EXIT_BUTTON_X, AppConfig::EXIT_BUTTON_Y});

    sf::Texture globeTexture, globeHoverTexture;
    if (!globeTexture.loadFromFile("assets/globe.png") ||
        !globeHoverTexture.loadFromFile("assets/globe_hover.png")) {
        std::cerr << "Error: Could not load globe button's images\n";
        return -1;
    }

    sf::Sprite globeSprite(globeTexture);
    globeSprite.setPosition({AppConfig::LOCATION_BUTTONS_X, AppConfig::LOCATION_BUTTONS_Y});

    sf::Texture backTexture, backHoverTexture;
    if (!backTexture.loadFromFile("assets/back.png") ||
        !backHoverTexture.loadFromFile("assets/back_hover.png")) {
        std::cerr << "Error: Could not load back button's images\n";
        return -1;
    }

    sf::Sprite backSprite(backTexture);
    backSprite.setPosition({AppConfig::LOCATION_BUTTONS_X, AppConfig::LOCATION_BUTTONS_Y});

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

    double lat = 50.271790;
    double lng = -119.276505;
    MoonInfo moonInfo(lat, lng);

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
    if (!font.openFromFile("assets/Pixellari.ttf")) {
        std::cout << "Failed to load font.";
    }

    std::string searchInputString;
    sf::Text searchInputText(font);
    searchInputText.setCharacterSize(25);
    searchInputText.setFillColor(sf::Color::Yellow);
    searchInputText.setPosition({AppConfig::SEARCH_BAR_X, AppConfig::SEARCH_BAR_Y});
    std::vector<std::pair<sf::Text, City>> searchResults;

    std::vector<City> allCities = loadWorld();

    sf::Text text(font);
    text.setString(
        "Illumination: " + moonInfo.illumination + "%\n" +
        "Phase: " + moonInfo.phase + "\n" +
        "Moonrise: " + moonInfo.riseTimeString + "\n" +
        "Moonset: " + moonInfo.setTimeString);
    text.setCharacterSize(29);
    text.setFillColor(sf::Color::Yellow);
    text.setPosition({AppConfig::INFO_PANEL_X + 44, AppConfig::INFO_PANEL_Y + 9});

    sf::Vector2i dragOffset;
    bool draggingWindow = false;

    const sf::IntRect draggableArea({0, 0}, {AppConfig::FRAME_WIDTH, 60});

    updateMoonDisplay(moonInfo, moonTexture, moonSprite, text, phaseToFilename);

    sf::RectangleShape searchHighlight({286, 27.466});
    searchHighlight.setFillColor(sf::Color(251,65,65));
    searchHighlight.setPosition({75, 158});
    int cityResults = 0;

    while (window.isOpen()) {
        sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (mouseWorld.y >= 158 && mouseWorld.y <= 569) {
            int locationOffset = (double)((mouseWorld.y - 158) / 27.466666667);

            switch (locationOffset) {
                case (0):
                if (1 > cityResults) break;
                searchHighlight.setPosition({75, 158});
                break;
                case (1):
                if (2 > cityResults) break;
                searchHighlight.setPosition({75, 185.46666667});
                break;
                case (2):
                if (3 > cityResults) break;
                searchHighlight.setPosition({75, 212.93333333});
                break;
                case (3):
                if (4 > cityResults) break;
                searchHighlight.setPosition({75, 240.4});
                break;
                case (4):
                if (5 > cityResults) break;
                searchHighlight.setPosition({75, 267.86666667});
                break;
                case (5):
                if (6 > cityResults) break;
                searchHighlight.setPosition({75, 295.333333333});
                break;
                case (6):
                if (7 > cityResults) break;
                searchHighlight.setPosition({75, 322.8});
                break;
                case (7):
                if (8 > cityResults) break;
                searchHighlight.setPosition({75, 350.26666667});
                break;
                case (8):
                if (9 > cityResults) break;
                searchHighlight.setPosition({75, 377.733333333});
                break;
                case (9):
                if (10 > cityResults) break;
                searchHighlight.setPosition({75, 405.2});
                break;
                case (10):
                if (11 > cityResults) break;
                searchHighlight.setPosition({75, 432.66666667});
                break;
                case (11):
                if (12 > cityResults) break;
                searchHighlight.setPosition({75, 460.1333333});
                break;
                case (12):
                if (13 > cityResults) break;
                searchHighlight.setPosition({75, 487.6});
                break;
                case (13):
                if (14 > cityResults) break;
                searchHighlight.setPosition({75, 515.06666667});
                break;
                case (14):
                if (15 > cityResults) break;
                searchHighlight.setPosition({75, 542.5333333});
                break;
            }
        }

        bool mouseOverGlobe = globeSprite.getGlobalBounds().contains(mouseWorld);
        bool mouseOverBack = backSprite.getGlobalBounds().contains(mouseWorld);
        bool mouseOverExit = exitSprite.getGlobalBounds().contains(mouseWorld);

        while (const std::optional<sf::Event> event = window.pollEvent()) {

            if (state == AppState::SearchView) {
                if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                    if (textEntered->unicode < 128) {
                        if (textEntered->unicode == 8 && !searchInputString.empty()) {
                            searchInputString.pop_back();
                        } else if (textEntered->unicode != 8) {
                            searchInputString += static_cast<char>(textEntered->unicode);
                        }
                        searchInputText.setString(sf::String::fromUtf8(searchInputString.begin(), searchInputString.end()));
                        updateSearchResults(searchInputString, allCities, searchResults, font); // Pass searchResults
                        searchHighlight.setPosition({75, 158});
                        cityResults = searchResults.size();
                    }
                }
            }

            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    if (draggableArea.contains({mouseButtonPressed->position.x, mouseButtonPressed->position.y})) {
                        draggingWindow = true;
                        dragOffset = sf::Mouse::getPosition() - window.getPosition();
                    }
                }
            } else if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
                    draggingWindow = false;
                    sf::Vector2f clickPosition = window.mapPixelToCoords({mouseButtonReleased->position.x, mouseButtonReleased->position.y});

                    if (exitSprite.getGlobalBounds().contains(clickPosition)) {
                        window.close();
                    }

                    switch (state) {
                        case AppState::MainView:
                            if (globeSprite.getGlobalBounds().contains(clickPosition)) {
                                state = AppState::SearchView;
                                searchInputString.clear();
                                searchInputText.setString("");
                                searchResults.clear();
                                cityResults = 0;
                            }
                            break;
                        case AppState::SearchView:
                            if (backSprite.getGlobalBounds().contains(clickPosition)) {
                                state = AppState::MainView;
                            } else {
                                for (const auto& pair : searchResults) {
                                    if (pair.first.getGlobalBounds().contains(clickPosition)) {
                                        const City& selectedCity = pair.second;
                                        lat = selectedCity.latitude;
                                        lng = selectedCity.longitude;

                                        moonInfo = MoonInfo(lat, lng);
                                        updateMoonDisplay(moonInfo, moonTexture, moonSprite, text, phaseToFilename);
                                        state = AppState::MainView;
                                        searchInputString.clear();
                                        searchInputText.setString("");
                                        searchResults.clear();
                                        break;
                                    }
                                }
                            }
                            break;
                    }
                }
            }
        }

        exitSprite.setTexture(mouseOverExit ? exitHoverTexture : exitTexture);
        globeSprite.setTexture(mouseOverGlobe ? globeHoverTexture : globeTexture);
        backSprite.setTexture(mouseOverBack ? backHoverTexture : backTexture);

        if (draggingWindow) {
            sf::Vector2i newPosition = sf::Mouse::getPosition() - dragOffset;
            window.setPosition(newPosition);
        }

        starAnimation.update();

        window.clear();

        switch (state) {
            case AppState::MainView:
                window.draw(backgroundSprite);
                window.draw(exitSprite);
                window.draw(globeSprite);
                window.draw(starAnimation);
                window.draw(moonSprite);
                window.draw(text);
                break;
            case AppState::SearchView:
                window.draw(searchSprite);
                window.draw(exitSprite);
                window.draw(backSprite);
                if (cityResults != 0) window.draw(searchHighlight);
                window.draw(searchInputText);
                for (const auto& pair : searchResults) {
                    window.draw(pair.first);
                }
        }

        window.display();
    }

    return 0;
}