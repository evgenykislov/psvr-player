#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <string>

/*! TODO ??? */
bool CreateShaderProgram(unsigned int& program, unsigned char* vert_data,
    unsigned int vert_len, unsigned char* frag_data, unsigned int frag_len);

/*! TODO ??? */
void DeleteShaderProgram(unsigned int program);

#endif  // SHADER_PROGRAM_H
