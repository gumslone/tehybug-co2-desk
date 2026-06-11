
#ifndef ENVIRONMENTAL_CALCULATIONS_H
#define ENVIRONMENTAL_CALCULATIONS_H

#include <Arduino.h>
#include <math.h>

/**
 * @class EnvironmentalCalculations
 * @brief Provides environmental comfort and air quality calculations
 * 
 * This class calculates derived environmental metrics including:
 * - Dew point temperature
 * - Heat index
 * - Indoor Air Quality (IAQ) score
 * - Comfort level index
 */
class EnvironmentalCalculations {
public:
    /**
     * @brief Calculate dew point temperature using Magnus formula
     * @param temperature Temperature in Celsius
     * @param humidity Relative humidity in percentage (0-100)
     * @return Dew point in Celsius, or NAN if inputs are invalid
     */
    static double calculateDewPoint(double temperature, double humidity) {
        if (temperature <= -273.15 || humidity <= 0 || humidity > 100) {
            return NAN;
        }
        
        const double a = 17.27;
        const double b = 237.7;
        double alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0);
        double dewPoint = (b * alpha) / (a - alpha);
        
        return dewPoint;
    }
    
    /**
     * @brief Calculate heat index using Steadman's formula
     * @param temperature Temperature in Celsius
     * @param humidity Relative humidity in percentage (0-100)
     * @return Heat index in Celsius, or NAN if conditions don't warrant calculation
     * @note Only calculated when temp >= 27°C and humidity >= 40%
     */
    static double calculateHeatIndex(double temperature, double humidity) {
        // Heat index is only meaningful in hot, humid conditions
        if (temperature < 27.0 || humidity < 40.0) {
            return NAN;
        }
        
        double T = temperature;
        double RH = humidity;
        
        // Steadman's formula (simplified version)
        double HI = -8.78469475556 + 
                    1.61139411 * T + 
                    2.33854883889 * RH + 
                    -0.14611605 * T * RH + 
                    -0.012308094 * T * T + 
                    -0.0164248277778 * RH * RH + 
                    0.002211732 * T * T * RH + 
                    0.00072546 * T * RH * RH + 
                    -0.000003582 * T * T * RH * RH;
        
        return HI;
    }
    
    /**
     * @brief Calculate Indoor Air Quality score
     * @param co2 CO2 concentration in ppm
     * @param pm25 PM2.5 concentration in µg/m³
     * @param temperature Temperature in Celsius
     * @param humidity Relative humidity in percentage (0-100)
     * @return IAQ score from 0 (poor) to 100 (excellent)
     * 
     * The IAQ score considers:
     * - CO2 levels (0-50 points impact)
     * - PM2.5 levels (0-30 points impact)
     * - Temperature comfort (0-10 points impact)
     * - Humidity comfort (0-10 points impact)
     */
    static double calculateIAQ(double co2, double pm25, double temperature, double humidity) {
        double iaqScore = 100.0; // Start with perfect score
        
        // CO2 impact (0-50 points penalty)
        if (co2 > 0) {
            if (co2 <= 600) {
                // Excellent
                iaqScore -= 0;
            } else if (co2 <= 800) {
                // Good
                iaqScore -= (co2 - 600) / 200.0 * 10.0;
            } else if (co2 <= 1000) {
                // Moderate
                iaqScore -= 10 + (co2 - 800) / 200.0 * 15.0;
            } else if (co2 <= 1500) {
                // Poor
                iaqScore -= 25 + (co2 - 1000) / 500.0 * 15.0;
            } else {
                // Very poor
                iaqScore -= 40 + min(10.0, (co2 - 1500) / 500.0 * 10.0);
            }
        }
        
        // PM2.5 impact (0-30 points penalty)
        if (pm25 > 0) {
            if (pm25 <= 12) {
                // Good
                iaqScore -= 0;
            } else if (pm25 <= 35) {
                // Moderate
                iaqScore -= (pm25 - 12) / 23.0 * 10.0;
            } else if (pm25 <= 55) {
                // Unhealthy for sensitive groups
                iaqScore -= 10 + (pm25 - 35) / 20.0 * 10.0;
            } else {
                // Unhealthy
                iaqScore -= 20 + min(10.0, (pm25 - 55) / 45.0 * 10.0);
            }
        }
        
        // Temperature comfort impact (0-10 points penalty)
        if (temperature > -273.15) {
            if (temperature < 18 || temperature > 26) {
                double tempDiff = (temperature < 18) ? (18 - temperature) : (temperature - 26);
                iaqScore -= min(10.0, tempDiff * 2.0);
            }
        }
        
        // Humidity comfort impact (0-10 points penalty)
        if (humidity > 0 && humidity <= 100) {
            if (humidity < 30 || humidity > 60) {
                double humiDiff = (humidity < 30) ? (30 - humidity) : (humidity - 60);
                iaqScore -= min(10.0, humiDiff / 4.0);
            }
        }
        
        // Ensure score is within 0-100 range
        return max(0.0, min(100.0, iaqScore));
    }
    
    /**
     * @brief Convert Celsius to Fahrenheit
     * @param celsius Temperature in Celsius
     * @return Temperature in Fahrenheit
     */
    static double celsiusToFahrenheit(double celsius) {
        return celsius * 9.0 / 5.0 + 32.0;
    }
    
};

#endif // ENVIRONMENTAL_CALCULATIONS_H