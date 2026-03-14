#define RS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
           "CBV(b0)," \
           "CBV(b1)," \
           "DescriptorTable(SRV(t0, numDescriptors = unbounded))," \
           "StaticSampler(s0)," \
           "SRV(t9, space = 1)"
