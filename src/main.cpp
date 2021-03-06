
#include <stdlib.h>
#include <iostream>
#include <string>
#include <unordered_map>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef USE_LOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/consoleappender.h>
#endif //USE_LOG4CXX

#include "ImageDummy.h"
#include "ImageJfif.h"
#include "ImageAverage.h"
#include "ImageVariance.h"
#include "ImageCollection.h"
#include "ImageDSSIM.h"
#include "settings.h"
#include "usage.h"

// Define static logger variable
#ifdef USE_LOG4CXX
log4cxx::LoggerPtr loggerMain           ( log4cxx::Logger::getLogger( "main" ) );
log4cxx::LoggerPtr loggerImage          ( log4cxx::Logger::getLogger( "image" ) );
log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );
#endif //USE_LOG4CXX

struct ImageComparisonResult
{
    ImageComparisonResult()
    : dssimAvg( 0.0 )
    , dssimPeak( 0.0 )
    {}

    double dssimAvg;
    double dssimPeak;
};


int main( int argc, const char* argv[] )
{
    // Initialize variables
#ifdef USE_LOG4CXX
    log4cxx::AppenderPtr defaultAppender = nullptr;
    log4cxx::LayoutPtr   defaultLayout   = nullptr;
#endif //USE_LOG4CXX

    Settings settings;

    bool error = false;

    // modify environment variables
    setenv( "TJ_OPTIMIZE", "1", 1 );  // enforce optimizition of the huffman table
//    setenv( "TJ_PROGRESSIVE", "1", 1);  // enables progressive encoding --> increases file size

    // parse arguments
    {
        int pos = 1;

        if( argc < 2 )
        {
            printUsage();
            return EXIT_FAILURE;
        }

        while(    ( pos < argc )
               && ( !error )
             )
        {
            const int nofRemainigArgs = argc - pos;
            const std::string arg( argv[pos] );
            bool somethingDone = false;
            pos++;

            /* START debug */
            // std::cout << "pos: "
            //           << pos
            //           << " arg: "
            //           << arg.c_str()
            //           << std::endl;
            /* END debug */

            if( nofRemainigArgs == 1 )
            {
                settings.outputFile = arg;
                somethingDone = true;
            }
            else if( nofRemainigArgs == 2 )
            {
                settings.inputFile = arg;
                somethingDone = true;
            }
            else if( nofRemainigArgs > 2 )
            {
                if( arg == "--min" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.qualityMin = std::stoi( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.qualityMin < Settings::qualityMin_min )
                        || ( settings.qualityMin > Settings::qualityMin_max )
                      )
                    {
                        error = true;
                    }

                    somethingDone = true;
                }
                else if( arg == "--max" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.qualityMax = std::stoi( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.qualityMax < Settings::qualityMax_min )
                        || ( settings.qualityMax > Settings::qualityMax_max )
                      )
                    {
                        error = true;
                    }


                    somethingDone = true;
                }
                else if( arg == "--dssimAvgMax" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.dssimAvgMax = std::stod( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.dssimAvgMax < Settings::dssimAvgMax_min )
                        || ( settings.dssimAvgMax > Settings::dssimAvgMax_max )
                      )
                    {
                        error = true;
                    }

                    somethingDone = true;
                }
                else if( arg == "--dssimPeakMax" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.dssimPeakMax = std::stod( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.dssimPeakMax < Settings::dssimPeakMax_min )
                        || ( settings.dssimPeakMax > Settings::dssimPeakMax_max )
                      )
                    {
                        error = true;
                    }

                    somethingDone = true;
                }
                else if( arg == "--copyMarkers" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    if( value == "all")
                    {
                        settings.copyMarkers = true;
                    }
                    else if( value == "none")
                    {
                        settings.copyMarkers = false;
                    }
                    else
                    {
                        error = true;
                    }

                    somethingDone = true;
                }
                else if( arg == "--initQualityStep" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.initQualityStep = std::stoi( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.initQualityStep < Settings::initQualityStep_min )
                        || ( settings.initQualityStep > Settings::initQualityStep_max )
                      )
                    {
                        error = true;
                    }


                    somethingDone = true;
                }
                else if( arg == "--cs444to420" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    if( value == "true" )
                    {
                        settings.cs444to420 = true;
                    }
                    else if( value == "false" )
                    {
                        settings.cs444to420 = false;
                    }
                    else
                    {
                        error = true;
                    }


                    somethingDone = true;
                }
                else if( arg == "--imageCompChunkSize" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;

                    try {
                        settings.imageCompChunkSize = std::stoi( value );
                    } catch (...) {
                        error = true;
                    }

                    if(    ( settings.imageCompChunkSize < Settings::imageCompChunkSize_min )
                        || ( settings.imageCompChunkSize > Settings::imageCompChunkSize_max )
                       )
                    {
                        error = true;
                    }


                    somethingDone = true;
                }
            }

            if( !somethingDone )
            {
                printUsage();
                return EXIT_FAILURE;
            }
        }

        if( error )
        {
            std::cerr << "error during argument parsing." << std::endl;
            printUsage();
            return EXIT_FAILURE;
        }
    }



    // configure logging
