#include <windows.h>
#include <stdio.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "fastlz.h"

#define SCALE_FACTOR 1000000

struct vector3
{
  float x, y, z;
};

struct vertex
{
  struct vector3 position;
  struct vector3 normal;
  struct vector3 color;
  float tu, tv;
};

void clean(FILE *file, ...)
{
  va_list args;
  va_start(args, file);
  BYTE *nextHeap = va_arg(args, BYTE*);
  while(nextHeap != 0)
  {
    HeapFree(GetProcessHeap(), 0, nextHeap);
    nextHeap = va_arg(args, BYTE*);
  }
  va_end(args);
  fclose(file);
}

struct vertex *vertices(struct aiMatrix4x4 transform, struct aiMesh *mesh,
  int *count)
{
  int vertexCount = 0;
  for(int i = 0; i < mesh->mNumFaces; i++)
  {
    vertexCount += mesh->mFaces[i].mNumIndices;
  }
  struct vertex *vertices = malloc(sizeof(struct vertex) * vertexCount);

  for(int i = 0; i < mesh->mNumFaces; i++)
  {
    struct aiFace face = mesh->mFaces[i];
    for(int j = 0; j < face.mNumIndices; j++)
    {
      struct vertex vertex;
      struct aiVector3D transformedVertex = mesh->mVertices[face.mIndices[j]];
      aiTransformVecByMatrix4(&transformedVertex, &transform);

      vertex.position.x = transformedVertex.x;
      vertex.position.y = transformedVertex.y;
      vertex.position.z = transformedVertex.z;

      if(mesh->mNormals != NULL)
      {
        vertex.normal.x = mesh->mNormals[face.mIndices[j]].x;
        vertex.normal.y = mesh->mNormals[face.mIndices[j]].y;
        vertex.normal.z = mesh->mNormals[face.mIndices[j]].z;
      }

      if(mesh->mColors[0] != NULL)
      {
        vertex.color.x = mesh->mColors[0][face.mIndices[j]].r;
        vertex.color.y = mesh->mColors[0][face.mIndices[j]].g;
        vertex.color.z = mesh->mColors[0][face.mIndices[j]].b;
      }

      if(mesh->mTextureCoords[0] != NULL)
      {
        vertex.tu = mesh->mTextureCoords[0][face.mIndices[j]].x;
        vertex.tv = mesh->mTextureCoords[0][face.mIndices[j]].y;
      }

      vertices[(i * 3) + j] = vertex;
    }
  }
  *count = vertexCount;
  return vertices;
}

void process(FILE *file, struct aiNode *node, const struct aiScene *scene)
{
  for(int i = 0; i < node->mNumMeshes; i++)
  {
    int count = 0;
    struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    struct vertex *verts = vertices(node->mTransformation, mesh, &count);
    for(int j = 0; j < count; j++)
    {
      fprintf(file, "%x %x %x %x %x %x %x %x %x %x %x\n",
        (int)(verts[j].position.x * SCALE_FACTOR),
        (int)(verts[j].position.y * SCALE_FACTOR),
        (int)(verts[j].position.z * SCALE_FACTOR),
        (int)(verts[j].normal.x * SCALE_FACTOR),
        (int)(verts[j].normal.y * SCALE_FACTOR),
        (int)(verts[j].normal.z * SCALE_FACTOR),
        (int)(verts[j].color.x * SCALE_FACTOR),
        (int)(verts[j].color.y * SCALE_FACTOR),
        (int)(verts[j].color.z * SCALE_FACTOR),
        (int)(verts[j].tu * SCALE_FACTOR),
        (int)(verts[j].tv * SCALE_FACTOR));
    }
    free(verts);
  }

  for(int i = 0; i < node->mNumChildren; i++)
  {
    process(file, node->mChildren[i], scene);
  }
}

int compress(char *outputName)
{
  FILE *file = fopen(outputName, "rb");
  if(file == NULL) return printf("Could not open simple model file\n");

  // Get the total size of the model file
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Read in all of the data to a buffer
  BYTE *fileData = HeapAlloc(GetProcessHeap(), 0, size);
  int bytesRead = fread(fileData, 1, size, file);
  if(bytesRead != size)
  {
    clean(file, fileData, 0);
    return printf("Error reading simple model file\n");
  }
  clean(file, 0);

  // Compressed buffer must be at least 5% larger.
  BYTE *compressedData = HeapAlloc(GetProcessHeap(), 0, size + (size * 0.05));
  int bytesCompressed = fastlz_compress(fileData, size, compressedData);
  if(bytesCompressed == 0)
  {
    clean(file, fileData, compressedData, 0);
    return printf("Error compressing simple model data\n");
  }

  // Annotate compressed data with uncompressed size.
  int annotatedSize = bytesCompressed + sizeof(int);
  BYTE *buffer = HeapAlloc(GetProcessHeap(), 0, annotatedSize);
  CopyMemory(buffer, &size, sizeof(int));
  CopyMemory(buffer + sizeof(int), compressedData, bytesCompressed);

  // Write compressed file back out.
  file = fopen(outputName, "wb");
  if(file == NULL)
  {
    clean(file, fileData, compressedData, buffer, 0);
    return printf("Could not open compressed file\n");
  }
  int bytesWritten = fwrite(buffer, 1, annotatedSize, file);
  if(bytesWritten != annotatedSize)
  {
    clean(file, fileData, compressedData, buffer, 0);
    return printf("Error writing compressed file");
  }

  // Clean up
  clean(file, fileData, compressedData, buffer, 0);
  return 1;
}

void import(const char *inputName, const char *outputName)
{
  const struct aiScene *scene = aiImportFile(inputName,
    aiProcess_Triangulate | aiProcess_ConvertToLeftHanded |
    aiProcess_OptimizeMeshes);
  if(scene == NULL)
  {
    printf("Error trying to import file\n");
    return;
  }
  FILE *file = fopen(outputName, "wb");
  if(file)
  {
    process(file, scene->mRootNode, scene);
    fclose(file);
  }
  aiReleaseImport(scene);
}

int main(int argc, char *argv[])
{
  if(argc < 3)
  {
    return printf("Usage: %s input.(dae|obj|fbx) output.sos.lz\n", argv[0]);
  }
  import(argv[1], argv[2]);
  compress(argv[2]);
  return 0;
}
