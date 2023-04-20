#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC 1
#include <crtdbg.h>
#endif // _DEBUG

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <stb_ds.h>
#include <stb_image.h>
#include <linmath.h>
#include <assert.h>

#include "mesh.h"


#include "wavefront.h"


// A chunk represents indices used for crafting vertices. 
struct IndexChunk
{
	uint32_t ixG;
	uint32_t ixT;
	uint32_t ixN;
};

struct IndexKV
{
	struct IndexChunk key;	//Chunk
	uint32_t value;		//Vertex index
};


#define GEOM_ELEM_SIZE 3
#define NORM_ELEM_SIZE 3
#define TEXT_ELEM_SIZE 2


/**
	@brief  Checks if str starts with pfx.
**/
inline bool startsWith(_In_z_ const char* str, _In_z_ const char* pfx)
{
	return strncmp(pfx, str, strlen(pfx)) == 0;
}

/**
	@brief  removes the line-feed from the line. The line terminator may be LF, CR, CRLF or LFCR.  
	@param  line - to remove line break from.
**/
inline void removeLineFeed(_In_z_ char* line)
{
	line[strcspn(line, "\r\n")] = '\0';
}


#define EXIT(type) break;
#define BACK_TO_TOP continue

#ifdef _DEBUG
#define CHECK(exp) assert(exp)
#else
#define CHECK(exp) (exp)
#endif // _DEBUG



struct WavefrontObj wavefront_obj_read(_In_ FILE* file)
{
	//to return: 

	struct WavefrontGroup* groups = NULL; /*stb_ds darray*/
	struct Vertex* vertices = NULL; /*stb_ds darray*/
	uint32_t* indices = NULL; /*stb_ds darray*/
	_Null_terminated_ char* mtllib = NULL; /*Path to mtllib used*/
	_Null_terminated_ char* name = NULL; /*o [name]*/

	// Utilities:

	vec3* geometries = NULL; /*stb_ds darray*/
	vec2* textures = NULL; /*stb_ds darray*/
	vec3* normals = NULL; /*stb_ds darray*/
	struct IndexKV* indexTable = NULL; /*stb_ds hashmap*/
	
	struct WavefrontGroup activeGroup;
	memset(&activeGroup, 0, sizeof activeGroup);

	const int LINEBUF_SIZE = 512;
	char* lineBuf = malloc(sizeof * lineBuf * LINEBUF_SIZE);

