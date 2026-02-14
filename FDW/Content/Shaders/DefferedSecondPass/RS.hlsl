#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"    \
           "DescriptorTable(SRV(t1, numDescriptors = 9)),"      \
           "CBV(b0),"                                           \
           "SRV(t0),"                                           \
           "SRV(t10),"                                          \
           "CBV(b1),"                                           \
           "SRV(t11),"                                          \
           "CBV(b2),"                                           \
           "StaticSampler(s0),"                                  \
           "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP)"