//
// Created by Eamonn Buss on 3/17/18.
//

#ifndef GERBERA_ICONFIGGENERATOR_H
#define GERBERA_ICONFIGGENERATOR_H


class IConfigGenerator {

public:
    virtual ~IConfigGenerator() {}
    virtual std::string generate() = 0;
};


#endif //GERBERA_ICONFIGGENERATOR_H
