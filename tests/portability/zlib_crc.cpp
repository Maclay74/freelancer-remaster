extern "C" {
#include "../../src/zlib/vendor/crc32.c"
}
#include <cstdio>
#include <vector>
static unsigned Reference(unsigned initial,const unsigned char* data,unsigned length) {
    unsigned crc=~initial;
    for(unsigned index=0;index<length;++index) {
        crc^=data[index];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
int main() {
    const unsigned char known[]="123456789";
    if(crc32(0,known,9)!=0xcbf43926u || crc32(17,nullptr,0)!=0) return 1;
    std::vector<unsigned char> data(65560);unsigned random=17319,cases=0;
    for(auto& byte:data) {random^=random<<13;random^=random>>17;random^=random<<5;byte=static_cast<unsigned char>(random);}
    for(unsigned offset=0;offset<16;++offset) for(unsigned sample=0;sample<100;++sample) {
        const unsigned length=sample<40?sample:(sample*1543u)%65536;
        const unsigned seed=sample%3==0?0:sample%3==1?0xffffffffu:random;
        const auto* bytes=data.data()+offset;
        const unsigned expected=Reference(seed,bytes,length);
        if(crc32_z(seed,bytes,length)!=expected) return 2;
        const unsigned split=length/3;
        const auto first=crc32_z(seed,bytes,split);
        if(crc32_z(first,bytes+split,length-split)!=expected) return 3;
        const auto second=crc32_z(0,bytes+split,length-split);
        if(crc32_combine(first,second,length-split)!=expected) return 4;
        ++cases;
    }
    std::printf("PASS: %u exact portable CRC cases, known vector, streaming and combined checksums\n",cases);
    return 0;
}
