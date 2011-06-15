//
// Hamming SECDEC ECC Implementation for 32-bit register architectures
//   [72,64] 64-bit Data, +7-bit ECC Field, +1 overall parity bit for 2-bit error detection
//
// Author: andrew.allan@analog.com
//
// Version 1.02  aja/jm 	6/09/2006	Adapted to new data structure definition, fixed cp/paste error
// Version 1.01  aja 		5/02/2006 	Major Rework - Unification with DMTC calling convention and struct
//                             			Cleanup in ecc[7] calculation (smaller code)
// Version 0.93  aja 		4/21/2006 	2-bit Error detection Working
// Version 0.92  aja 		4/20/2006 	First Version Verified for SEC (Single Error Correction)
//

#include <stdio.h>
#include <stdlib.h>

typedef struct tag_U64
{
	unsigned long l; 	// data bits: (msb) 0,1,2...,31 (lsb)
	unsigned long h; 	// data bits: (msb) 32,33,...,63 (lsb)	
} DU64;

// Protos
int HammingCode7264(DU64 *InputU64, unsigned int fn, unsigned int ecc);
unsigned int ecc_64h(DU64 *InputU64, unsigned int ecc_l, unsigned int fn, unsigned int ecc_ll);
unsigned int ecc_64l(DU64 *InputU64);


#define HAMencode 0
#define HAMdecode 1
#define MAX_SYNDROME 73

// Globals

//section ("otp_ecc_data") 
char syndrome_xlate[MAX_SYNDROME] = {-1,-1,-1, 0,-1, 1, 2, 3,-1, 4, 5, 6, 7, 8, 9,10,-1,
                                       11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,
                                       26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,
                                       42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,-1,
                                       57,58,59,60,61,62,63};

#ifdef MOAB
#define PTR_JTAB_SYNDROME_XLATE  (0xEF000044)
#endif

//--------------------------------------------------------------------------
 int HammingCode7264(DU64 *InputU64, unsigned int fn, unsigned int ecc)
{
 unsigned int ecc_calc;
 unsigned int syndrome;
 unsigned int corrbit;
 int corrnum;
 int Error;
#ifdef MOAB
 char *local_syndrome_xlate;
#endif

  Error=0x0; // Default to "no Error"

  ecc_calc = ecc_64l(InputU64);
  ecc_calc = ecc_64h(InputU64,ecc_calc, fn, ecc);
  
  if(fn == HAMencode)
   return(ecc_calc);
    

  // Init Defaults
  corrnum = -2; // Default 

  syndrome = ecc^ecc_calc;


  if((syndrome&0x80) == 0x0)  // 7th bit
   {
    if((syndrome&0x7F) != 0x00)
     Error=0x02; // Double Error
   // else no error
   }
  else
   {
    if((syndrome&0x7F)>MAX_SYNDROME) // Invalid syndrome
     {
      Error=0x3;
     }
    else
     {
#ifdef MOAB
       local_syndrome_xlate = *(char **)(PTR_JTAB_SYNDROME_XLATE);
       corrnum = local_syndrome_xlate[syndrome&0x07F];       // Only 7 LSBs for syndrome lookup
#else
       corrnum = syndrome_xlate[syndrome&0x07F];       // Only 7 LSBs for syndrome lookup
#endif
       Error=0x01; // Single Error
     }

   }

#if DEBUG
  printf("syndrome[7]=%02X syndrome[6:0]=%02X corrnum=%d Error=%d\n", (syndrome&0x80)>>7, syndrome&0x7F, corrnum, Error);
#endif

  if(Error!=0x01 || corrnum<0)
   return(Error); // No correction possible or needed

  if(corrnum<32)
   {
    corrbit = 0x1U<<corrnum;
    InputU64->l ^= (corrbit&0x0FFFFFFFFUL);
   }
  else
   {
    corrbit = 0x1U<<(corrnum-32);
    InputU64->h ^= (corrbit&0x0FFFFFFFFUL);
   }

  return(Error);

}


