#include <windows.h>
#include <stdio.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "fastlz.h"

void import(const char *inputName)
{
  const struct aiScene *scene = aiImportFile(inputName,
    aiProcess_Triangulate |
    aiProcess_ConvertToLeftHanded |
    aiProcess_OptimizeMeshes);
  if(scene == NULL)
  {
    printf("Error trying to import file\n");
    return;
  }
  aiReleaseImport(scene);
}

int main(int argc, char *argv[])
{
  if(argc < 3)
  {
    return printf("Usage: %s input.(dae|obj|fbx) output.sos.lz\n", argv[0]);
  }
  import(argv[1]);
  return 0;
}
