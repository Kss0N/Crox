# Command Line Arguments:
#
# py "$(SolutionDir)cxShaderBuildEventScript\cxShaderBuildEventScript.py" "$(ProjectDir)." "$(OutDir)." 
#
# argv[1] = $(ProjectDir) shader containing project name
# argv[2] = $(OutDir)
#

import os
from os import path
import subprocess
import sys

assert(os.environ['VULKAN_SDK'] is not None)

glslExtensions = [
	".vert",
	".tesc",
	".tese",
	".geom",
	".frag",
	".comp"
]

ShaderPath	= (sys.argv[1])[:-2]
OutDir		= (sys.argv[2])[:-2]


def procedure():

	for p in os.listdir(ShaderPath):
		fullPath = path.join(ShaderPath, p);
		if path.isfile(fullPath):

			fileName = path.basename(p)
			name, ext = path.splitext(fileName)
			outputFileName = f"{name}_{ext.replace('.','')}.spv"
			
			if(ext in glslExtensions):
				print(f"\tCompiling {fileName}")
				#os.remove(path.join(OutDir, outputFileName))
				subprocess.call([path.join(os.environ['VULKAN_SDK'], 'bin', 'glslc'), fullPath, '-o', path.join(OutDir, outputFileName) ])

			

	return 

if __name__ == "__main__":
	procedure() 