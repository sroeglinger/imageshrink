
#include <stdlib.h>
#include <iostream>
#include <string>
#include <unordered_map>

#include <sys/stat.h>
#include <sys/types.h>


#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/consoleappender.h>


#include "ImageDummy.h"
#include "ImageJfif.h"
#include "ImageAverage.h"
#include "ImageVariance.h"
#include "ImageCollection.h"
#include "ImageDSSIM.h"

// Define static logger variable
log4cxx::LoggerPtr loggerMain           ( log4cxx::Logger::getLogger( "main" ) );
log4cxx::LoggerPtr loggerImage          ( log4cxx::Logger::getLogger( "image" ) );
log4cxx::LoggerPtr loggerTransformation ( log4cxx::Logger::getLogger( "transformation" ) );

struct Settings
{
    Settings()
    : qualityMin( 50 )
    , qualityMax( 85 )
    , dssimAvgMax( 70.0e-6 )
    , dssimPeakMax( 8900.0e-6 )
    , copyMarkers( true )
    , initQualityStep( 10 )
    , inputFile()
    , outputFile()
    {}
    
    int    qualityMin;
    int    qualityMax;
    double dssimAvgMax;
    double dssimPeakMax;
    bool   copyMarkers;
    int    initQualityStep;

    std::string inputFile;
    std::string outputFile;
};

struct ImageComparisonResult
{
    ImageComparisonResult()
    : dssimAvg( 0.0 )
    , dssimPeak( 0.0 )
    {}
    
    double dssimAvg;
    double dssimPeak;
};


void printUsage()
{
    std::cout << "imageshrink [settings] inputFile outputFile" << std::endl;
    std::cout << std::endl;
    std::cout << "settings:" << std::endl;
    std::cout << "    --min value               minimum jpeg quality (0 <= value <= 100)" << std::endl;
    std::cout << "    --max value               maximum jpeg quality (0 <= value <= 100)" << std::endl;
    std::cout << "    --dssimAvgMax value       maximum for the average DSSIM (0.0 <= value <= 1.0)" << std::endl;
    std::cout << "    --dssimPeakMax value      maximum for the peak DSSIM (0.0 <= value <= 1.0)" << std::endl;
    std::cout << "    --copyMarkers value       maximum for the peak DSSIM (value = none|all)" << std::endl;
    std::cout << "    --initQualityStep value   init value for qulaity steps (1 <= value <= 10)" << std::endl;
}


int main( int argc, const char* argv[] )
{
    // Initialize variables
    log4cxx::AppenderPtr defaultAppender = nullptr;
    log4cxx::LayoutPtr   defaultLayout   = nullptr;

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
                    
                    if(    ( settings.qualityMin < 0 )
                        || ( settings.qualityMin > 100 )
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
                    
                    if(    ( settings.qualityMax < 0 )
                        || ( settings.qualityMax > 100 )
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
                    
                    if(    ( settings.dssimAvgMax < 0.0 )
                        || ( settings.dssimAvgMax > 1.0 )
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
                    
                    if(    ( settings.dssimPeakMax < 0.0 )
                        || ( settings.dssimPeakMax > 1.0 )
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
                    
                    if(    ( settings.initQualityStep < 1 )
                        || ( settings.initQualityStep > 10 )
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

        auto logLevel = log4cxx::Level::getDebug();
//        auto logLevel = log4cxx::Level::getInfo();
//        auto logLevel = log4cxx::Level::getWarn();
//        auto logLevel = log4cxx::Level::getError();
//        auto logLevel = log4cxx::Level::getFatal();
//        auto logLevel = log4cxx::Level::getOff();
        
        loggerMain->setLevel           ( logLevel );  // Log level set to DEBUG
        loggerImage->setLevel          ( logLevel );   // Log level set to INFO
        loggerTransformation->setLevel ( logLevel );   // Log level set to INFO

//        std::cout << "Could not open Log4cxx configuration XML file: " << log4cxxConfigFile << std::endl;
//        perror("Problem opening log4cxx config file");
    }

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
        
        imageshrink::ImageAverage    image1Average;
        imageshrink::ImageVariance   image1Variance;
        imageshrink::ImageJfif       imagejfif2;
        imageshrink::ImageAverage    image2Average;
        imageshrink::ImageVariance   image2Variance;

        image1Average = imageshrink::ImageAverage( imagejfif1 );
        image1Variance = imageshrink::ImageVariance( imagejfif1, image1Average );

        imageshrink::ImageCollection collection1;
        collection1.addImage( "original", std::make_shared<imageshrink::ImageDummy>( imagejfif1 ) );
        collection1.addImage( "average",  std::make_shared<imageshrink::ImageDummy>( image1Average ) );
        collection1.addImage( "variance", std::make_shared<imageshrink::ImageDummy>( image1Variance ) );

        int quality = settings.qualityMax;
        int qualityLast = quality;
        int qualityStep = settings.initQualityStep;
        ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_420;
        std::unordered_map<int /*quality*/, ImageComparisonResult> icrMap;
        
        while( qualityStep != 0 )
        {
            double dssim = 0.0;
            double dssimPeak = 0.0;
            
            while(    ( dssim < settings.dssimAvgMax )
                   && ( dssimPeak < settings.dssimPeakMax )
                   && ( quality > settings.qualityMin ) )
            {
                const auto icrMapEntry = icrMap.find( quality );
                
                if( icrMapEntry == icrMap.end() )
                {
                    imagejfif2 = imagejfif1.getCompressedDecompressedImage( /*quality*/ quality, cs );
                    imagejfif2 = imagejfif2.getImageWithChrominanceSubsampling( imagejfif1.getChrominanceSubsampling() );
                    image2Average = imageshrink::ImageAverage( imagejfif2 );
                    image2Variance = imageshrink::ImageVariance( imagejfif2, image2Average );
                    
                    imageshrink::ImageCollection collection2;
                    collection2.addImage( "original", std::make_shared<imageshrink::ImageDummy>( imagejfif2 ) );
                    collection2.addImage( "average",  std::make_shared<imageshrink::ImageDummy>( image2Average ) );
                    collection2.addImage( "variance", std::make_shared<imageshrink::ImageDummy>( image2Variance ) );
                    
                    imageshrink::ImageDSSIM imageDSSIM( collection1, collection2 );
                    
                    dssim = imageDSSIM.getDssim();
                    dssimPeak = imageDSSIM.getDssimPeak();
                    
                    LOG4CXX_WARN( loggerMain,
                                 "DSSIM = "
                                 << dssim
                                 << "; DSSIM Peak = "
                                 << dssimPeak
                                 << "; quality = " << quality
                    );
                    
                    ImageComparisonResult icr;
                    icr.dssimAvg = dssim;
                    icr.dssimPeak = dssimPeak;
                    icrMap[ quality ] = icr;
                }
                else
                {
                    dssim = icrMapEntry->second.dssimAvg;
                    dssimPeak = icrMapEntry->second.dssimPeak;
                    
                    LOG4CXX_WARN( loggerMain,
                                 "DSSIM = "
                                 << dssim
                                 << "; DSSIM Peak = "
                                 << dssimPeak
                                 << "; quality = " << quality
                                 << " (restored result)"
                    );
                }
                
                qualityLast = quality;
                quality -= qualityStep;
            }
            
            quality     += ( 2 * qualityStep );
            qualityLast  = quality;
            qualityStep /= 2;   // qualityStep == 0: end of loop
            
            LOG4CXX_INFO( loggerMain, "qualityStep = " << qualityStep );
        }
        
        LOG4CXX_INFO( loggerMain, "final quality setting = " << quality );
        
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