	errno_t error = 0;
	bool isError = false;
	bool isEOF = false;
	while (!isEOF && !isError && error == 0)
	{
		char* line = fgets(lineBuf, LINEBUF_SIZE, file);
		
		if (line == NULL)
		{
			if (isEOF = feof(file))
				break;
			else if (isError = ferror(file))
			{
				//TODO: Check if error is recoverable.
				break;
			}
			continue;
		}
		else
			removeLineFeed(line);



		const char* it = line;	//iterator through the line
		switch (*it++)
		{
		
		case 'f':	//Face data 
		{
			//TODO: Face Triangulation

			for (uint32_t i = 0; i < 3; i++)
			{
				//spaces between face index chunks
				it = strchr(it, ' ') + 1;

				signed int
					ixG = 0,
					ixT = 0,
					ixN = 0;

				int result = 0;

				if ((result = sscanf_s(it, "%d/%d/%d",	&ixG, &ixT, &ixN))  != 3 &&
					(result = sscanf_s(it, "%d/%d",		&ixG, &ixT))		!= 2 &&
					(result = sscanf_s(it, "%d",			&ixG))				!= 1 &&
					(result = sscanf_s(it, "%d//%d",		&ixG, &ixN))		!= 2 &&
					(result == EOF))
				{
					isError = true;
					break;
				}


				struct IndexChunk chunk = { 
					ixG < 0 ? (uint32_t)arrlenu(geometries) + ixG : ixG - 1, 
					ixT < 0 ? (uint32_t)arrlenu(textures)	+ ixT : ixT - 1,
					ixN < 0 ? (uint32_t)arrlenu(normals)	+ ixN : ixN - 1,
				};
				uint32_t index;

				if (hmgeti(indexTable, chunk) == -1)
				{
					// The chunk does not exist. A new Vertex has to be created.
					
					struct Vertex vertex;

					if (ixG != 0)
						CHECK((error = memcpy_s(
							vertex.pos, 
							sizeof vertex.pos,
							&geometries[chunk.ixG], 
							sizeof * geometries)) == 0);
					else memset(vertex.pos, 0, sizeof vertex.pos);

					if (ixT != 0)
						CHECK((error = memcpy_s(
							vertex.texcoord0, 
							sizeof vertex.texcoord0,
							&textures[chunk.ixT], 
							sizeof * textures)) == 0);
					else memset(vertex.texcoord0, 0, sizeof vertex.texcoord0);

					if (ixN != 0)
						CHECK((error = memcpy_s(
							vertex.normal,
							sizeof vertex.normal,
							normals[chunk.ixN],
							sizeof * normals)) == 0);
					else memset(vertex.normal, 0, sizeof vertex.normal);

					arrput(vertices, vertex);

					index = (uint32_t)arrlenu(vertices) - 1;
					hmput(indexTable, chunk, index);
				}
				else 
					index = hmget(indexTable, chunk);

				arrput(indices, index);
			}

			if (isError) continue;

			//TODO: Face triangulation

			break;
		}
		
		case 'v':	//Vertex Data
		{
			uint32_t elemCount = PENDING;
			float** pList = PENDING_PTR;

			switch (*it++){
			// Geometry Data
			case ' ': {
				elemCount = GEOM_ELEM_SIZE;
				pList = (float**) & geometries;
				it--;
				break;
			}
			// Texture Data
			case 't': {
				elemCount = TEXT_ELEM_SIZE;
				pList = (float**) & textures;
				break;
			}
			// Normal Data
			case 'n': {
				elemCount = NORM_ELEM_SIZE;
				pList = (float**) & normals;
				break;
			}
			//Vertex Space data (NOT SUPPORTED [FIXME])
			case 'p': {
				//TODO
				continue;
			}
			default:
				isError = true;
				continue;
			}
			assert(elemCount != PENDING);
			assert(pList != PENDING_PTR);

			for (uint32_t i = 0; i < elemCount; i++)
			{
				it = strchr(it, ' ') + 1;

				float value;
				int result = 0;
				if (result = sscanf_s(it, "%f", &value))
					assert(result != EOF);
				else {
					isError = true;
					break;
				}
				arrput(*pList, value);
			}
			if (isError) continue;

			break;
		}
		
		case 'g':	//Group specification
		{
			// 1) finalize prev group
			// 2) begin new
			it = strchr(line, ' ') + 1;

			activeGroup.count = (uint32_t)arrlen(indices)- activeGroup.ixFirst;
			if (activeGroup.count <= 0 && activeGroup.name == NULL && activeGroup.mtlName == NULL)
			{
				// There was no prev group, do nothing
			}
			else // append prev group
				arrput(groups, activeGroup);
			
			memset(&activeGroup, 0, sizeof activeGroup);
			activeGroup.ixFirst = (uint32_t)arrlenu(indices);

			//If there's a name, remainder after trainling space is the name of the new group.
			const char* name = it != NULL ? it : "noname";
			
			size_t nameLen = strlen(it)+1;//name's length + nul terminator
			activeGroup.name = _aligned_malloc(nameLen, 8);
			if (!activeGroup.name)
			{
				isError = true;
				continue;
			}

			CHECK((error = strcpy_s(activeGroup.name, nameLen, it)) == 0);

			break;
		}

		case '#':	// Ignore comments
			break;

		case 'o':	// Object Name specification
		{
			if (name) _aligned_free(name);

			//Everything after the trailing space is considered the name of the object
			it = strchr(it, ' ') + 1;

			size_t nameLen = strlen(it) + 1; //length of name including null terminator
			name = _aligned_malloc(nameLen, 8);
			if (!name)
			{
				isError = true;
				BACK_TO_TOP;
			}

			errno_t err = strcpy_s(name, nameLen, it);
			assert(err == 0);

			continue;
		}
		
		default:
			if (startsWith(line, "mtllib"))
			{
				if (mtllib) _aligned_free(mtllib);

				//Everything after the trailing space is considered the path to the materials-library
				it = strchr(line, ' ') + 1;

				size_t pathLen = strlen(it)+1; //length of the path including NUL terminator
				mtllib = _aligned_malloc(pathLen, 8);
				if (!mtllib)
				{
					isError = true;
					continue;
				}

				CHECK((error = strcpy_s(mtllib, pathLen, it)) == 0);
			}
			else if (startsWith(line, "usemtl"))
			{
				if (activeGroup.mtlName) _aligned_free(activeGroup.mtlName);
				
				//Everything after the trailing space is considered the material-name
				it = strchr(line, ' ') + 1;
				
				size_t len = strlen(it)+1; //length of the name, including NUL terminator
				activeGroup.mtlName = _aligned_malloc(len, 8);
				if (!mtllib)
				{
					isError = true;
					continue;
				}

				CHECK((error = strcpy_s(activeGroup.mtlName, len, it)) == 0);
			}


			break;;
		}

	}

