#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUNK_HEADER 0x000003F3UL
#define READ_BUFFER_SIZE 4096
#define SHA256_BLOCK_SIZE 64

struct sha256_ctx {
    unsigned long state[8];
    unsigned long bitlen_hi;
    unsigned long bitlen_lo;
    unsigned char data[SHA256_BLOCK_SIZE];
    unsigned int datalen;
};

static unsigned long rotr32(unsigned long x, unsigned int n)
{
    x &= 0xFFFFFFFFUL;
    return ((x >> n) | (x << (32U - n))) & 0xFFFFFFFFUL;
}

static unsigned long crc32_update(unsigned long crc, const unsigned char *buf, size_t len)
{
    size_t i;
    unsigned int bit;

    crc ^= 0xFFFFFFFFUL;
    for (i = 0; i < len; ++i) {
        crc ^= (unsigned long)buf[i];
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 1UL)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

static unsigned long read_be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) |
           ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) |
           (unsigned long)p[3];
}

static void write_be32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static void sha256_transform(struct sha256_ctx *ctx, const unsigned char data[64])
{
    static const unsigned long k[64] = {
        0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,
        0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
        0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,
        0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
        0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,
        0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
        0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,
        0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
        0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,
        0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
        0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,
        0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
        0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,
        0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
        0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,
        0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL
    };
    unsigned long m[64];
    unsigned long a,b,c,d,e,f,g,h,t1,t2,s0,s1,ch,maj;
    unsigned int i;

    for (i = 0; i < 16; ++i)
        m[i] = read_be32(data + (i * 4));
    for (i = 16; i < 64; ++i) {
        s0 = rotr32(m[i-15],7) ^ rotr32(m[i-15],18) ^ (m[i-15] >> 3);
        s1 = rotr32(m[i-2],17) ^ rotr32(m[i-2],19) ^ (m[i-2] >> 10);
        m[i] = (m[i-16] + s0 + m[i-7] + s1) & 0xFFFFFFFFUL;
    }

    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];

    for (i = 0; i < 64; ++i) {
        s1 = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
        ch = (e & f) ^ ((~e) & g);
        t1 = (h + s1 + ch + k[i] + m[i]) & 0xFFFFFFFFUL;
        s0 = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
        maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = (s0 + maj) & 0xFFFFFFFFUL;
        h=g; g=f; f=e; e=(d+t1)&0xFFFFFFFFUL;
        d=c; c=b; b=a; a=(t1+t2)&0xFFFFFFFFUL;
    }

    ctx->state[0]=(ctx->state[0]+a)&0xFFFFFFFFUL;
    ctx->state[1]=(ctx->state[1]+b)&0xFFFFFFFFUL;
    ctx->state[2]=(ctx->state[2]+c)&0xFFFFFFFFUL;
    ctx->state[3]=(ctx->state[3]+d)&0xFFFFFFFFUL;
    ctx->state[4]=(ctx->state[4]+e)&0xFFFFFFFFUL;
    ctx->state[5]=(ctx->state[5]+f)&0xFFFFFFFFUL;
    ctx->state[6]=(ctx->state[6]+g)&0xFFFFFFFFUL;
    ctx->state[7]=(ctx->state[7]+h)&0xFFFFFFFFUL;
}

static void sha256_init(struct sha256_ctx *ctx)
{
    ctx->datalen=0; ctx->bitlen_hi=0; ctx->bitlen_lo=0;
    ctx->state[0]=0x6a09e667UL; ctx->state[1]=0xbb67ae85UL;
    ctx->state[2]=0x3c6ef372UL; ctx->state[3]=0xa54ff53aUL;
    ctx->state[4]=0x510e527fUL; ctx->state[5]=0x9b05688cUL;
    ctx->state[6]=0x1f83d9abUL; ctx->state[7]=0x5be0cd19UL;
}

static void sha256_add_bits(struct sha256_ctx *ctx, unsigned long bits)
{
    unsigned long old = ctx->bitlen_lo;
    ctx->bitlen_lo = (ctx->bitlen_lo + bits) & 0xFFFFFFFFUL;
    if (ctx->bitlen_lo < old)
        ctx->bitlen_hi = (ctx->bitlen_hi + 1UL) & 0xFFFFFFFFUL;
}

