#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <fstream>

void writeInt(std::ostream & stream, int val);
void writeFloat(std::ostream & stream, float val);

int readInt(std::istream & stream);
float readFloat(std::istream & stream);

#endif
