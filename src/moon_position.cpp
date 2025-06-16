#include "json.hpp"
#include "httplib.h"
#include <ios>
#include <iostream>
#include <libnova/lunar.h>
#include <libnova/rise_set.h>
#include <libnova/ln_types.h>
#include <libnova/julian_day.h>
#include <ostream>
#include <string>
#include <utility>
#include <iomanip>
#include <sstream>
#include <wx/wx.h> // Include wxWidgets header

using json = nlohmann::json;

std::string militaryToStandard(const ln_zonedate time) {
    int displayHour = time.hours;
    std::string ampm = "am";
    
    if (displayHour >= 12) {
        ampm = "pm";
    }

    if (displayHour == 0) {
        displayHour = 12;
    } else if (displayHour > 12) {
        displayHour -= 12;
    }

    std::string formattedMinutes = std::to_string((time.minutes));
    if (time.minutes < 10) {
        formattedMinutes = "0" + formattedMinutes;
    }
    return std::to_string(displayHour) + ":" + formattedMinutes + ampm;
}

class MoonInfo {
public:
    std::string phase;
    std::string illumination;
    std::string riseTimeString;
    std::string setTimeString;

    MoonInfo() {
        double JD = ln_get_julian_from_sys();

        //Lunar Illumination
        double illuminationVal = ln_get_lunar_disk(JD);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (illuminationVal * 100.0);
        illumination = oss.str();

        //Lunar Phase
        bool waxing = ln_get_lunar_disk(JD + 0.0020833333) > ln_get_lunar_disk(JD);

        const double NEW_MOON = 0.02;
        const double CRESCENT = 0.49;
        const double QUARTER = 0.51;
        const double GIBBOUS = 0.97;

        phase = "";
        if (illuminationVal < NEW_MOON) {
            phase = "New";
        } else if (illuminationVal < CRESCENT) {
            phase = waxing ? "Waxing Crescent" : "Waning Crescent";
        } else if (illuminationVal < QUARTER) {
            phase = waxing ? "First Quarter" : "Last Quarter";
        } else if (illuminationVal < GIBBOUS) {
            phase = waxing ? "Waxing Gibbous" : "Waning Gibbous";
        } else {
            phase = "Full";
        }

        //Moonrise/Moonset
        std::pair<double, double> ip_coords = getCoordinates();
        struct ln_lnlat_posn coords;
        coords.lng = ip_coords.first;
        coords.lat = ip_coords.second;

        struct ln_rst_time rst;
        int rst_status = ln_get_lunar_rst(JD, &coords, &rst);

        struct ln_zonedate localRise;
        ln_get_local_date(rst.rise, &localRise);
        struct ln_zonedate localSet;
        ln_get_local_date(rst.set, &localSet);

        if (rst_status == 0) {
            riseTimeString = militaryToStandard(localRise);
            setTimeString = militaryToStandard(localSet);
        } else {
            riseTimeString = "N/A";
            setTimeString = "N/A";
        }
    }
private:
    std::pair<double, double> getCoordinates() {
        
        httplib::Client cli("http://ip-api.com");
        auto res = cli.Get("/json/");
        json json_data = json::parse(res->body);

        return std::make_pair(json_data["lon"].get<double>(), json_data["lat"].get<double>());
    }
};

// --- wxWidgets GUI Implementation ---

// 1. Define your Application class
class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

// 2. Define your Main Frame class
class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
    // Pointers to wxStaticText controls
    wxStaticText* m_phaseText;
    wxStaticText* m_illuminationText;
    wxStaticText* m_moonriseText;
    wxStaticText* m_moonsetText;
};

// 3. Implement OnInit for your Application
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame *frame = new MyFrame("Moon Info", wxPoint(50, 50), wxSize(300, 200));
    frame->Show(true);
    return true;
}

// 4. Implement the Constructor for your Main Frame
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    // Create a panel to hold the controls
    wxPanel* panel = new wxPanel(this, wxID_ANY);

    // Use a sizer for automatic layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Get moon information
    MoonInfo moon;

    // Create wxStaticText controls and add them to the sizer
    m_phaseText = new wxStaticText(panel, wxID_ANY, "Phase: " + moon.phase);
    sizer->Add(m_phaseText, 0, wxALL | wxEXPAND, 5); // 0 for not stretchable, wxALL for padding, wxEXPAND to fill width

    m_illuminationText = new wxStaticText(panel, wxID_ANY, "Illumination: " + moon.illumination + "%");
    sizer->Add(m_illuminationText, 0, wxALL | wxEXPAND, 5);

    m_moonriseText = new wxStaticText(panel, wxID_ANY, "Moonrise: " + moon.riseTimeString);
    sizer->Add(m_moonriseText, 0, wxALL | wxEXPAND, 5);

    m_moonsetText = new wxStaticText(panel, wxID_ANY, "Moonset: " + moon.setTimeString);
    sizer->Add(m_moonsetText, 0, wxALL | wxEXPAND, 5);

    // Set the sizer for the panel
    panel->SetSizer(sizer);
    sizer->Fit(this); // Adjust frame size to fit content
}