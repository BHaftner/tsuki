#define _USE_MATH_DEFINES

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <limits>

constexpr double PI = M_PI;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / PI;
constexpr double JD_2000_0 = 2451545.0;
constexpr double EARTH_RADIUS_KM = 6378.137;
constexpr double HORIZON_ALT_DEG = -0.566;

namespace {

double degreesToRadians(double deg) {
    return deg * DEG_TO_RAD;
}

double radiansToDegrees(double rad) {
    return rad * RAD_TO_DEG;
}

double normalizeDegrees(double angle) {
    double result = fmod(angle, 360.0);
    if (result < 0) {
        result += 360.0;
    }
    return result;
}

double normalizeRadians(double angle) {
    double result = fmod(angle, 2.0 * PI);
    if (result < 0) {
        result += 2.0 * PI;
    }
    return result;
}

std::tm getUtcTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm;

#ifdef _WIN32
    gmtime_s(&utc_tm, &now_c);
#else
    gmtime_r(&now_c, &utc_tm);
#endif
    return utc_tm;
}

double getJulianDay(const std::tm& utc_tm) {
    int year = utc_tm.tm_year + 1900;
    int month = utc_tm.tm_mon + 1;
    double day = utc_tm.tm_mday + utc_tm.tm_hour / 24.0 + utc_tm.tm_min / 1440.0 + utc_tm.tm_sec / 86400.0;

    if (month <= 2) {
        year--;
        month += 12;
    }

    double A = floor(year / 100.0);
    double B = 2 - A + floor(A / 4.0);

    return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

std::string militaryToStandard(const std::tm& local_tm) {
    std::stringstream ss;
    int hour = local_tm.tm_hour;
    int min = local_tm.tm_min;
    std::string ampm = (hour >= 12) ? "PM" : "AM";

    if (hour == 0 || hour == 24) {
        hour = 12;
    } else if (hour > 12) {
        hour -= 12;
    }

    ss << std::setfill('0') << std::setw(2) << hour << ":"
       << std::setfill('0') << std::setw(2) << min << " " << ampm;
    return ss.str();
}

std::tm convertJdUtcToLocalTm(double JD_utc) {
    double seconds_since_epoch = (JD_utc - 2440587.5) * 86400.0;
    std::time_t tt_utc = static_cast<std::time_t>(seconds_since_epoch);

    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &tt_utc);
#else
    localtime_r(&tt_utc, &local_tm);
#endif
    return local_tm;
}

}

struct SolarCoords {
    double eclipticLongitude;
    double radiusVector;
};

struct LunarCoords {
    double eclipticLongitude;
    double eclipticLatitude;
    double radiusVector;
};

class MoonInfo {
public:
    std::string phase;
    std::string illumination;
    std::string riseTimeString;
    std::string setTimeString;

    MoonInfo(double lat, double lng) {
        std::tm utc_tm = getUtcTime();
        const double current_julian_day_utc = getJulianDay(utc_tm);

        double observer_latitude = lat;
        double observer_longitude = lng;

        calculatePhaseAndIllumination(current_julian_day_utc);
        calculateRiseAndSetTimes(current_julian_day_utc, observer_longitude, observer_latitude);
    }

private:
    double getGMST(double JD) {
        double T = (JD - JD_2000_0) / 36525.0;

        double GMST_deg = 280.46061837 + 360.98564736629 * (JD - JD_2000_0) +
                          0.000387933 * T * T - T * T * T / 38710000.0;

        return normalizeDegrees(GMST_deg);
    }

    double getObliquityAndNutation(double JD, double& delta_psi, double& delta_epsilon) {
        double T = (JD - JD_2000_0) / 36525.0;

        double epsilon0_arcsec = 84381.448 - 46.8150 * T - 0.00059 * T * T + 0.001813 * T * T * T;
        double epsilon0_deg = epsilon0_arcsec / 3600.0;

        double L_prime = normalizeDegrees(218.3164477 + 481267.88123421 * T);
        double M = normalizeDegrees(357.5291092 + 35999.05034 * T);
        double F = normalizeDegrees(93.2720950 + 483202.0175 * T);
        double Omega = normalizeDegrees(125.04452 - 1934.13626 * T);

        L_prime = degreesToRadians(L_prime);
        M = degreesToRadians(M);
        F = degreesToRadians(F);
        Omega = degreesToRadians(Omega);

        delta_psi = (-17.200 * sin(Omega) - 1.319 * sin(2 * L_prime) - 0.227 * sin(2 * F) + 0.206 * sin(2 * Omega)) / 3600.0; // in degrees
        delta_epsilon = (9.202 * cos(Omega) + 0.573 * cos(2 * L_prime) + 0.098 * cos(2 * F) - 0.090 * cos(2 * Omega)) / 3600.0; // in degrees

        return epsilon0_deg;
    }

