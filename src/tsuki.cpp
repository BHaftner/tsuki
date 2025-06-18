#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <utility>

#include "httplib.h"
#include "json.hpp"
#include <libnova/julian_day.h>
#include <libnova/ln_types.h>
#include <libnova/lunar.h>
#include <libnova/rise_set.h>

#include <wx/wx.h>
#include <wx/animate.h>
#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/timer.h>
#include <wx/statbmp.h>

using json = nlohmann::json;

namespace AppConfig
{
    constexpr int FRAME_WIDTH = 400;
    constexpr int FRAME_HEIGHT = 600;

    constexpr int EXIT_BUTTON_X = 328;
    constexpr int EXIT_BUTTON_Y = 0;
    constexpr int EXIT_BUTTON_WIDTH = 72;
    constexpr int EXIT_BUTTON_HEIGHT = 77;

    constexpr int INFO_PANEL_X = 12;
    constexpr int INFO_PANEL_Y = 433;
    constexpr int INFO_PANEL_WIDTH = 375;
    constexpr int INFO_PANEL_HEIGHT = 155;

    constexpr int STAR_AREA_X = 12;
    constexpr int STAR_AREA_Y = 77;
    constexpr int STAR_AREA_WIDTH = 376;
    constexpr int STAR_AREA_HEIGHT = 344;

    constexpr int DRAG_AREA_HEIGHT = 60;
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
                wxLogError("JSON parsing failed: %s", e.what());
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

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);
    ~MyFrame() override;

private:
    void OnPaint(wxPaintEvent& event);
    void OnDragBegin(wxMouseEvent& event);
    void OnDragMove(wxMouseEvent& event);
    void OnDragEnd(wxMouseEvent& event);
    void OnCloseButton(wxCommandEvent& event);

    void LoadAssets();
    void PrepareBackground();
    void SetupUI();
    void SetupFont();
    void SetupMoonInfoPanel(const MoonInfo& info);
    void SetupStarAnimation();
    void SetupMoonImage(const MoonInfo& info);
    void SetupCustomChrome();

    wxPoint m_dragStartPos;
    std::map<wxString, wxBitmap> m_bitmaps;
    std::map<std::string, wxBitmap> m_moonPhaseBitmaps;
    wxBitmap m_scaledBackground;
    wxFont m_pixelFont;
};

class MyApp : public wxApp {
public:
    bool OnInit() override;
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    wxInitAllImageHandlers();
    auto* frame = new MyFrame("Moon Info");
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition,
              wxSize(FromDIP(AppConfig::FRAME_WIDTH), FromDIP(AppConfig::FRAME_HEIGHT)),
              wxFRAME_NO_TASKBAR | wxNO_BORDER) {
    SetMinClientSize(FromDIP(wxSize(AppConfig::FRAME_WIDTH, AppConfig::FRAME_HEIGHT)));
    LoadAssets();
    PrepareBackground();
    SetupUI();
    Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);
}

MyFrame::~MyFrame() {
    if (HasCapture()) {
        ReleaseMouse();
    }
}

