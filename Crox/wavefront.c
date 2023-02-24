#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <stb_ds.h>
#include <stb_image.h>
#include <linmath.h>

#include "mesh.h"
#include <assert.h>

#include "framework_winapi.h"


struct WavefrontItems
{
	/*stb_ds*/ struct Vertex* vertices;
};

struct IndexKV
{
	uint32_t key[3];	//Chunk
	uint32_t value;		//Vertex index
};


#define GEOM_ELEM_SIZE 3
#define NORM_ELEM_SIZE 3
#define TEXT_ELEM_SIZE 2

/**
	@brief 
	@param file - containing content to be read.
	@returns    - stb ds Vertex AOS and indices. Indices are ONLY triangles 
**/
struct Vertex* wavefront_read(_In_ FILE* file)
{
	vec3* geometries	= NULL; // 3 values per geometry
	vec2* textures		= NULL; // 3 values per texture
	vec3* normals		= NULL; // 2 values per normal


	struct Vertex* vertices = NULL;
	uint32_t* indices = NULL;

	struct IndexKV* indexTable = NULL;

	const size_t LINE_BUF_SIZE = 512;
	char* lineBuf = malloc(sizeof * lineBuf * LINE_BUF_SIZE);
	if (!lineBuf)
	{
		return; //Note to user: check errno
	}

	
	
	bool isError = false;
	bool isEOF = false;
	while (!isEOF && !isError)
	{
		/*Handle Lines*/

		// Zero terminated string inside the buffer
		const char* line = fgets(lineBuf, LINE_BUF_SIZE, file);
		if (line == NULL)
		{
			isError	= ferror(file);
			isEOF	= feof(file);
			if (isEOF)
			{
				break;
			}
			if (isError)
			{

				break;
			}
		}
		char* it = line;

		/* Process lines */
		switch (*it++)
		{

		// Faces
		case 'f':	// TODO: 1) indices, 2) polygon triangulation
		{
			struct Vertex v;
			memset(&v, 0, sizeof v);

			for (uint32_t i = 0; i < 3; i++)
			{
				it = strchr(it, ' ')+1;

				int ixG = 0;
				int ixT = 0;
				int ixN = 0;

				int result;
				if (result = sscanf_s(it, "%d/%d/%d", &ixG, &ixT, &ixN))
				{
					
				}
				else if (result = sscanf_s(it, "%d", &ixG))
				{

				}
				else if (result = sscanf_s(it, "%d/%d", &ixG, &ixT))
				{

				}
				else if (result = sscanf_s(it, "%d//%d", &ixG, &ixN))
				{

				}
				else
				{
					isError = true;
				}

				assert(ixG != 0);

				ixG = ixG < 0 ? arrlen(geometries) - ixG : ixG;
				ixT = ixT < 0 ? arrlen(textures) - ixT : ixT;
				ixN = ixN < 0 ? arrlen(normals) - ixN : ixN;

				if (ixG)
					memcpy_s(
						v.pos, sizeof(vec3),
						&geometries[ixG-1], GEOM_ELEM_SIZE * sizeof(float));
				else memset(v.pos, 0, sizeof(vec3)); // It's illegal for a face to not have a geometrical position

				if (ixT)
					memcpy_s(
						v.texcoord0, sizeof(vec2),
						&textures[ixT-1], TEXT_ELEM_SIZE * sizeof(float));
				else memset(v.texcoord0, 0, sizeof(vec2));

				if (ixN)
					memcpy_s(
						v.normal, sizeof(vec3),
						&normals[ixN-1], NORM_ELEM_SIZE * sizeof(float));
				else memset(v.normal, 0, sizeof(vec3));


				arrput(vertices, v);
			}


			break;
		}


		//Vertex Data
		case 'v':
		{
			uint32_t elemCount ;/*PENDING*/
			float** pList;		/*PENDING*/

			switch (*it++)
			{
			case ' ': //Geometry
			{
				pList = &geometries;
				elemCount = GEOM_ELEM_SIZE;
				it--; //backtrack to make it work like the other ones.
				break;
			}
			case 't': //Texture
			{
				pList = &textures;
				elemCount = TEXT_ELEM_SIZE;
				break;
			}
			//Normal
			case 'n': {
				pList = &normals;
				elemCount = NORM_ELEM_SIZE;
				break;
			}
			// Free Form Geometry Vertex space
			case 'p':{
				//TODO:
				continue;
			}

			default:
				isError = true;
				errno = EILSEQ;
				continue;
			}
			
			float value = 0;
			for (uint32_t i = 0; i < elemCount; i++)
			{
				it = strchr(it, ' ')+1; //skip to value
				uint32_t result;
				if (result = sscanf_s(it, "%f", &value))
				{
					
				}
				else if (result == EOF )
				{
					isError = true;
					break;
				}

				arrput(*pList, value);
				
			}

			break;
		}
		
		

		case '#': // Ignore comments.
		{
			continue;
		}
		default: // TODO (ignored for now)
			//isError = true;
			//errno = EILSEQ;
			continue;
		}

		

	}

	free(lineBuf);
	arrfree(geometries);
	arrfree(textures);
	arrfree(normals);
	hmfree(indexTable);


	return vertices;
}