    SolarCoords getSolarCoordinates(double JD) {
        double D = JD - JD_2000_0;

        double M_sun_deg = normalizeDegrees(357.5291092 + 0.985600283 * D);
        double M_sun_rad = degreesToRadians(M_sun_deg);

        double C_sun_deg = 1.9148 * sin(M_sun_rad) + 0.0200 * sin(2 * M_sun_rad) + 0.0003 * sin(3 * M_sun_rad);

        double L0_sun_deg = normalizeDegrees(280.46646 + 0.98564736 * D);

        double lambda_sun = normalizeDegrees(L0_sun_deg + C_sun_deg);

        double R_sun_AU = 1.00014 - 0.01671 * cos(M_sun_rad) - 0.00014 * cos(2 * M_sun_rad);

        return {lambda_sun, R_sun_AU};
    }

    LunarCoords getLunarCoordinates(double JD) {
        double D_days_from_J2000 = JD - JD_2000_0;
        double L_moon = normalizeDegrees(218.3164477 + 13.17639647 * D_days_from_J2000);
        double M_moon = normalizeDegrees(134.9634114 + 13.06499295 * D_days_from_J2000);
        double M_sun = normalizeDegrees(357.5291092 + 0.985600283 * D_days_from_J2000);
        double F = normalizeDegrees(93.2720950 + 13.22935035 * D_days_from_J2000);
        double L_sun_mean = normalizeDegrees(280.46646 + 0.98564736 * D_days_from_J2000);
        double D_angle = normalizeDegrees(L_moon - L_sun_mean);
        double M_moon_rad = degreesToRadians(M_moon);
        double M_sun_rad = degreesToRadians(M_sun);
        double F_rad = degreesToRadians(F);
        double D_angle_rad = degreesToRadians(D_angle);
        double sum_lon = 0.0;
        sum_lon += 6.28875 * sin(M_moon_rad);
        sum_lon += 1.27401 * sin(2 * D_angle_rad);
        sum_lon += 0.65831 * sin(2 * F_rad);
        sum_lon -= 0.18581 * sin(M_sun_rad);
        sum_lon -= 0.11433 * sin(2 * F_rad + M_moon_rad);
        sum_lon += 0.05877 * sin(2 * D_angle_rad - M_moon_rad);
        sum_lon += 0.05730 * sin(2 * D_angle_rad + M_moon_rad);
        sum_lon += 0.05322 * sin(2 * F_rad + M_sun_rad);
        sum_lon += 0.04620 * sin(2 * F_rad - M_moon_rad);
        sum_lon += 0.04092 * sin(2 * D_angle_rad - M_sun_rad);
        sum_lon += 0.03044 * sin(M_moon_rad + M_sun_rad);
        sum_lon += 0.01526 * sin(2 * D_angle_rad - 2 * F_rad);
        sum_lon += 0.01130 * sin(M_moon_rad - M_sun_rad);

        sum_lon += 0.01024 * sin(2 * F_rad - M_sun_rad);
        sum_lon -= 0.00914 * sin(2 * D_angle_rad + M_sun_rad);
        sum_lon += 0.00422 * sin(2 * D_angle_rad + 2 * F_rad);
        sum_lon += 0.00386 * sin(2 * D_angle_rad - 3 * F_rad);
        sum_lon += 0.00366 * sin(3 * M_moon_rad);
        sum_lon += 0.00293 * sin(2 * M_sun_rad);
        sum_lon += 0.00276 * sin(2 * M_moon_rad - 2 * D_angle_rad);
        sum_lon += 0.00252 * sin(2 * M_moon_rad + 2 * D_angle_rad);
        sum_lon += 0.00224 * sin(2 * D_angle_rad + M_moon_rad - M_sun_rad);

        double lambda_moon = normalizeDegrees(L_moon + sum_lon);

        double sum_lat = 0.0;
        sum_lat += 5.12819 * sin(F_rad);
        sum_lat += 0.28060 * sin(M_moon_rad + F_rad);
        sum_lat += 0.27769 * sin(F_rad - M_moon_rad);
        sum_lat += 0.17320 * sin(M_sun_rad + F_rad);
        sum_lat += 0.05538 * sin(2 * D_angle_rad + F_rad);
        sum_lat += 0.04627 * sin(2 * D_angle_rad - F_rad);
        sum_lat += 0.03257 * sin(2 * D_angle_rad - M_moon_rad + F_rad);

        sum_lat += 0.01633 * sin(M_sun_rad + 2 * D_angle_rad - F_rad);
        sum_lat += 0.00809 * sin(M_moon_rad + 2 * D_angle_rad + F_rad);
        sum_lat += 0.00769 * sin(2 * M_moon_rad + F_rad);
        sum_lat += 0.00755 * sin(2 * D_angle_rad + 2 * F_rad - M_moon_rad);
        sum_lat += 0.00705 * sin(2 * D_angle_rad + M_sun_rad + F_rad);
        sum_lat += 0.00583 * sin(2 * D_angle_rad - M_sun_rad + F_rad);
        sum_lat += 0.00517 * sin(2 * D_angle_rad + 2 * F_rad);
        sum_lat += 0.00412 * sin(M_sun_rad + 2 * D_angle_rad - 2 * F_rad);
        sum_lat += 0.00388 * sin(2 * D_angle_rad + M_moon_rad + 2 * F_rad);
        sum_lat += 0.00277 * sin(2 * D_angle_rad + 2 * F_rad + M_moon_rad);

        double beta_moon = sum_lat;
        double R_moon = 385000.0;
        R_moon -= 20905.0 * cos(M_moon_rad);
        R_moon -= 3699.0 * cos(2 * D_angle_rad - M_moon_rad);
        R_moon -= 2956.0 * cos(2 * D_angle_rad);
        R_moon -= 569.0 * cos(2 * F_rad);
        R_moon += 246.0 * cos(2 * D_angle_rad - 2 * F_rad);
        R_moon += 209.0 * cos(M_moon_rad + M_sun_rad);
        R_moon += 105.0 * cos(M_sun_rad);
        R_moon -= 103.0 * cos(M_moon_rad - M_sun_rad);
        R_moon -= 57.0 * cos(M_moon_rad + 2 * D_angle_rad);
        R_moon -= 48.0 * cos(M_moon_rad + 2 * F_rad);
        R_moon += 46.0 * cos(2 * D_angle_rad - M_sun_rad - M_moon_rad);
        R_moon += 38.0 * cos(2 * D_angle_rad + M_moon_rad);
        R_moon -= 30.0 * cos(M_moon_rad + F_rad + 2 * D_angle_rad);
        R_moon -= 24.0 * cos(M_moon_rad - 2 * D_angle_rad);
        R_moon -= 22.0 * cos(2 * D_angle_rad - F_rad);
        R_moon += 15.0 * cos(M_moon_rad - 2 * F_rad);
        R_moon -= 13.0 * cos(M_moon_rad + 2 * D_angle_rad + F_rad);
        R_moon -= 12.0 * cos(M_sun_rad + 2 * D_angle_rad);
        R_moon += 10.0 * cos(M_sun_rad - 2 * F_rad);
        R_moon += 8.0 * cos(2 * D_angle_rad + M_sun_rad + F_rad);
        R_moon += 7.0 * cos(M_moon_rad + F_rad);
        R_moon -= 6.0 * cos(2 * D_angle_rad + M_sun_rad - F_rad);
        R_moon -= 5.0 * cos(M_moon_rad + 2 * F_rad - 2 * D_angle_rad);
        R_moon -= 4.0 * cos(M_moon_rad - F_rad + 2 * D_angle_rad);
        R_moon += 4.0 * cos(M_moon_rad + M_sun_rad + 2 * D_angle_rad);
        R_moon -= 4.0 * cos(2 * D_angle_rad - 2 * M_sun_rad);
        R_moon -= 3.0 * cos(M_moon_rad - M_sun_rad - 2 * D_angle_rad);
        R_moon -= 3.0 * cos(M_sun_rad - F_rad);
        R_moon -= 3.0 * cos(2 * F_rad + 2 * D_angle_rad);
        R_moon += 3.0 * cos(M_moon_rad + M_sun_rad - F_rad);
        R_moon -= 3.0 * cos(2 * F_rad + M_sun_rad);
        R_moon -= 3.0 * cos(2 * D_angle_rad - M_moon_rad - M_sun_rad);
        R_moon += 3.0 * cos(M_moon_rad - M_sun_rad + 2 * D_angle_rad);
        R_moon += 3.0 * cos(M_moon_rad + M_sun_rad + F_rad);
        R_moon -= 3.0 * cos(M_moon_rad + F_rad - 2 * D_angle_rad);
        R_moon -= 2.0 * cos(2 * D_angle_rad - M_moon_rad - F_rad);

        return {lambda_moon, beta_moon, R_moon};
    }