static void sha256_update(struct sha256_ctx *ctx, const unsigned char *data, size_t len)
{
    size_t i;
    for (i=0;i<len;++i) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            sha256_add_bits(ctx, 512UL);
            ctx->datalen=0;
        }
    }
}

static void sha256_final(struct sha256_ctx *ctx, unsigned char out[32])
{
    unsigned int i = ctx->datalen;
    unsigned long tail_bits = ((unsigned long)ctx->datalen) * 8UL;
    sha256_add_bits(ctx, tail_bits);

    ctx->data[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->data[i++] = 0;
        sha256_transform(ctx, ctx->data);
        i=0;
    }
    while (i < 56) ctx->data[i++] = 0;
    write_be32(ctx->data+56, ctx->bitlen_hi);
    write_be32(ctx->data+60, ctx->bitlen_lo);
    sha256_transform(ctx, ctx->data);
    for (i=0;i<8;++i) write_be32(out+(i*4), ctx->state[i]);
}

static const char *classify_file(const unsigned char *head, size_t head_len)
{
    if (head_len >= 4 && read_be32(head) == HUNK_HEADER)
        return "amiga-hunk";
    if (head_len >= 4 && head[0]=='D' && head[1]=='O' && head[2]=='S' && head[3] <= 7)
        return "amiga-dos-bootblock";
    return "unknown-raw";
}

static unsigned int byte_diversity(const unsigned long hist[256])
{
    unsigned int i, used=0;
    for (i=0;i<256;++i) if (hist[i] != 0) ++used;
    return used;
}

static const char *packed_hint(unsigned long total, unsigned int diversity)
{
    if (total >= 1024UL && diversity >= 240U) return "high";
    if (total >= 512UL && diversity >= 208U) return "possible";
    return "low";
}

static void print_sha256(const unsigned char digest[32])
{
    unsigned int i;
    for (i=0;i<32;++i) printf("%02x", (unsigned int)digest[i]);
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [--kv] <file>\n", prog);
}

int main(int argc, char **argv)
{
    FILE *fp;
    unsigned char buf[READ_BUFFER_SIZE], head[16], digest[32];
    unsigned long hist[256];
    size_t n, head_len=0;
    unsigned long total=0, crc=0;
    struct sha256_ctx sha;
    const char *path;
    const char *type;
    unsigned int diversity;
    int kv=0;
    unsigned int i;

    if (argc == 3 && strcmp(argv[1], "--kv") == 0) { kv=1; path=argv[2]; }
    else if (argc == 2) path=argv[1];
    else { usage(argv[0]); return 10; }

    for (i=0;i<256;++i) hist[i]=0;
    sha256_init(&sha);

    fp=fopen(path,"rb");
    if (fp == NULL) { fprintf(stderr,"FileInfo: cannot open '%s'\n",path); return 20; }

    while ((n=fread(buf,1,sizeof(buf),fp)) != 0) {
        size_t j, copy_len=0;
        if (head_len < sizeof(head)) {
            copy_len=sizeof(head)-head_len;
            if (copy_len > n) copy_len=n;
            memcpy(head+head_len,buf,copy_len);
            head_len += copy_len;
        }
        crc=crc32_update(crc,buf,n);
        sha256_update(&sha,buf,n);
        for (j=0;j<n;++j) hist[buf[j]]++;
        total += (unsigned long)n;
    }
    if (ferror(fp)) { fprintf(stderr,"FileInfo: read error on '%s'\n",path); fclose(fp); return 20; }
    fclose(fp);

    sha256_final(&sha,digest);
    type=classify_file(head,head_len);
    diversity=byte_diversity(hist);

    if (kv) {
        printf("file=%s\n",path);
        printf("size=%lu\n",total);
        printf("crc32=%08lX\n",crc & 0xFFFFFFFFUL);
        printf("sha256="); print_sha256(digest); printf("\n");
        printf("type=%s\n",type);
        printf("byte_diversity=%u\n",diversity);
        printf("packed_hint=%s\n",packed_hint(total,diversity));
    } else {
        printf("File: %s\n",path);
        printf("Size: %lu bytes\n",total);
        printf("CRC32: %08lX\n",crc & 0xFFFFFFFFUL);
        printf("SHA256: "); print_sha256(digest); printf("\n");
        printf("Type: %s\n",type);
        printf("Byte diversity: %u/256\n",diversity);
        printf("Packed/compressed hint: %s\n",packed_hint(total,diversity));
    }
    return 0;
}
