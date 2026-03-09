#include "mcy_helpers.h"
#include "vendor/glm/vec2.hpp"
#include "vendor/glm/vec3.hpp"

#include <cstdlib>
#include <cstring>
#include <cctype>

struct ObjFaceVertex
{
    u32 vIndex;
    u32 vtIndex;
    u32 vnIndex;
};

struct ObjFace
{
    ObjFaceVertex vertices[3];
};

struct ObjLoadedInfo
{
    glm::vec3* v;
    st vCount;

    glm::vec2* vt;
    st vtCount;

    glm::vec3* vn;
    st vnCount;

    ObjFace* faces;
    st faceCount;
};

static void ObjReset(ObjLoadedInfo* obj)
{
    obj->v = nullptr;
    obj->vCount = 0;
    obj->vt = nullptr;
    obj->vtCount = 0;
    obj->vn = nullptr;
    obj->vnCount = 0;
    obj->faces = nullptr;
    obj->faceCount = 0;
}

static void ObjFree(ObjLoadedInfo* obj)
{
    delete[] obj->v;
    delete[] obj->vt;
    delete[] obj->vn;
    delete[] obj->faces;
    ObjReset(obj);
}

static void ObjDebugPrint(const char* message, const char* fileName, st lineNumber)
{
    char buffer[512];
    sprintf_s(buffer, "OBJ load failed (%s:%zu): %s\n", fileName, lineNumber, message);
    OutputDebugStringA(buffer);
}

static char* SkipWhitespace(char* at)
{
    while (*at && std::isspace((unsigned char)*at))
        ++at;
    return at;
}

static bool HasOnlyWhitespaceOrComment(char* at)
{
    at = SkipWhitespace(at);
    return (*at == 0 || *at == '#');
}

static bool ParseFloat3Strict(char* at, glm::vec3* outValue)
{
    at = SkipWhitespace(at);

    char* end = nullptr;
    float x = strtof(at, &end);
    if (end == at) return false;
    at = SkipWhitespace(end);

    float y = strtof(at, &end);
    if (end == at) return false;
    at = SkipWhitespace(end);

    float z = strtof(at, &end);
    if (end == at) return false;
    at = end;

    if (!HasOnlyWhitespaceOrComment(at))
        return false;

    outValue->x = x;
    outValue->y = y;
    outValue->z = z;
    return true;
}

static bool ParseFloat2Strict(char* at, glm::vec2* outValue)
{
    at = SkipWhitespace(at);

    char* end = nullptr;
    float x = strtof(at, &end);
    if (end == at) return false;
    at = SkipWhitespace(end);

    float y = strtof(at, &end);
    if (end == at) return false;
    at = end;

    if (!HasOnlyWhitespaceOrComment(at))
        return false;

    outValue->x = x;
    outValue->y = y;
    return true;
}

static bool ParsePositiveIndex(char*& at, u32* outIndexZeroBased)
{
    at = SkipWhitespace(at);

    if (*at == '-')
        return false;

    char* end = nullptr;
    unsigned long value = strtoul(at, &end, 10);
    if (end == at || value == 0)
        return false;

    *outIndexZeroBased = (u32)(value - 1);
    at = end;
    return true;
}

static bool ParseFaceVertex(char*& at, ObjFaceVertex* outVertex)
{
    if (!ParsePositiveIndex(at, &outVertex->vIndex))
        return false;

    if (*at != '/')
        return false;
    ++at;

    if (!ParsePositiveIndex(at, &outVertex->vtIndex))
        return false;

    if (*at != '/')
        return false;
    ++at;

    if (!ParsePositiveIndex(at, &outVertex->vnIndex))
        return false;

    return true;
}

static bool ParseFaceStrict(char* at, ObjFace* outFace)
{
    at = SkipWhitespace(at);

    if (!ParseFaceVertex(at, &outFace->vertices[0]))
        return false;
    at = SkipWhitespace(at);

    if (!ParseFaceVertex(at, &outFace->vertices[1]))
        return false;
    at = SkipWhitespace(at);

    if (!ParseFaceVertex(at, &outFace->vertices[2]))
        return false;
    at = SkipWhitespace(at);

    if (!HasOnlyWhitespaceOrComment(at))
        return false;

    return true;
}

static bool IsIgnoredObjLine(char* line)
{
    if (*line == 0 || *line == '#')
        return true;

    if (std::strncmp(line, "o ", 2) == 0) return true;
    if (std::strncmp(line, "g ", 2) == 0) return true;
    if (std::strncmp(line, "s ", 2) == 0) return true;
    if (std::strncmp(line, "usemtl ", 7) == 0) return true;
    if (std::strncmp(line, "mtllib ", 7) == 0) return true;

    return false;
}