#ifdef USE_LOG4CXX
    struct stat fileStat;
    std::string log4cxxConfigFile = "Log4cxxConfig.xml";
    errno = 0;  // Set by stat() upon an error

    if( stat(log4cxxConfigFile.c_str(), &fileStat) == 0)
    {
        if(fileStat.st_mode & S_IRUSR) // User has read permission?
        {
            log4cxx::xml::DOMConfigurator::configure( log4cxxConfigFile );
        }
    }
    else   // Set logging to use the console appender with a "simple" layout
    {
        defaultLayout   = new log4cxx::SimpleLayout();
        defaultAppender = new log4cxx::ConsoleAppender(defaultLayout);

        loggerMain->addAppender           ( defaultAppender );
        loggerImage->addAppender          ( defaultAppender );
        loggerTransformation->addAppender ( defaultAppender );

//        auto logLevel = log4cxx::Level::getDebug();
//        auto logLevel = log4cxx::Level::getInfo();
//        auto logLevel = log4cxx::Level::getWarn();
        auto logLevel = log4cxx::Level::getError();
//        auto logLevel = log4cxx::Level::getFatal();
//        auto logLevel = log4cxx::Level::getOff();

        loggerMain->setLevel           ( logLevel );  // Log level set to DEBUG
        loggerImage->setLevel          ( logLevel );   // Log level set to INFO
        loggerTransformation->setLevel ( logLevel );   // Log level set to INFO

//        std::cout << "Could not open Log4cxx configuration XML file: " << log4cxxConfigFile << std::endl;
//        perror("Problem opening log4cxx config file");
    }