//--------------------------------------------------------------------------
unsigned int ecc_64l(DU64 *InputU64)
{
 register unsigned long d;
 register unsigned int res;
 register unsigned int ecc;
#if size_opt
 int i;
#endif

 // Calculate the parity bit 1
 d=InputU64->l;
 res=d;       // Bit 0  (Seed)
 res^=d>>=1;  // Bit 1
 res^=d>>=2;  // Bit 3
 res^=d>>=1;  // Bit 4
 res^=d>>=2;  // Bit 6
 res^=d>>=2;  // Bit 8
 res^=d>>=2;  // Bit 10
 res^=d>>=1;  // Bit 11
#if size_opt
 for(i=0;i<7;i++)
  res^=d>>=2;
#else
 res^=d>>=2;  // Bit 13
 res^=d>>=2;  // Bit 15
 res^=d>>=2;  // Bit 17
 res^=d>>=2;  // Bit 19
 res^=d>>=2;  // Bit 21
 res^=d>>=2;  // Bit 23
 res^=d>>=2;  // Bit 25
#endif
 res^=d>>=1;  // Bit 26
 res^=d>>=2;  // Bit 28
 res^=d>>=2;  // Bit 30
 ecc = res&0x01; // Init ECC bit [0]

 // Calculate the parity bit 2
 d=InputU64->l;
 res=d;       // Bit 0  (Seed)
 res^=d>>=2;  // Bit 2
 res^=d>>=1;  // Bit 3
 res^=d>>=2;  // Bit 5
 res^=d>>=1;  // Bit 6
 res^=d>>=3;  // Bit 9
 res^=d>>=1;  // Bit 10
 res^=d>>=2;  // Bit 12
#if size_opt
 for(i=0;i<3;i++)
  {
   res^=d>>=1;
   res^=d>>=3;
  }
#else
 res^=d>>=1;  // Bit 13
 res^=d>>=3;  // Bit 16
 res^=d>>=1;  // Bit 17
 res^=d>>=3;  // Bit 20
 res^=d>>=1;  // Bit 21
 res^=d>>=3;  // Bit 24
#endif
 res^=d>>=1;  // Bit 25
 res^=d>>=2;  // Bit 27
 res^=d>>=1;  // Bit 28
 res^=d>>=3;  // Bit 31
 ecc |= ((res&0x1)<<1); // Save ecc bit [1]

 // Calculate the parity bit 4
 d=InputU64->l;
 res=d>>=1;   // Bit 1  (Seed)
 res^=d>>=1;  // Bit 2
 res^=d>>=1;  // Bit 3
 res^=d>>=4;  // Bit 7
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 8
 res^=d>>=1;  // Bit 9
 res^=d>>=1;  // Bit 10
#endif
 res^=d>>=4;  // Bit 14
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 15
 res^=d>>=1;  // Bit 16
 res^=d>>=1;  // Bit 17
#endif
 res^=d>>=5;  // Bit 22
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 23
 res^=d>>=1;  // Bit 24
 res^=d>>=1;  // Bit 25
#endif
 res^=d>>=4;  // Bit 29
 res^=d>>=1;  // Bit 30
 res^=d>>=1;  // Bit 31
 ecc |= ((res&0x1)<<2); // Save ecc bit [2]

 // Calculate the parity bit 8
 d=InputU64->l;
 res=d>>=4;   // Bit 4  (Seed)
#if size_opt
 for(i=0;i<6;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 5
 res^=d>>=1;  // Bit 6
 res^=d>>=1;  // Bit 7
 res^=d>>=1;  // Bit 8
 res^=d>>=1;  // Bit 9
 res^=d>>=1;  // Bit 10
#endif 
 res^=d>>=8;  // Bit 18
#if size_opt
 for(i=0;i<7;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 19
 res^=d>>=1;  // Bit 20
 res^=d>>=1;  // Bit 21
 res^=d>>=1;  // Bit 22
 res^=d>>=1;  // Bit 23
 res^=d>>=1;  // Bit 24
 res^=d>>=1;  // Bit 25
#endif
 ecc |= ((res&0x1)<<3); // Save ecc bit [3]

 // Calculate the parity bit 16
 d=InputU64->l;
 res=d>>=11;  // Bit 11  (Seed)
#if size_opt
 for(i=0;i<14;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 12
 res^=d>>=1;  // Bit 13
 res^=d>>=1;  // Bit 14
 res^=d>>=1;  // Bit 15
 res^=d>>=1;  // Bit 16
 res^=d>>=1;  // Bit 17
 res^=d>>=1;  // Bit 18
 res^=d>>=1;  // Bit 19
 res^=d>>=1;  // Bit 20
 res^=d>>=1;  // Bit 21
 res^=d>>=1;  // Bit 22
 res^=d>>=1;  // Bit 23
 res^=d>>=1;  // Bit 24
 res^=d>>=1;  // Bit 25
#endif
 ecc |= ((res&0x1)<<4); // Save ecc bit [4]

 // Calculate the parity bit 32
 d=InputU64->l;
 res=d>>=26;  // Bit 26  (Seed)
#if size_opt
 for(i=0;i<5;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 27
 res^=d>>=1;  // Bit 28
 res^=d>>=1;  // Bit 29
 res^=d>>=1;  // Bit 30
 res^=d>>=1;  // Bit 31
#endif
 ecc |= ((res&0x1)<<5); // Save ecc bit [5]


 // ecc bit [6] does not use any lower 32 data bits


 // Calculate the overall parity bit
 d=InputU64->l;
 res=d;       // Bit 0  (Seed)
#if size_opt
 for(i=0;i<31;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 1
 res^=d>>=1;  // Bit 2
 res^=d>>=1;  // Bit 3
 res^=d>>=1;  // Bit 4
 res^=d>>=1;  // Bit 5
 res^=d>>=1;  // Bit 6
 res^=d>>=1;  // Bit 7
 res^=d>>=1;  // Bit 8
 res^=d>>=1;  // Bit 9
 res^=d>>=1;  // Bit 10
 res^=d>>=1;  // Bit 11
 res^=d>>=1;  // Bit 12
 res^=d>>=1;  // Bit 13
 res^=d>>=1;  // Bit 14
 res^=d>>=1;  // Bit 15
 res^=d>>=1;  // Bit 16
 res^=d>>=1;  // Bit 17
 res^=d>>=1;  // Bit 18
 res^=d>>=1;  // Bit 19
 res^=d>>=1;  // Bit 20
 res^=d>>=1;  // Bit 21
 res^=d>>=1;  // Bit 22
 res^=d>>=1;  // Bit 23
 res^=d>>=1;  // Bit 24
 res^=d>>=1;  // Bit 25
 res^=d>>=1;  // Bit 26
 res^=d>>=1;  // Bit 27
 res^=d>>=1;  // Bit 28
 res^=d>>=1;  // Bit 29
 res^=d>>=1;  // Bit 30
 res^=d>>=1;  // Bit 31
#endif
 ecc |= ((res&0x1)<<7);  // Save ECC bit [7]


 return ecc;
}


//--------------------------------------------------------------------------
 unsigned int ecc_64h(DU64 *InputU64, unsigned int ecc_l, unsigned int fn, unsigned int ecc_ll)
{
 register unsigned long d;
 register unsigned int res;
 register unsigned int ecc;
 register unsigned int ecc2;
#if size_opt
 int i;
#endif

 // Calculate the parity bit 1
 d=InputU64->h;
 res=d^ecc_l; // Bit 32 XOR ecc_l (bit 0)
#if size_opt
 for(i=0;i<12;i++)
  res^=d>>=2;
#else
 res^=d>>=2;  // Bit 34
 res^=d>>=2;  // Bit 36
 res^=d>>=2;  // Bit 38
 res^=d>>=2;  // Bit 40
 res^=d>>=2;  // Bit 42
 res^=d>>=2;  // Bit 44
 res^=d>>=2;  // Bit 46
 res^=d>>=2;  // Bit 48
 res^=d>>=2;  // Bit 50
 res^=d>>=2;  // Bit 52
 res^=d>>=2;  // Bit 54
 res^=d>>=2;  // Bit 56
#endif
 res^=d>>=1;  // Bit 57
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=2;
#else
 res^=d>>=2;  // Bit 59
 res^=d>>=2;  // Bit 61
 res^=d>>=2;  // Bit 63
#endif
 ecc = res&0x01; // Init ECC bit [0]

 // Calculate the parity bit 2
 d=InputU64->h;
 res=d^(ecc_l>>1);   // Bit 32 XOR ecc_l (bit 1)
 res^=d>>=3;  // Bit 35
#if size_opt
 for(i=0;i<5;i++)
  {
   res^=d>>=1;
   res^=d>>=3;
  }
#else
 res^=d>>=1;  // Bit 36
 res^=d>>=3;  // Bit 39
 res^=d>>=1;  // Bit 40
 res^=d>>=3;  // Bit 43
 res^=d>>=1;  // Bit 44
 res^=d>>=3;  // Bit 47
 res^=d>>=1;  // Bit 48
 res^=d>>=3;  // Bit 51
 res^=d>>=1;  // Bit 52
 res^=d>>=3;  // Bit 55
#endif
 res^=d>>=1;  // Bit 56
 res^=d>>=2;  // Bit 58
 res^=d>>=1;  // Bit 59
 res^=d>>=3;  // Bit 62
 res^=d>>=1;  // Bit 63
 ecc |= ((res&0x1)<<1); // Save ecc bit [1]

 // Calculate the parity bit 4
 d=InputU64->h;
 res=d^(ecc_l>>2);   // Bit 32 XOR ecc_l (bit 2)
 res^=d>>=5;  // Bit 37
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 38
 res^=d>>=1;  // Bit 39
 res^=d>>=1;  // Bit 40
#endif
 res^=d>>=5;  // Bit 45
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 46
 res^=d>>=1;  // Bit 47
 res^=d>>=1;  // Bit 48
#endif
 res^=d>>=5;  // Bit 53
 #if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 54
 res^=d>>=1;  // Bit 55
 res^=d>>=1;  // Bit 56
#endif
 res^=d>>=4;  // Bit 60
#if size_opt
 for(i=0;i<3;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 61
 res^=d>>=1;  // Bit 62
 res^=d>>=1;  // Bit 63
#endif
 ecc |= ((res&0x1)<<2); // Save ecc bit [2]

 // Calculate the parity bit 8
 d=InputU64->h;
 res=(d>>=1)^(ecc_l>>3);   // Bit 33 XOR ecc_l (bit 3)
#if size_opt
 for(i=0;i<7;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 34
 res^=d>>=1;  // Bit 35
 res^=d>>=1;  // Bit 36
 res^=d>>=1;  // Bit 37
 res^=d>>=1;  // Bit 38
 res^=d>>=1;  // Bit 39
 res^=d>>=1;  // Bit 40
#endif
 res^=d>>=9;  // Bit 49
#if size_opt
 for(i=0;i<7;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 50
 res^=d>>=1;  // Bit 51
 res^=d>>=1;  // Bit 52
 res^=d>>=1;  // Bit 53
 res^=d>>=1;  // Bit 54
 res^=d>>=1;  // Bit 55
 res^=d>>=1;  // Bit 56
#endif
 ecc |= ((res&0x1)<<3); // Save ecc bit [3]

 // Calculate the parity bit 16
 d=InputU64->h;
 res=(d>>=9)^(ecc_l>>4);  // Bit 41 XOR ecc_l (bit 4)
#if size_opt
 for(i=0;i<15;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 42
 res^=d>>=1;  // Bit 43
 res^=d>>=1;  // Bit 44
 res^=d>>=1;  // Bit 45
 res^=d>>=1;  // Bit 46
 res^=d>>=1;  // Bit 47
 res^=d>>=1;  // Bit 48
 res^=d>>=1;  // Bit 49
 res^=d>>=1;  // Bit 50
 res^=d>>=1;  // Bit 51
 res^=d>>=1;  // Bit 52
 res^=d>>=1;  // Bit 53
 res^=d>>=1;  // Bit 54
 res^=d>>=1;  // Bit 55
 res^=d>>=1;  // Bit 56
#endif
 ecc |= ((res&0x1)<<4); // Save ecc bit [4]

 // Calculate the parity bit 32
 d=InputU64->h;
 res=d^(ecc_l>>5);  // Bit 32 XOR ecc_l (bit 5)
#if size_opt
 for(i=0;i<24;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 33
 res^=d>>=1;  // Bit 34
 res^=d>>=1;  // Bit 35
 res^=d>>=1;  // Bit 36
 res^=d>>=1;  // Bit 37
 res^=d>>=1;  // Bit 38
 res^=d>>=1;  // Bit 39
 res^=d>>=1;  // Bit 40
 res^=d>>=1;  // Bit 41
 res^=d>>=1;  // Bit 42
 res^=d>>=1;  // Bit 43
 res^=d>>=1;  // Bit 44
 res^=d>>=1;  // Bit 45
 res^=d>>=1;  // Bit 46
 res^=d>>=1;  // Bit 47
 res^=d>>=1;  // Bit 48
 res^=d>>=1;  // Bit 49
 res^=d>>=1;  // Bit 50
 res^=d>>=1;  // Bit 51
 res^=d>>=1;  // Bit 52
 res^=d>>=1;  // Bit 53
 res^=d>>=1;  // Bit 54
 res^=d>>=1;  // Bit 55
 res^=d>>=1;  // Bit 56
#endif
 ecc |= ((res&0x1)<<5); // Save ecc bit [5]

 // Calculate the parity bit 64
 d=InputU64->h;
 res=(d>>=25); // Bit 57  (Seed - no bits in ecc_l)
#if size_opt
 for(i=0;i<6;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 58
 res^=d>>=1;  // Bit 59
 res^=d>>=1;  // Bit 60
 res^=d>>=1;  // Bit 61
 res^=d>>=1;  // Bit 62
 res^=d>>=1;  // Bit 63
#endif
 ecc |= ((res&0x1)<<6); // Save ecc bit [6]


 // Calculate the overall parity bit
 d=InputU64->h;
 res=d^(ecc_l>>7);   // Bit 32 XOR ecc_l (bit 7)
#if size_opt
 for(i=0;i<32;i++)
  res^=d>>=1;
#else
 res^=d>>=1;  // Bit 33
 res^=d>>=1;  // Bit 34
 res^=d>>=1;  // Bit 35
 res^=d>>=1;  // Bit 36
 res^=d>>=1;  // Bit 37
 res^=d>>=1;  // Bit 38
 res^=d>>=1;  // Bit 39
 res^=d>>=1;  // Bit 40
 res^=d>>=1;  // Bit 41
 res^=d>>=1;  // Bit 42
 res^=d>>=1;  // Bit 43
 res^=d>>=1;  // Bit 44
 res^=d>>=1;  // Bit 45
 res^=d>>=1;  // Bit 46
 res^=d>>=1;  // Bit 47
 res^=d>>=1;  // Bit 48
 res^=d>>=1;  // Bit 49
 res^=d>>=1;  // Bit 50
 res^=d>>=1;  // Bit 51
 res^=d>>=1;  // Bit 52
 res^=d>>=1;  // Bit 53
 res^=d>>=1;  // Bit 54
 res^=d>>=1;  // Bit 55
 res^=d>>=1;  // Bit 56
 res^=d>>=1;  // Bit 57
 res^=d>>=1;  // Bit 58
 res^=d>>=1;  // Bit 59
 res^=d>>=1;  // Bit 60
 res^=d>>=1;  // Bit 61
 res^=d>>=1;  // Bit 62
 res^=d>>=1;  // Bit 63
#endif

 // XOR in the ecc bits
 if(fn==HAMencode)
  ecc2=ecc;
 else
  ecc2= ecc_ll;
  
 res^=ecc2>>6;
 res^=ecc2>>5;
 res^=ecc2>>4;
 res^=ecc2>>3;
 res^=ecc2>>2;
 res^=ecc2>>1;
 res^=ecc2;
 ecc |= ((res&0x1)<<7);  // Save ECC bit [7]

 return ecc;
}