bool LoadObjFile(const char* fileName, ObjLoadedInfo* outObj)
{
    if (!fileName || !outObj)
        return false;

    ObjReset(outObj);

    char* fileBuffer = nullptr;
    st fileSize = 0;
    if (!ReadEntireFile(fileName, &fileBuffer, &fileSize))
        return false;

    char* text = new char[fileSize + 1];
    memcpy(text, fileBuffer, fileSize);
    text[fileSize] = 0;
    delete[] fileBuffer;

    st vCount = 0;
    st vtCount = 0;
    st vnCount = 0;
    st faceCount = 0;

    st lineNumber = 1;
    for (char* line = text; *line;)
    {
        char* nextLine = line;
        while (*nextLine && *nextLine != '\n' && *nextLine != '\r')
            ++nextLine;

        char saved = *nextLine;
        *nextLine = 0;

        char* trimmed = SkipWhitespace(line);

        if (std::strncmp(trimmed, "v ", 2) == 0)
        {
            glm::vec3 temp;
            if (!ParseFloat3Strict(trimmed + 2, &temp))
            {
                ObjDebugPrint("invalid vertex; expected: v x y z", fileName, lineNumber);
                delete[] text;
                return false;
            }
            ++vCount;
        }
        else if (std::strncmp(trimmed, "vt ", 3) == 0)
        {
            glm::vec2 temp;
            if (!ParseFloat2Strict(trimmed + 3, &temp))
            {
                ObjDebugPrint("invalid texcoord; expected: vt u v", fileName, lineNumber);
                delete[] text;
                return false;
            }
            ++vtCount;
        }
        else if (std::strncmp(trimmed, "vn ", 3) == 0)
        {
            glm::vec3 temp;
            if (!ParseFloat3Strict(trimmed + 3, &temp))
            {
                ObjDebugPrint("invalid normal; expected: vn x y z", fileName, lineNumber);
                delete[] text;
                return false;
            }
            ++vnCount;
        }
        else if (std::strncmp(trimmed, "f ", 2) == 0)
        {
            ObjFace temp;
            if (!ParseFaceStrict(trimmed + 2, &temp))
            {
                ObjDebugPrint("invalid face; expected triangle in v/vt/vn format with positive absolute indices", fileName, lineNumber);
                delete[] text;
                return false;
            }
            ++faceCount;
        }
        else if (!IsIgnoredObjLine(trimmed))
        {
            ObjDebugPrint("unsupported OBJ directive", fileName, lineNumber);
            delete[] text;
            return false;
        }

        *nextLine = saved;

        while (*nextLine == '\r' || *nextLine == '\n')
        {
            if (*nextLine == '\n')
                ++lineNumber;
            ++nextLine;
        }

        line = nextLine;
    }

    outObj->v = (vCount > 0) ? new glm::vec3[vCount] : nullptr;
    outObj->vt = (vtCount > 0) ? new glm::vec2[vtCount] : nullptr;
    outObj->vn = (vnCount > 0) ? new glm::vec3[vnCount] : nullptr;
    outObj->faces = (faceCount > 0) ? new ObjFace[faceCount] : nullptr;

    outObj->vCount = vCount;
    outObj->vtCount = vtCount;
    outObj->vnCount = vnCount;
    outObj->faceCount = faceCount;

    st vWrite = 0;
    st vtWrite = 0;
    st vnWrite = 0;
    st faceWrite = 0;

    lineNumber = 1;
    for (char* line = text; *line;)
    {
        char* nextLine = line;
        while (*nextLine && *nextLine != '\n' && *nextLine != '\r')
            ++nextLine;

        char saved = *nextLine;
        *nextLine = 0;

        char* trimmed = SkipWhitespace(line);

        if (std::strncmp(trimmed, "v ", 2) == 0)
        {
            if (!ParseFloat3Strict(trimmed + 2, &outObj->v[vWrite]))
            {
                ObjDebugPrint("invalid vertex during second pass", fileName, lineNumber);
                ObjFree(outObj);
                delete[] text;
                return false;
            }
            ++vWrite;
        }
        else if (std::strncmp(trimmed, "vt ", 3) == 0)
        {
            if (!ParseFloat2Strict(trimmed + 3, &outObj->vt[vtWrite]))
            {
                ObjDebugPrint("invalid texcoord during second pass", fileName, lineNumber);
                ObjFree(outObj);
                delete[] text;
                return false;
            }
            ++vtWrite;
        }
        else if (std::strncmp(trimmed, "vn ", 3) == 0)
        {
            if (!ParseFloat3Strict(trimmed + 3, &outObj->vn[vnWrite]))
            {
                ObjDebugPrint("invalid normal during second pass", fileName, lineNumber);
                ObjFree(outObj);
                delete[] text;
                return false;
            }
            ++vnWrite;
        }
        else if (std::strncmp(trimmed, "f ", 2) == 0)
        {
            if (!ParseFaceStrict(trimmed + 2, &outObj->faces[faceWrite]))
            {
                ObjDebugPrint("invalid face during second pass", fileName, lineNumber);
                ObjFree(outObj);
                delete[] text;
                return false;
            }

            for (int i = 0; i < 3; ++i)
            {
                if (outObj->faces[faceWrite].vertices[i].vIndex >= outObj->vCount)
                {
                    ObjDebugPrint("face references vertex out of range", fileName, lineNumber);
                    ObjFree(outObj);
                    delete[] text;
                    return false;
                }

                if (outObj->faces[faceWrite].vertices[i].vtIndex >= outObj->vtCount)
                {
                    ObjDebugPrint("face references texcoord out of range", fileName, lineNumber);
                    ObjFree(outObj);
                    delete[] text;
                    return false;
                }

                if (outObj->faces[faceWrite].vertices[i].vnIndex >= outObj->vnCount)
                {
                    ObjDebugPrint("face references normal out of range", fileName, lineNumber);
                    ObjFree(outObj);
                    delete[] text;
                    return false;
                }
            }

            ++faceWrite;
        }

        *nextLine = saved;

        while (*nextLine == '\r' || *nextLine == '\n')
        {
            if (*nextLine == '\n')
                ++lineNumber;
            ++nextLine;
        }

        line = nextLine;
    }

    delete[] text;
    return true;
}