#endif //USE_LOG4CXX

    // reduce image size
    do
    {
        imageshrink::ImageJfif imagejfif1( settings.inputFile );

        if( !imagejfif1.isImageValid() )
        {
            error = true;
            std::cerr << "image file count not be loaded" << std::endl;
            break;
        }

        imageshrink::ImageCollection collection1;
        {
            imageshrink::ImageAverage image1Average   = imageshrink::ImageAverage( imagejfif1, settings.imageCompChunkSize );
            imageshrink::ImageVariance image1Variance = imageshrink::ImageVariance( imagejfif1, image1Average, settings.imageCompChunkSize );

            collection1.addImage( "original", std::make_shared<imageshrink::ImageDummy>( imagejfif1 ) );
            collection1.addImage( "average",  std::make_shared<imageshrink::ImageDummy>( image1Average ) );
            collection1.addImage( "variance", std::make_shared<imageshrink::ImageDummy>( image1Variance ) );
        }

        int quality = settings.qualityMax;
        int qualityStep = settings.initQualityStep;
        std::unordered_map<int /*quality*/, ImageComparisonResult> icrMap;

        ChrominanceSubsampling::VALUE cs = imagejfif1.getChrominanceSubsampling();
        if(    ( cs == ChrominanceSubsampling::CS_444 )
            && ( settings.cs444to420 )
           )
        {
            cs = ChrominanceSubsampling::CS_420;
        }

        while( qualityStep != 0 )
        {
            ImageComparisonResult icr;

            while(    ( icr.dssimAvg < settings.dssimAvgMax )
                   && ( icr.dssimPeak < settings.dssimPeakMax )
                   && ( quality > settings.qualityMin )
                 )
            {
                const auto icrMapEntry = icrMap.find( quality );

                if( icrMapEntry == icrMap.end() )
                {
                    imageshrink::ImageJfif imagejfif2         = imagejfif1.getCompressedDecompressedImage( /*quality*/ quality, cs );
                    imagejfif2                                = imagejfif2.getImageWithChrominanceSubsampling( imagejfif1.getChrominanceSubsampling() );
                    imageshrink::ImageAverage image2Average   = imageshrink::ImageAverage( imagejfif2, settings.imageCompChunkSize );
                    imageshrink::ImageVariance image2Variance = imageshrink::ImageVariance( imagejfif2, image2Average, settings.imageCompChunkSize );

                    imageshrink::ImageCollection collection2;
                    collection2.addImage( "original", std::make_shared<imageshrink::ImageDummy>( imagejfif2 ) );
                    collection2.addImage( "average",  std::make_shared<imageshrink::ImageDummy>( image2Average ) );
                    collection2.addImage( "variance", std::make_shared<imageshrink::ImageDummy>( image2Variance ) );

                    imageshrink::ImageDSSIM imageDSSIM( collection1, collection2, settings.imageCompChunkSize );

                    icr.dssimAvg  = imageDSSIM.getDssim();
                    icr.dssimPeak = imageDSSIM.getDssimPeak();

#ifdef USE_LOG4CXX
                    LOG4CXX_WARN( loggerMain,
                                 "DSSIM = "
                                 << icr.dssimAvg
                                 << "; DSSIM Peak = "
                                 << icr.dssimPeak
                                 << "; quality = " << quality
                    );
#endif //USE_LOG4CXX

                    icrMap[ quality ] = icr;
                }
                else
                {
                    icr = icrMapEntry->second;

#ifdef USE_LOG4CXX
                    LOG4CXX_WARN( loggerMain,
                                 "DSSIM = "
                                 << icr.dssimAvg
                                 << "; DSSIM Peak = "
                                 << icr.dssimPeak
                                 << "; quality = " << quality
                                 << " (restored result)"
                    );
#endif //USE_LOG4CXX
                }

                quality -= qualityStep;
            }

            quality     += ( 2 * qualityStep );
            qualityStep /= 2;   // qualityStep == 0: end of loop

#ifdef USE_LOG4CXX
            LOG4CXX_INFO( loggerMain, "qualityStep = " << qualityStep );
#endif //USE_LOG4CXX
        }

#ifdef USE_LOG4CXX
        LOG4CXX_INFO( loggerMain, "final quality setting = " << quality );
#else
        std::cout << "final quality setting = " << quality << std::endl;
#endif //USE_LOG4CXX

        if( settings.copyMarkers )
        {
            imageshrink::ImageJfif::ListOfMarkerShrdPtr markers = imagejfif1.getMarkers();
            imagejfif1.storeInFile( settings.outputFile, markers, quality, cs );
        }
        else
        {
            imagejfif1.storeInFile( settings.outputFile, quality, cs );
        }
    } while(0);


    // cleanup when application closes --> does not work for any reason
    // if( defaultAppender ) { delete defaultAppender; defaultAppender = nullptr; }
    // if( defaultLayout )   { delete defaultLayout; defaultLayout = nullptr; }


    // end of application
    if( error )
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