    void calculatePhaseAndIllumination(double JD) {
        SolarCoords sun = getSolarCoordinates(JD);
        LunarCoords moon = getLunarCoordinates(JD);

        double g_rad = acos(-cos(degreesToRadians(moon.eclipticLatitude)) * cos(degreesToRadians(moon.eclipticLongitude - sun.eclipticLongitude)));

        double illum_fraction = (1.0 + cos(g_rad)) / 2.0;

        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (illum_fraction * 100.0);
        illumination = ss.str();

        const double LUNAR_PHASE_TIME_DELTA = 3.0 / (24.0 * 60.0);

        SolarCoords sun_future = getSolarCoordinates(JD + LUNAR_PHASE_TIME_DELTA);
        LunarCoords moon_future = getLunarCoordinates(JD + LUNAR_PHASE_TIME_DELTA);
        double g_rad_future = acos(-cos(degreesToRadians(moon_future.eclipticLatitude)) * cos(degreesToRadians(moon_future.eclipticLongitude - sun_future.eclipticLongitude)));
        double illum_fraction_future = (1.0 + cos(g_rad_future)) / 2.0;

        bool is_waxing_heuristic = (illum_fraction_future > illum_fraction);

        constexpr double NEW_MOON_MAX = 0.01;
        constexpr double QUARTER_MIN = 0.49;
        constexpr double QUARTER_MAX = 0.51; 
        constexpr double GIBBOUS_MIN = 0.99;

        if (illum_fraction < NEW_MOON_MAX) {
            phase = "New Moon";
        } else if (illum_fraction < QUARTER_MIN) {
            phase = is_waxing_heuristic ? "Waxing Crescent" : "Waning Crescent";
        } else if (illum_fraction >= QUARTER_MIN && illum_fraction <= QUARTER_MAX) {
            phase = is_waxing_heuristic ? "First Quarter" : "Last Quarter";
        } else if (illum_fraction < GIBBOUS_MIN) {
            phase = is_waxing_heuristic ? "Waxing Gibbous" : "Waning Gibbous";
        } else {
            phase = "Full Moon";
        }
    }

