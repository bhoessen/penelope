/* 
 * File:   INIParser.h
 * Author: bhoessen
 *
 * Created on 20 février 2012, 10:10
 */

#ifndef INIPARSER_H
#define	INIPARSER_H

#include <map>
#include <string>

namespace penelope{

    /**
     * The aim of the iniparser is to parse a .ini file
     */
    class INIParser{
    public:

        /**
         * Create a new parser
         * @param fileName the name of the file that we will parse
         */
        INIParser(const std::string& fileName);

        /**
         * Destructor
         */
        ~INIParser();

        /**
         * Parse the file and store it's content
         */
        void parse();

        /**
         * Check whether a given configuration exists or not
         * @param conf the name of the configuration we want to check.
         * @return true if the configuration conf exists, false otherwise
         */
        bool configurationExist(const std::string& conf) const;

        /**
         * Retrieve the attribute value for a given configuration
         * @param conf the configuration
         * @param attribute the name of the attribute that we would like its value
         * @return the value of the attribute. If either the configuration or
         *         the attribute isn't present, an empty string will be returned
         */
        const std::string& getValueForConf(const std::string& conf, const std::string& attribute) const ;

    private:

        /** The name of the file that was parsed */
        std::string fileName;
        /** The map configuration name -> (key -> value) */
        std::map<std::string, std::map<std::string, std::string> > values;

    };

}

#endif	/* INIPARSER_H */

