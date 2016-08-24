
#include <stdlib.h>
#include <iostream>
#include <string>

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
    , inputFile()
    , outputFile()
    {}
    
    int    qualityMin;
    int    qualityMax;
    double dssimAvgMax;
    double dssimPeakMax;

    std::string inputFile;
    std::string outputFile;
};


void printUsage()
{
    std::cout << "imageshrink [settings] inputFile outputFile" << std::endl;
    std::cout << std::endl;
    std::cout << "settings:" << std::endl;
    std::cout << "    --min value           minimum jpeg quality" << std::endl;
    std::cout << "    --max value           maximum jpeg quality" << std::endl;
    std::cout << "    --dssimAvgMax value   maximum for the average DSSIM" << std::endl;
    std::cout << "    --dssimPeakMax value  maximum for the peak DSSIM" << std::endl;
}


int main( int argc, const char* argv[] )
{
    // Initialize variables
    log4cxx::AppenderPtr defaultAppender = nullptr;
    log4cxx::LayoutPtr   defaultLayout   = nullptr;

    Settings settings;
    
    // parse arguments
    {
        int pos = 1;
        
        if( argc < 2 )
        {
            printUsage();
            return EXIT_FAILURE;
        }
        
        while( pos < argc )
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
                    
                    settings.qualityMin = std::stoi( value );
                    
                    somethingDone = true;
                }
                else if( arg == "--max" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;
                    
                    settings.qualityMax = std::stoi( value );
                    
                    somethingDone = true;
                }
                else if( arg == "--dssimAvgMax" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;
                    
                    settings.dssimAvgMax = std::stod( value );
                    
                    somethingDone = true;
                }
                else if( arg == "--dssimPeakMax" )
                {
                    const std::string value( argv[ pos ] );
                    pos = pos + 1;
                    
                    settings.dssimPeakMax = std::stod( value );
                    
                    somethingDone = true;
                }
            }
            
            if( !somethingDone )
            {
                printUsage();
                return EXIT_FAILURE;
            }
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
        // auto logLevel = log4cxx::Level::getInfo();
        // auto logLevel = log4cxx::Level::getWarn();
        
        loggerMain->setLevel           ( logLevel );  // Log level set to DEBUG
        loggerImage->setLevel          ( logLevel );   // Log level set to INFO
        loggerTransformation->setLevel ( logLevel );   // Log level set to INFO

        std::cout << "Could not open Log4cxx configuration XML file: " << log4cxxConfigFile << std::endl;
        perror("Problem opening log4cxx config file");
    }

    // reduce image size
    {
        imageshrink::ImageJfif imagejfif1( settings.inputFile );
        
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

        double dssim = 0.0;
        double dssimPeak = 0.0;
        int quality = settings.qualityMax;
        ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_420;
        while(    ( dssim < settings.dssimAvgMax )
               && ( dssimPeak < settings.dssimPeakMax )
               && ( quality > settings.qualityMin ) )
        {
            imagejfif2 = imagejfif1.getCompressedDecompressedImage( /*quality*/ quality, cs );
            imagejfif2 = imagejfif2.getImageWithChrominanceSubsampling( ChrominanceSubsampling::CS_444 );
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
                << "; quality = " << quality );

            quality -= 1;
        }
        
        imagejfif1.storeInFile( settings.outputFile, quality, cs );
    }


    // cleanup when application closes
    // if( defaultAppender ) { delete defaultAppender; defaultAppender = nullptr; }
    // if( defaultLayout )   { delete defaultLayout; defaultLayout = nullptr; }
    

    // end of application
    return EXIT_SUCCESS;
}