    double calculateAltitude(double JD_utc, double longitude_deg, double latitude_deg) {
        LunarCoords moon = getLunarCoordinates(JD_utc);

        double delta_psi, delta_epsilon;
        double mean_obliquity_deg = getObliquityAndNutation(JD_utc, delta_psi, delta_epsilon);

        double true_obliquity_rad = degreesToRadians(mean_obliquity_deg + delta_epsilon);

        double true_ecl_lon_moon_rad = degreesToRadians(moon.eclipticLongitude + delta_psi);
        double true_ecl_lat_moon_rad = degreesToRadians(moon.eclipticLatitude);

        double geocentric_ra_rad = normalizeRadians(atan2(sin(true_ecl_lon_moon_rad) * cos(true_obliquity_rad) - tan(true_ecl_lat_moon_rad) * sin(true_obliquity_rad),
                                                          cos(true_ecl_lon_moon_rad)));
        double geocentric_dec_rad = asin(sin(true_ecl_lat_moon_rad) * cos(true_obliquity_rad) +
                                         cos(true_ecl_lat_moon_rad) * sin(true_obliquity_rad) * sin(true_ecl_lon_moon_rad));

        double lat_rad = degreesToRadians(latitude_deg);
        double lon_rad = degreesToRadians(longitude_deg);

        double gmst_rad = degreesToRadians(getGMST(JD_utc));

        double lst_rad = normalizeRadians(gmst_rad + lon_rad);

        double lha_geocentric_rad = normalizeRadians(lst_rad - geocentric_ra_rad);

        double sin_phi_prime = sin(lat_rad);
        double cos_phi_prime = cos(lat_rad);

        double horizontal_parallax_rad = asin(EARTH_RADIUS_KM / moon.radiusVector);

        double delta_alpha_rad = atan2(-cos_phi_prime * sin(horizontal_parallax_rad) * sin(lha_geocentric_rad),
                                       cos(geocentric_dec_rad) - cos_phi_prime * sin(horizontal_parallax_rad) * cos(lha_geocentric_rad));

        double topocentric_ra_rad = geocentric_ra_rad + delta_alpha_rad;

        double topocentric_dec_rad = atan2(sin(geocentric_dec_rad) - sin_phi_prime * sin(horizontal_parallax_rad),
                                          (cos(geocentric_dec_rad) - cos_phi_prime * sin(horizontal_parallax_rad) * cos(lha_geocentric_rad)) * cos(delta_alpha_rad));

        double lha_topocentric_rad = normalizeRadians(lst_rad - topocentric_ra_rad);

        double sin_h = sin(topocentric_dec_rad) * sin_phi_prime +
                       cos(topocentric_dec_rad) * cos_phi_prime * cos(lha_topocentric_rad);
        double altitude_rad = asin(sin_h);

        return radiansToDegrees(altitude_rad);
    }

