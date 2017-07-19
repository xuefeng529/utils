#include "base/Regex.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        if (base::Regex::isMatch(line, argv[1]))
        {
            std::cout << "it is match" << std::endl;
        }
        else
        {
            std::cout << "it no match" << std::endl;
        }
    }
    
    return 0;
}