void MyFrame::LoadAssets() {
    auto loadAndScaleBitmap = [&](const wxString& name, const wxString& path, int width, int height) {
        wxImage image;
        if (image.LoadFile(path, wxBITMAP_TYPE_PNG)) {
            image.Scale(FromDIP(width), FromDIP(height), wxIMAGE_QUALITY_NEAREST);
            m_bitmaps[name] = wxBitmap(image);
        } else {
            wxLogError("Failed to load image: %s", path);
        }
    };
    
    wxImage bgImage;
    if (bgImage.LoadFile("assets/background.png", wxBITMAP_TYPE_PNG)) {
        m_bitmaps["background"] = wxBitmap(bgImage);
    } else {
        wxLogError("Failed to load image: background.png");
    }

    loadAndScaleBitmap("exit", "assets/exit.png", AppConfig::EXIT_BUTTON_WIDTH, AppConfig::EXIT_BUTTON_HEIGHT);
    loadAndScaleBitmap("exit_hover", "assets/exit_hover.png", AppConfig::EXIT_BUTTON_WIDTH, AppConfig::EXIT_BUTTON_HEIGHT);

    auto loadMoonBitmap = [&](const std::string& phaseName, const wxString& path) {
        wxImage image;
        if (image.LoadFile(path, wxBITMAP_TYPE_PNG)) {
            m_moonPhaseBitmaps[phaseName] = wxBitmap(image);
        } else {
            wxLogError("Failed to load moon image: %s", path);
        }
    };
    
    loadMoonBitmap("First Quarter", "assets/first_quarter.png");
    loadMoonBitmap("Full", "assets/full_moon.png");
    loadMoonBitmap("Last Quarter", "assets/last_quarter.png");
    loadMoonBitmap("Waning Crescent", "assets/waning_crescent.png");
    loadMoonBitmap("Waxing Crescent", "assets/waxing_crescent.png");
    loadMoonBitmap("Waning Gibbous", "assets/waning_gibbous.png");
    loadMoonBitmap("Waxing Gibbous", "assets/waxing_gibbous.png");
}

void MyFrame::PrepareBackground() {
    if (m_bitmaps.count("background")) {
        wxImage tempImage = m_bitmaps["background"].ConvertToImage();
        m_scaledBackground = wxBitmap(tempImage);
    }
}

void MyFrame::SetupUI() {
    SetupFont();
    const MoonInfo moonInfo;
    SetupMoonInfoPanel(moonInfo);
    SetupStarAnimation();
    SetupMoonImage(moonInfo);
    SetupCustomChrome();
}

void MyFrame::SetupFont() {
    constexpr int FONT_SIZE = 20;
    if (m_pixelFont.AddPrivateFont("assets/PixelPurl.ttf")) {
        m_pixelFont.SetFaceName("PixelPurl");
        m_pixelFont.SetPointSize(FromDIP(FONT_SIZE));
    } else {
        wxLogError("Failed to load custom font! Falling back to default.");
        m_pixelFont = wxFont(wxFontInfo(FromDIP(FONT_SIZE)).Bold());
    }
}

void MyFrame::SetupMoonInfoPanel(const MoonInfo& info) {
    auto* panel = new wxPanel(this, wxID_ANY,
                              FromDIP(wxPoint(AppConfig::INFO_PANEL_X, AppConfig::INFO_PANEL_Y)),
                              FromDIP(wxSize(AppConfig::INFO_PANEL_WIDTH, AppConfig::INFO_PANEL_HEIGHT)));

    const long textStyle = wxST_NO_AUTORESIZE | wxBG_STYLE_TRANSPARENT;
    wxStaticText* illuminationText = new wxStaticText(panel, wxID_ANY, "Illumination: " + info.illumination + "%", wxDefaultPosition, wxDefaultSize, textStyle);
    wxStaticText* phaseText = new wxStaticText(panel, wxID_ANY, "Phase: " + info.phase, wxDefaultPosition, wxDefaultSize, textStyle);
    wxStaticText* moonriseText = new wxStaticText(panel, wxID_ANY, "Moonrise: " + info.riseTimeString, wxDefaultPosition, wxDefaultSize, textStyle);
    wxStaticText* moonsetText = new wxStaticText(panel, wxID_ANY, "Moonset: " + info.setTimeString, wxDefaultPosition, wxDefaultSize, textStyle);

    for (auto* textCtrl : {illuminationText, phaseText, moonriseText, moonsetText}) {
        textCtrl->SetFont(m_pixelFont);
        textCtrl->SetForegroundColour(*wxYELLOW);
    }

    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    const int verticalGap = FromDIP(8);
    contentSizer->Add(illuminationText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, verticalGap);
    contentSizer->Add(phaseText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, verticalGap);
    contentSizer->Add(moonriseText, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, verticalGap);
    contentSizer->Add(moonsetText, 0, wxALIGN_CENTER_HORIZONTAL);

    auto* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->AddStretchSpacer(1);
    panelSizer->Add(contentSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));
    panelSizer->AddStretchSpacer(1);
    panel->SetSizer(panelSizer);
    panel->Layout();
}

