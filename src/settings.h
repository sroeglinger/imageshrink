
#ifndef ENUM_SETTINGS_H_
#define ENUM_SETTINGS_H_

struct Settings
{
    Settings()
    : qualityMin( qualityMin_default )
    , qualityMax( qualityMax_default )
    , dssimAvgMax( dssimAvgMax_default )
    , dssimPeakMax( dssimPeakMax_default )
    , copyMarkers( copyMarkers_default )
    , initQualityStep( initQualityStep_default )
    , cs444to420( cs444to420_default )
    , imageCompChunkSize( imageCompChunkSize_default )
    , inputFile()
    , outputFile()
    {}

    // settings
    int              qualityMin;
    const static int qualityMin_min = 20;
    const static int qualityMin_max = 100;
    const static int qualityMin_default = 20;

    int              qualityMax;
    const static int qualityMax_min = 50;
    const static int qualityMax_max = 100;
    const static int qualityMax_default = 85;

    double                        dssimAvgMax;
    constexpr const static double dssimAvgMax_min = 0.0;
    constexpr const static double dssimAvgMax_max = 1.0;
    constexpr const static double dssimAvgMax_default = 0.0001;

    double                        dssimPeakMax;
    constexpr const static double dssimPeakMax_min = 0.0;
    constexpr const static double dssimPeakMax_max = 1.0;
    constexpr const static double dssimPeakMax_default = 0.001;

    bool              copyMarkers;
    const static bool copyMarkers_default = true;

    int              initQualityStep;
    const static int initQualityStep_min = 1;
    const static int initQualityStep_max = 10;
    const static int initQualityStep_default = 10;

    bool              cs444to420;
    const static bool cs444to420_default = true;

    int              imageCompChunkSize;
    const static int imageCompChunkSize_min = 8;
    const static int imageCompChunkSize_max = 256;
    const static int imageCompChunkSize_default = 20 * 8;

    std::string inputFile;
    std::string outputFile;

    // helper
    const char * copyMarkersAsString()
    {
        if (copyMarkers)
            return "all";
        else
            return "none";
    }

    const char * cs444to420AsString()
    {
        if (cs444to420)
            return "true";
        else
            return "false";
    }
};

#endif // ENUM_SETTINGS_H_

