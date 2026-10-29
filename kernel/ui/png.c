#include <png.h>
#include <string.h>

#define PNG_MAX_W 1080u
#define PNG_MAX_H 1920u
#define PNG_RAW_CAP ((PNG_MAX_W * 4u + 1u) * PNG_MAX_H)
#define PNG_Z_CAP (256u * 1024u)
#define HUFF_MAX 15
static uint8_t compressed[PNG_Z_CAP];
static uint8_t raw[PNG_RAW_CAP];
static uint8_t palette[768], palette_alpha[256];
static uint32_t palette_len, alpha_len;

typedef struct { unsigned count[HUFF_MAX + 1]; unsigned short symbol[320]; } huffman_t;
typedef struct { const uint8_t *p; size_t n, at; uint32_t bits; unsigned have; int bad; } bits_t;

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static int tag_is(const uint8_t *a,const char *b) { return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3]; }
static unsigned getbits(bits_t *b, unsigned n) {
    while (b->have < n) { if (b->at >= b->n) { b->bad=1; return 0; } b->bits |= (uint32_t)b->p[b->at++] << b->have; b->have += 8; }
    unsigned v=b->bits & ((1u<<n)-1); b->bits >>= n; b->have -= n; return v;
}
static int build_huff(huffman_t *h, const uint8_t *lens, unsigned n) {
    memset(h,0,sizeof(*h));
    for (unsigned i=0;i<n;i++) { if(lens[i]>HUFF_MAX)return 0; h->count[lens[i]]++; }
    if (h->count[0]==n) return 1;
    int left=1; for(unsigned len=1;len<=HUFF_MAX;len++){ left=(left<<1)-(int)h->count[len]; if(left<0)return 0; }
    unsigned offsets[HUFF_MAX+1]={0};
    for(unsigned len=1;len<HUFF_MAX;len++) offsets[len+1]=offsets[len]+h->count[len];
    for(unsigned sym=0;sym<n;sym++) if(lens[sym]) h->symbol[offsets[lens[sym]]++]=(unsigned short)sym;
    return 1;
}
static int decode_symbol(bits_t *b,const huffman_t *h) {
    unsigned code=0, first=0, index=0;
    for(unsigned len=1;len<=HUFF_MAX;len++) {
        code |= getbits(b,1); if(b->bad)return -1;
        unsigned count=h->count[len];
        if(code>=first && code-first<count) return h->symbol[index+(code-first)];
        index+=count; first=(first+count)<<1; code<<=1;
    }
    b->bad=1; return -1;
}
static int inflate_zlib(const uint8_t *src,size_t n,uint8_t *dst,size_t cap,size_t *outn) {
    if(n<6 || (src[0]&15)!=8 || ((unsigned)src[0]*256+src[1])%31 || (src[1]&32))return 0;
    bits_t b={src+2,n-6,0,0,0,0}; size_t out=0; int final=0;
    static const uint16_t lb[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const uint8_t le[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const uint16_t db[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const uint8_t de[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    while(!final && !b.bad) {
        final=(int)getbits(&b,1); unsigned type=getbits(&b,2); huffman_t lit,dist;
        if(type==0) { b.bits=0;b.have=0; if(b.at+4>b.n)return 0; unsigned len=b.p[b.at]|(b.p[b.at+1]<<8), inv=b.p[b.at+2]|(b.p[b.at+3]<<8);b.at+=4;if((len^(unsigned)0xFFFF)!=inv||b.at+len>b.n||out+len>cap)return 0;memcpy(dst+out,b.p+b.at,len);out+=len;b.at+=len;continue; }
        uint8_t lengths[320]={0};
        if(type==1) {
            for(unsigned i=0;i<=287;i++) lengths[i]=i<=143?8:i<=255?9:i<=279?7:8;
            if(!build_huff(&lit,lengths,288))return 0; for(unsigned i=0;i<32;i++)lengths[i]=5;
            if(!build_huff(&dist,lengths,32))return 0;
        } else if(type==2) {
            unsigned nl=getbits(&b,5)+257, nd=getbits(&b,5)+1, nc=getbits(&b,4)+4;
            static const uint8_t order[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15}; uint8_t cl[19]={0};
            for(unsigned i=0;i<nc;i++)cl[order[i]]=getbits(&b,3);
            huffman_t ch;if(!build_huff(&ch,cl,19))return 0;
            unsigned total=nl+nd, pos=0;
            while(pos<total&&!b.bad) { int s=decode_symbol(&b,&ch); if(s<0)return 0;
                if(s<16)lengths[pos++]=(uint8_t)s;
                else { unsigned rep,val=0;if(s==16){if(!pos)return 0;rep=getbits(&b,2)+3;val=lengths[pos-1];}else if(s==17)rep=getbits(&b,3)+3;else rep=getbits(&b,7)+11;
                    if(pos+rep>total)return 0;while(rep--)lengths[pos++]=(uint8_t)val; }
            }
            if(b.bad||!build_huff(&lit,lengths,nl)||!build_huff(&dist,lengths+nl,nd))return 0;
        } else return 0;
        for(;;) { int sym=decode_symbol(&b,&lit);if(sym<0)return 0;if(sym<256){if(out>=cap)return 0;dst[out++]=(uint8_t)sym;continue;}if(sym==256)break;if(sym<257||sym>285)return 0;
            unsigned li=(unsigned)sym-257, length=lb[li]+getbits(&b,le[li]);int ds=decode_symbol(&b,&dist);if(ds<0||ds>29)return 0;unsigned distance=db[ds]+getbits(&b,de[ds]);if(!distance||distance>out||out+length>cap)return 0;
            while(length--){dst[out]=dst[out-distance];out++;}
        }
    }
    if(b.bad)return 0;*outn=out;return 1;
}
static uint8_t paeth(uint8_t a,uint8_t b,uint8_t c) { int p=(int)a+b-c,pa=p-a,pb=p-b,pc=p-c;if(pa<0)pa=-pa;if(pb<0)pb=-pb;if(pc<0)pc=-pc;return pa<=pb&&pa<=pc?a:pb<=pc?b:c; }

int png_decode(const uint8_t *data,size_t length,uint32_t *pixels,size_t capacity,uint32_t *width,uint32_t *height) {
    static const uint8_t sig[8]={137,80,78,71,13,10,26,10};
    if(!data||!pixels||length<8)return 0;
    for(unsigned i=0;i<8;i++)if(data[i]!=sig[i])return 0;
    uint32_t w=0,h=0;uint8_t depth=0,color=0,interlace=0;size_t zlen=0,at=8;palette_len=0;alpha_len=0;
    while(at+12<=length){uint32_t n=be32(data+at);if(n>length-at-12)return 0;const uint8_t *type=data+at+4,*d=data+at+8;
        if(tag_is(type,"IHDR")){if(n!=13)return 0;w=be32(d);h=be32(d+4);depth=d[8];color=d[9];interlace=d[12];if(d[10]||d[11])return 0;}
        else if(tag_is(type,"IDAT")){if(zlen+n>PNG_Z_CAP)return 0;memcpy(compressed+zlen,d,n);zlen+=n;}
        else if(tag_is(type,"PLTE")){if(n>768||n%3)return 0;memcpy(palette,d,n);palette_len=n;}
        else if(tag_is(type,"tRNS")){if(n>256)return 0;memcpy(palette_alpha,d,n);alpha_len=n;}
        at+=12+n;if(tag_is(type,"IEND"))break;
    }
    unsigned channels=color==0?1:color==2?3:color==3?1:color==4?2:color==6?4:0;
    if(!w||!h||w>PNG_MAX_W||h>PNG_MAX_H||capacity<(size_t)w*h||depth!=8||interlace||!channels||!zlen||(color==3&&!palette_len))return 0;
    size_t stride=(size_t)w*channels, rawlen;if((stride+1)*h>PNG_RAW_CAP)return 0;
    if(!inflate_zlib(compressed,zlen,raw,PNG_RAW_CAP,&rawlen))return 0;
    if(rawlen!=(stride+1)*h)return 0;
    for(uint32_t y=0;y<h;y++){uint8_t *row=raw+(size_t)y*(stride+1)+1,*prev=y?raw+(size_t)(y-1)*(stride+1)+1:NULL;unsigned filter=raw[(size_t)y*(stride+1)];if(filter>4)return 0;
        for(size_t x=0;x<stride;x++){uint8_t a=x>=channels?row[x-channels]:0,b=prev?prev[x]:0,c=prev&&x>=channels?prev[x-channels]:0;uint8_t f=filter==1?a:filter==2?b:filter==3?(uint8_t)(((unsigned)a+b)/2):filter==4?paeth(a,b,c):0;row[x]=(uint8_t)(row[x]+f);}}
    for(size_t i=0;i<(size_t)w*h;i++){uint8_t *p=raw+(i/w)*(stride+1)+1+(i%w)*channels;uint8_t r,g,b,a=255;
        if(color==0)r=g=b=p[0];else if(color==2){r=p[0];g=p[1];b=p[2];}else if(color==3){unsigned ix=p[0];if(ix*3+2>=palette_len)return 0;r=palette[ix*3];g=palette[ix*3+1];b=palette[ix*3+2];if(ix<alpha_len)a=palette_alpha[ix];}else if(color==4){r=g=b=p[0];a=p[1];}else{r=p[0];g=p[1];b=p[2];a=p[3];}
        pixels[i]=((uint32_t)a<<24)|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
    }
    *width=w;*height=h;return 1;
}