	if (isError)
		assert(isError);

	activeGroup.count = (uint32_t)arrlenu(indices) - activeGroup.ixFirst;
	
	if (activeGroup.count != 0 && arrlenu(indices) > 0)
	{
		arrput(groups, activeGroup);
	}

	// Free dynamic objects

	free(lineBuf);
	hmfree(indexTable);
	arrfree(geometries);
	arrfree(textures);
	arrfree(normals);


	return (struct WavefrontObj) {
		vertices, indices, groups, name, mtllib
	};
}


struct WavefrontMtllib wavefront_mtl_read(_In_ FILE* file)
{
	// returns:
	
	struct WavefrontMtllibKV* materialsKV = NULL; // stb_ds string map
	const char** keys = NULL;
	// Utilities:

	const int LINEBUF_SIZE = 512;
	char* lineBuf = _aligned_malloc(sizeof * lineBuf * LINEBUF_SIZE, 8);

	_Null_terminated_ char* name = NULL;// name of the material
	struct WavefrontMtl active;			// Active material to read
	memset(&active, 0, sizeof active);

	errno_t error;
	bool isEOF = false;
	bool isError = false;
	while (!isEOF && !isError)
	{
		char* line = fgets(lineBuf, LINEBUF_SIZE, file);

		if (line == NULL)
		{
			if (isEOF = feof(file))
				break;
			else if (isError = ferror(file))
			{
				//TODO: Check if error is recoverable.
				break;
			}
			continue;
		}
		else
			removeLineFeed(line);

		const char* it = line; // iterator through the line.
		switch (*it++)
		{
		case 'N': // Scalars
		{
			float* pVal;
			switch (*it++)
			{
			case 's':
			{
				pVal = &active.shininess;
				break;
			}
			default:
				//isError = true; // TODO
				continue;
			}

			int result;
			it = strchr(it, ' ') + 1;
			if (!(result = sscanf_s(it, "%f", pVal)))
			{
				isError = true;
				continue;
			}

			break;
		}
		case 'K': // vectors
		{
			float* vec;
			switch (*it++)
			{
			//ambient
			case 'a':
			{
				vec = active.ambient;
				break;
			}
			//diffuese
			case 'd':
			{
				vec = active.diffuse;
				break;
			}
			//specular color
			case 's':
			{
				vec = active.specular;
				break;
			}

			default:
				//isError = true; //TODO
				continue;
			}

			for (uint32_t i = 0; i < 3; i++)
			{
				it = strchr(it, ' ') + 1;

				float component;
				int result;
				if ( !(result = sscanf_s(it, "%f", &component)) )
				{
					isError = true;
					break;
				}

				vec[i] = component;
			}
			if (isError) continue;

			break;
		}
		
		case 'd': // dissolvency
		{
			int result;
			if (!(result = sscanf_s(it, "%f", &active.alpha)))
			{
				isError = true;
				continue;
			}

			break;
		}

		case '#': // ignore comments
		{
			continue;
		}
		
		default:
			if (startsWith(line, "newmtl"))
			{
				if (name) 
				{

					shput(materialsKV, name, active);
					arrput(keys, name);

					memset(&active, 0, sizeof active);
				}

				it = strchr(it, ' ') + 1;
				size_t len = strlen(it)+1; //material name length including nul terminator
				name = _aligned_malloc(len, 8);
				CHECK((error = strcpy_s(name, len, it)) == 0);

				break;
			}

			//Transparency  [NOTE: Tr = 1 - d]
			else if (startsWith(line, "Tr"))
			{
				it = strchr(it, ' ') + 1;

				int result;
				float transparency;
				if (!(result = sscanf_s(it, "%f", &transparency)))
				{
					isError = true;
					continue;
				}
				active.alpha = 1 - transparency;

				break;
			}

			break;
		}


	}

	shput(materialsKV, name, active);
	arrput(keys, name);

	_aligned_free(lineBuf);

	return (struct WavefrontMtllib) { materialsKV, keys };
}
