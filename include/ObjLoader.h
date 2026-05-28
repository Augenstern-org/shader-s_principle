//
// Created by neuroil on 2026/5/28.
//

#ifndef SHADER_S_PRINCIPLE_OBJLOADER_H
#define SHADER_S_PRINCIPLE_OBJLOADER_H

#include "Mesh.h"
#include <string>

namespace ObjLoader {

Mesh load(const std::string& filePath);

} // namespace ObjLoader

#endif // SHADER_S_PRINCIPLE_OBJLOADER_H