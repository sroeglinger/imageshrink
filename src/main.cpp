
#include <stdlib.h>
#include <iostream>

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

int main( int argc, const char* argv[] )
{

    // Initialize variables
    log4cxx::AppenderPtr defaultAppender = nullptr;
    log4cxx::LayoutPtr   defaultLayout   = nullptr;


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




    {
        // int value = 5;

        // LOG4CXX_TRACE( loggerMain, "this is a debug message for detailed code discovery. Value=" << value );
        // LOG4CXX_DEBUG( loggerMain, "this is a debug message." );
        // LOG4CXX_INFO ( loggerMain, "this is a info message, ignore. Value=" << value );
        // LOG4CXX_WARN ( loggerMain, "this is a warn message, not too bad." );
        // LOG4CXX_ERROR( loggerMain, "this is a error message, something serious is happening." );
        // LOG4CXX_FATAL( loggerMain, "this is a fatal message!!!" );

        // imageshrink::ImageJfif       imagejfif1( "/home/wast/Documents/test/test/resources/test.jpg" );
        imageshrink::ImageJfif       imagejfif1( "/home/wast/Documents/test/test/resources/test2.jpg" );
        // imageshrink::ImageJfif       imagejfif1( "/home/wast/Documents/test/test/resources/lena.jpg" );
        
        imageshrink::ImageAverage    image1Average;
        imageshrink::ImageVariance   image1Variance;
        imageshrink::ImageJfif       imagejfif2;
        imageshrink::ImageAverage    image2Average;
        imageshrink::ImageVariance   image2Variance;

        image1Average = imageshrink::ImageAverage( imagejfif1 );
        image1Variance = imageshrink::ImageVariance( imagejfif1, image1Average );

        imageshrink::ImageCollection collection1;
        collection1.addImage( "original", imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( imagejfif1 ) ) );
        collection1.addImage( "average",  imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( image1Average ) ) );
        collection1.addImage( "variance", imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( image1Variance ) ) );

        double dssim = 0.0;
        double dssimPeak = 0.0;
        int quality = 90;
        ChrominanceSubsampling::VALUE cs = ChrominanceSubsampling::CS_420;
        while(    ( dssim < 50.0e-6 ) 
               && ( dssimPeak < 8800.0e-6 )
               && ( quality > 20 ) )
        {
            imagejfif2 = imagejfif1.getCompressedDecompressedImage( /*quality*/ quality, cs );
            imagejfif2 = imagejfif2.getImageWithChrominanceSubsampling( ChrominanceSubsampling::CS_444 );
            image2Average = imageshrink::ImageAverage( imagejfif2 );
            image2Variance = imageshrink::ImageVariance( imagejfif2, image2Average );

            imageshrink::ImageCollection collection2;
            collection2.addImage( "original", imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( imagejfif2 ) ) );
            collection2.addImage( "average",  imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( image2Average ) ) );
            collection2.addImage( "variance", imageshrink::ImageInterfaceShrdPtr( new imageshrink::ImageDummy( image2Variance ) ) );

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
        
        imagejfif1.storeInFile( "out.jpg", quality, cs );

    }


    // cleanup when application closes
    // if( defaultAppender ) { delete defaultAppender; defaultAppender = nullptr; }
    // if( defaultLayout )   { delete defaultLayout; defaultLayout = nullptr; }
    

    // end of application
    return EXIT_SUCCESS;
}