void MyFrame::SetupStarAnimation() {
    auto* starAnimation = new wxAnimationCtrl(this, wxID_ANY, wxNullAnimation,
                                              FromDIP(wxPoint(AppConfig::STAR_AREA_X, AppConfig::STAR_AREA_Y)),
                                              FromDIP(wxSize(AppConfig::STAR_AREA_WIDTH, AppConfig::STAR_AREA_HEIGHT)));
    wxAnimation animation;
    if (animation.LoadFile("assets/stars.gif", wxANIMATION_TYPE_GIF)) {
        starAnimation->SetAnimation(animation);
        starAnimation->Play();
    } else {
        wxLogError("Failed to load star animation!");
    }
}

void MyFrame::SetupMoonImage(const MoonInfo& info) {
    auto it = m_moonPhaseBitmaps.find(info.phase);
    if (it == m_moonPhaseBitmaps.end()) {
        wxLogMessage("No moon image for phase: %s", info.phase);
        return;
    }

    const wxBitmap& moonBitmap = it->second;

    constexpr int MOON_IMG_WIDTH = 200;
    constexpr int MOON_IMG_HEIGHT = 200;

    const int moonX = AppConfig::STAR_AREA_X + (AppConfig::STAR_AREA_WIDTH - MOON_IMG_WIDTH) / 2;
    const int moonY = AppConfig::STAR_AREA_Y + (AppConfig::STAR_AREA_HEIGHT - MOON_IMG_HEIGHT) / 2;

    new wxStaticBitmap(this, wxID_ANY, moonBitmap,
                       FromDIP(wxPoint(moonX, moonY)),
                       FromDIP(wxSize(MOON_IMG_WIDTH, MOON_IMG_HEIGHT)));
}

void MyFrame::SetupCustomChrome() {
    auto* dragAreaPanel = new wxPanel(this, wxID_ANY,
                                      FromDIP(wxPoint(0, 0)),
                                      FromDIP(wxSize(AppConfig::EXIT_BUTTON_X, AppConfig::DRAG_AREA_HEIGHT)));
    dragAreaPanel->Bind(wxEVT_LEFT_DOWN, &MyFrame::OnDragBegin, this);
    dragAreaPanel->Bind(wxEVT_MOTION, &MyFrame::OnDragMove, this);
    
    this->Bind(wxEVT_LEFT_UP, &MyFrame::OnDragEnd, this);

    auto* exitButton = new wxBitmapButton(this, wxID_EXIT, m_bitmaps["exit"],
                                          FromDIP(wxPoint(AppConfig::EXIT_BUTTON_X, AppConfig::EXIT_BUTTON_Y)),
                                          FromDIP(wxSize(AppConfig::EXIT_BUTTON_WIDTH, AppConfig::EXIT_BUTTON_HEIGHT)),
                                          wxBORDER_NONE | wxBU_EXACTFIT);

    if (m_bitmaps.count("exit_hover")) {
        exitButton->SetBitmapHover(m_bitmaps["exit_hover"]);
    }
    exitButton->Bind(wxEVT_BUTTON, &MyFrame::OnCloseButton, this);
}

void MyFrame::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    if (m_scaledBackground.IsOk()) {
        dc.DrawBitmap(m_scaledBackground, 0, 0, true);
    } else {
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawRectangle(GetClientRect());
    }
    event.Skip(); 
}

void MyFrame::OnDragBegin(wxMouseEvent& event) {
    CaptureMouse();
    m_dragStartPos = ClientToScreen(event.GetPosition()) - GetPosition();
}

void MyFrame::OnDragMove(wxMouseEvent& event) {
    if (HasCapture() && event.Dragging() && event.LeftIsDown()) {
        Move(ClientToScreen(event.GetPosition()) - m_dragStartPos);
    }
}

void MyFrame::OnDragEnd(wxMouseEvent& event) {
    if (HasCapture()) {
        ReleaseMouse();
    }
    event.Skip();
}

void MyFrame::OnCloseButton(wxCommandEvent& event) {
    Close(true);
}