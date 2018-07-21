
#ifndef ENUM_USAGE_H_
#define ENUM_USAGE_H_

void printUsage()
{
    Settings s;

    std::cout << "imageshrink [settings] inputFile outputFile"
              << std::endl;

    std::cout << std::endl;

    std::cout << "settings:"
              << std::endl;

    std::cout << "    --min value                 minimum jpeg quality "
              << "("
              << Settings::qualityMin_min
              << " <= value <= "
              << Settings::qualityMin_max
              <<  ", default = "
              << Settings::qualityMin_default
              << ")"
              << std::endl;

    std::cout << "    --max value                 maximum jpeg quality "
              << "("
              << Settings::qualityMax_min
              << " <= value <= "
              << Settings::qualityMax_max
              << ", default = "
              << Settings::qualityMax_default
              << ")"
              << std::endl;

    std::cout << "    --dssimAvgMax value         maximum for the average DSSIM "
              << "("
              << Settings::dssimAvgMax_min
              << " <= value <= "
              << Settings::dssimAvgMax_max
              << ", default = "
              << Settings::dssimAvgMax_default
              << ")"
              << std::endl;

    std::cout << "    --dssimPeakMax value        maximum for the peak DSSIM "
              << "("
              << Settings::dssimPeakMax_min
              << " <= value <= "
              << Settings::dssimPeakMax_max
              << ", default = "
              << Settings::dssimPeakMax_default
              << ")"
              << std::endl;

    std::cout << "    --copyMarkers value         maximum for the peak DSSIM "
              << "(value = none|all, default = "
              << s.copyMarkersAsString()
              << ")"
              << std::endl;

    std::cout << "    --initQualityStep value     init value for qulaity steps "
              << "("
              << Settings::initQualityStep_min
              << " <= value <= "
              << Settings::initQualityStep_max
              << ", default = "
              << Settings::initQualityStep_default
              << ")"
              << std::endl;

    std::cout << "    --cs444to420 value          convert cs444 to cs420 "
              << "(value = true|false, default = "
              << s.cs444to420AsString()
              << ")"
              << std::endl;

    std::cout << "    --imageCompChunkSize value  image chunk size for comparison "
              << "(" << Settings::imageCompChunkSize_min
              << " <= value <= "
              << Settings::imageCompChunkSize_max
              << ", default = "
              << Settings::imageCompChunkSize_default
              << ")"
              << std::endl;
}

#endif // ENUM_USAGE_H_
