slangc.exe ../shaders/shader.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -matrix-layout-row-major -entry vert_main -entry frag_main -o ../shaders/slang.spv
