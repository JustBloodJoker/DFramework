#define RS  "SRV(t0)," \       
            "DescriptorTable(UAV(u0))," \    
            "DescriptorTable(SRV(t1, numDescriptors=2))," \
            "CBV(b1)," \
            "SRV(t3)," \
            "CBV(b2)," \  
            "DescriptorTable(SRV(t4))," \ 
            "DescriptorTable(UAV(u1))," \
            "SRV(t5)," \
            "CBV(b3)," \
            "StaticSampler(s0)"