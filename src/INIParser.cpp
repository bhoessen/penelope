#include <penelope/utils/INIParser.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

using namespace penelope;

const std::string emptyString("");

INIParser::INIParser(const std::string& aFileName) : fileName(aFileName), values(){

}

INIParser::~INIParser(){

}

void INIParser::parse(){
    std::ifstream file;
    file.open(fileName.c_str());

    const int BUF_LENGHT = 2048;
    char buffer[BUF_LENGHT+1];
    memset(buffer, 0, BUF_LENGHT );

    std::string line;
    std::string currentConf("global");
    if(values.find(currentConf)==values.end()){
        std::map<std::string, std::string> tmp;
        values[currentConf]=tmp;
    }
    while(std::getline(file, line)){

        if(line.length()==0 || line.at(0)==';'){
            continue;
        }


        if(line.at(0)=='['){
            //start of a new configuration

            std::stringstream input;
            for(unsigned int i=1;i<line.length(); i++){
                if(line.at(i)==']'){
                    break;
                }
                if(line.at(i)!=' '){
                    input << line.at(i);
                }
            }
            currentConf = input.str();

            if(values.find(currentConf)==values.end()){
                //create the map in the values map for the new configuration
                std::map<std::string, std::string> tmp;
                values[currentConf]=tmp;
            }
            
        }else{

            std::stringstream key;
            std::stringstream value;
            bool eqFound = false;
            for(unsigned int i=0; i<line.length(); i++){
                if(line.at(i)=='='){
                    eqFound = true;
                }else if(line.at(i)==';'){
                    i = line.length();
                }else if(line.at(i)!=' '){
                    if(eqFound){
                        value << line.at(i);
                    }else{
                        key << line.at(i);
                    }
                }
            }

            std::string k(key.str());
            std::string v(value.str());
            if(k.length()>0 && v.length()>0){
                values[currentConf][k]=v;
            }

        }
    }
}

bool INIParser::configurationExist(const std::string& conf) const{
    return values.find(conf) != values.end();
}

const std::string& INIParser::getValueForConf(const std::string& conf, const std::string& key) const {
    std::map<std::string, std::map<std::string, std::string> >::const_iterator it(values.find(conf));
    if(it!=values.end()){
        std::map<std::string, std::string>::const_iterator iit(it->second.find(key));
        if(iit!=it->second.end()){
            return iit->second;
        }
    }

    return emptyString;
    
}
