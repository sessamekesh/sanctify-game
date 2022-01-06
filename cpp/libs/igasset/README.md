Asset processing should be independent of rendering API, since so many of them have similar
concepts (vertex buffers containing at least data from which a vertex shader can generate
positional data, etc)

This is NOT ALLOWED to have an OpenGL / Vulkan / WebGL / WebGPU dependency!!!

It only has to deal with processing assets, etc.