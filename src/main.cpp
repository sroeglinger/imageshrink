
#include <stdlib.h>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>


#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/consoleappender.h>


#include "ImageJfif.h"
#include "ImageAverage.h"
#include "ImageVariance.h"
#include "ImageCovariance.h"

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

        imageshrink::ImageJfif       imagejfif( "/home/wast/Documents/test/test/resources/lena.jpg" );
        imageshrink::ImageAverage    imageAverage( imagejfif );
        imageshrink::ImageVariance   imageVariance( imagejfif, imageAverage );
        imageshrink::ImageCovariance imageCovariance( imagejfif, imageAverage, imagejfif, imageAverage );
        imageshrink::ImageJfif       imagejfif2( imageCovariance );
        imagejfif2.storeInFile( "test.jpg" );
    }


    // cleanup when application closes
    // if( defaultAppender ) { delete defaultAppender; defaultAppender = nullptr; }
    // if( defaultLayout )   { delete defaultLayout; defaultLayout = nullptr; }
    

    // end of application
    return EXIT_SUCCESS;
}