    double refineRiseSetTime(double JD_interval_start, double JD_interval_end, double longitude_deg, double latitude_deg, double target_alt_deg) {
        const double TOLERANCE_JD = 1.0 / (24.0 * 60.0 * 60.0);
        double mid_JD;
        double alt_at_mid;

        for (int i = 0; i < 100; ++i) {
            if (std::abs(JD_interval_end - JD_interval_start) < TOLERANCE_JD) {
                break;
            }

            mid_JD = (JD_interval_start + JD_interval_end) / 2.0;
            alt_at_mid = calculateAltitude(mid_JD, longitude_deg, latitude_deg);

            double diff_start = calculateAltitude(JD_interval_start, longitude_deg, latitude_deg) - target_alt_deg;
            double diff_mid = alt_at_mid - target_alt_deg;

            if (diff_start * diff_mid < 0) {
                JD_interval_end = mid_JD;
            } else {
                JD_interval_start = mid_JD;
            }
        }
        return (JD_interval_start + JD_interval_end) / 2.0;
    }

    double getLocalMidnightJD(double JD_utc_approx) {
        std::time_t tt_utc_now = static_cast<std::time_t>((JD_utc_approx - 2440587.5) * 86400.0);

        std::tm local_tm_now;
#ifdef _WIN32
        localtime_s(&local_tm_now, &tt_utc_now);
#else
        localtime_r(&tt_utc_now, &local_tm_now);
#endif

        local_tm_now.tm_hour = 0;
        local_tm_now.tm_min = 0;
        local_tm_now.tm_sec = 0;
        local_tm_now.tm_isdst = -1;

        std::time_t local_midnight_tt = mktime(&local_tm_now);
        if (local_midnight_tt == (std::time_t)-1) {
            std::cerr << "Error: mktime failed to convert local midnight time." << std::endl;
            return JD_utc_approx;
        }

        std::tm utc_tm_midnight;
#ifdef _WIN32
        gmtime_s(&utc_tm_midnight, &local_midnight_tt);
#else
        gmtime_r(&local_midnight_tt, &utc_tm_midnight);
#endif

        return getJulianDay(utc_tm_midnight);
    }

