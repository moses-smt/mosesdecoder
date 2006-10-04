
#include <string>
#include <fstream>

std::string GetTempFolder();
void createtempfile(std::ofstream  &fileStream, std::string &filePath, std::ios_base::open_mode flags);
void removefile(const std::string &filePath);
