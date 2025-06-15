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

int main() {
    
    MoonInfo moon;

    std::cout << "Phase: " << moon.phase << std::endl;
    std::cout << "Illumination: " << moon.illumination << std::endl;
    std::cout << "Moonrise: " << moon.riseTimeString << std::endl;
    std::cout << "Moonset: " << moon.setTimeString << std::endl;

    return 0;
}