    void calculateRiseAndSetTimes(double JD_utc_now, double longitude_deg, double latitude_deg) {
        riseTimeString = "N/A";
        setTimeString = "N/A";

        const double SEARCH_START_JD = JD_utc_now - 1.0;
        const double SEARCH_END_JD = JD_utc_now + 1.0;

        const double STEP_JD = 5.0 / (24.0 * 60.0);

        std::vector<double> rise_JDs;
        std::vector<double> set_JDs;

        double prev_alt = calculateAltitude(SEARCH_START_JD, longitude_deg, latitude_deg);
        double prev_JD = SEARCH_START_JD;

        for (double current_JD_iter = SEARCH_START_JD + STEP_JD; current_JD_iter <= SEARCH_END_JD; current_JD_iter += STEP_JD) {
            double current_alt = calculateAltitude(current_JD_iter, longitude_deg, latitude_deg);

            if (prev_alt < HORIZON_ALT_DEG && current_alt >= HORIZON_ALT_DEG) {
                double refined_JD = refineRiseSetTime(prev_JD, current_JD_iter, longitude_deg, latitude_deg, HORIZON_ALT_DEG);
                rise_JDs.push_back(refined_JD);
            }
            else if (prev_alt > HORIZON_ALT_DEG && current_alt <= HORIZON_ALT_DEG) {
                double refined_JD = refineRiseSetTime(prev_JD, current_JD_iter, longitude_deg, latitude_deg, HORIZON_ALT_DEG);
                set_JDs.push_back(refined_JD);
            }

            prev_alt = current_alt;
            prev_JD = current_JD_iter;
        }

        double local_midnight_today_jd = getLocalMidnightJD(JD_utc_now);
        double local_midnight_tomorrow_jd = local_midnight_today_jd + 1.0;

        double best_rise_jd = std::numeric_limits<double>::infinity();
        bool rise_found = false;
        for (double jd : rise_JDs) {
            if (jd >= local_midnight_today_jd && jd < local_midnight_tomorrow_jd) {
                if (!rise_found || jd < best_rise_jd) {
                    best_rise_jd = jd;
                    rise_found = true;
                }
            }
        }

        double best_set_jd = std::numeric_limits<double>::infinity();
        bool set_found = false;
        for (double jd : set_JDs) {
            if (jd >= local_midnight_today_jd && jd < local_midnight_tomorrow_jd) {
                if (!set_found || jd < best_set_jd) {
                    best_set_jd = jd;
                    set_found = true;
                }
            }
        }

        if (rise_found) {
            std::tm rise_tm = convertJdUtcToLocalTm(best_rise_jd);
            riseTimeString = militaryToStandard(rise_tm);
        } else {
            double alt_at_local_midnight = calculateAltitude(local_midnight_today_jd, longitude_deg, latitude_deg);
            double alt_at_next_local_midnight_minus_epsilon = calculateAltitude(local_midnight_tomorrow_jd - 0.0001, longitude_deg, latitude_deg);
            if (alt_at_local_midnight > HORIZON_ALT_DEG && alt_at_next_local_midnight_minus_epsilon > HORIZON_ALT_DEG) {
                riseTimeString = "Always Above Horizon";
            } else if (alt_at_local_midnight < HORIZON_ALT_DEG && alt_at_next_local_midnight_minus_epsilon < HORIZON_ALT_DEG) {
                riseTimeString = "Always Below Horizon";
            }
        }

        if (set_found) {
            std::tm set_tm = convertJdUtcToLocalTm(best_set_jd);
            setTimeString = militaryToStandard(set_tm);
        } else {
            double alt_at_local_midnight = calculateAltitude(local_midnight_today_jd, longitude_deg, latitude_deg);
            double alt_at_next_local_midnight_minus_epsilon = calculateAltitude(local_midnight_tomorrow_jd - 0.0001, longitude_deg, latitude_deg);
            if (alt_at_local_midnight > HORIZON_ALT_DEG && alt_at_next_local_midnight_minus_epsilon > HORIZON_ALT_DEG) {
                setTimeString = "Always Above Horizon";
            } else if (alt_at_local_midnight < HORIZON_ALT_DEG && alt_at_next_local_midnight_minus_epsilon < HORIZON_ALT_DEG) {
                setTimeString = "Always Below Horizon";
            }
        }
    }
};