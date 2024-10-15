#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <string>

/*! Создать gl программу на основе вершинного и фрагментного шейдера
\param programm возвращаемый идентификатор созданной gl программы
\param vert_data текстовый блок с вершинным шейдером
\param vert_len длина текстового блока с вершинным шейдером
\param frag_data текстовый блок с фрагментным шейдером
\param frag_len длина текстового блока с фрагментным шейдером
\return признак успешного создания gl программы */
bool CreateShaderProgram(unsigned int& program, unsigned char* vert_data,
    unsigned int vert_len, unsigned char* frag_data, unsigned int frag_len);

/*! Удалить gl программу, освободить ресуры
\param program программа для удаления */
void DeleteShaderProgram(unsigned int program);

#endif  // SHADER_PROGRAM_H
