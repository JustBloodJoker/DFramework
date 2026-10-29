#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),"    \
           "DescriptorTable(SRV(t1, numDescriptors = 14)),"     \
           "CBV(b0),"                                           \
           "SRV(t0),"                                           \
           "SRV(t15),"                                          \
           "CBV(b1),"                                           \
           "SRV(t16),"                                          \
           "CBV(b2),"                                           \
           "RootConstants(num32BitConstants = 1, b3),"          \
           "SRV(t17),"                                          \
           "SRV(t18),"                                          \
           "SRV(t19),"                                          \
           "SRV(t20),"                                          \
           "SRV(t21),"                                          \
           "SRV(t22),"                                          \
           "SRV(t23),"                                          \
           "SRV(t24),"                                          \
           "SRV(t25),"                                          \
           "SRV(t26),"                                          \
           "StaticSampler(s0),"                                 \
           "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP)"
