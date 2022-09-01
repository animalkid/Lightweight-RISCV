#ifndef ALGORITHM_SEL
#define ALGORITHM_SEL

#define AEAD 0
#define HASH 0
#define AUTH 0
#define AES 1
#define SHA256 0

#if (AES)

#define SOFTORHARD 0 //0 for software 1 for hardware

#define ECB 1
#define CBC 0
#define GCM 0
#define CTR 0

#define BIT128 1
#define BIT192 0
#define BIT256 0

#if (ECB && CBC) || (ECB && GCM) || (ECB && CTR) || (CBC && GCM) || (CBC && CTR) || (GCM && CTR) || (!ECB && !CBC && !GCM && !CTR)
#error choose only one AES type
#endif

#if (CTR && SOFTORHARD)
#error ctr is not available for hardware
#endif

#if (GCM && !SOFTORHARD)
#error gcm is not available for software
#endif

#if (BIT128 && BIT192) || (BIT128 && BIT256) || (BIT192 && BIT256) || (!BIT128 && !BIT192 && !BIT256)
#error choose only one BIT quantity
#endif

#endif

#if (AEAD && HASH) || (AEAD && AUTH) || (AEAD && AES) || (AEAD && SHA256) || (HASH && AUTH) || (HASH && AES) || (HASH && SHA256) || (AUTH && AES) || (AUTH && SHA256) || (AES && SHA256) || (!AEAD && !HASH && !AUTH && !AES && !SHA256)
#error choose only one algorithm
#endif

#endif
