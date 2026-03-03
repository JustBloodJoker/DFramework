#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
           "DescriptorTable(SRV(t0, numDescriptors = 6))," \
           "CBV(b0)," \
           "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP)," \
           "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP)"