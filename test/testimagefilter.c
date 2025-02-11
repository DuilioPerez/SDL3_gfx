/* 

TestImageFilter.c: test program for MMX filter routines

(C) A. Schiffler, 2012-2014, zlib license
(C) Sylvain Beucler, 2013, zlib license

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef WIN32
#include <windows.h>
#endif
#include "SDL3_imageFilter.h"

#define SRC_SIZE 23

int total_count = 0;
int ok_count = 0;

char* append_number_to_string(char* text, const char* text_end, const char* fmt, unsigned char i);

void setup_src(unsigned char *src1, unsigned char *src2)
{
	int i;

	src1[0]=1;
	src1[2]=1; 
	src1[1]=4;
	src1[3]=3;
	src1[4]=33;
	for (i=5; i<14; i++) src1[i]=i;
	src1[14]=8;
	for (i=15; i<SRC_SIZE; i++) src1[i]=rand();

	src2[0]=1;
	src2[1]=3;
	src2[2]=3; 
	src2[3]=2;
	src2[4]=44;
	for (i=5; i<14; i++) src2[i]=14-i;
	src2[14]=10;
	for (i=15; i<SRC_SIZE; i++) src2[i]=src1[i];
}

void print_result(int mmx, char *label, unsigned char *src1, unsigned char *src2, unsigned char *dst) 
{
	char number_str[256];
	const char* buf_end = number_str + 256;
	char* res;
	char blabel[80];
   int i;
	memset((void *)blabel, ' ', 80);
	blabel[strlen(label)+4]=0;

	SDL_Log("");
	res = number_str;
	res += SDL_snprintf(res, buf_end - res, "%s   pos   ", blabel);
	for (i = 0; i < SRC_SIZE; i++) {
		//SDL_Log("%2d ", i);
		res = append_number_to_string(res, buf_end, "%2d ", i);
	}
		  SDL_Log("%s", number_str);

		  res = number_str;
		  res += SDL_snprintf(res, buf_end - res, "%s   src1  ", blabel);
        for (i = 0; i < SRC_SIZE; i++)
			  res = append_number_to_string(res, buf_end, "%02x ", src1[i]);
		  SDL_Log("%s", number_str);

	if (src2) {
		res = number_str;
		res += SDL_snprintf(res, buf_end - res, "%s   src2  ", blabel);
            for (i = 0; i < SRC_SIZE; i++)
					res = append_number_to_string(res, buf_end, "%02x ", src2[i]);
		SDL_Log("%s", number_str);
	}
	res = number_str;
	res += SDL_snprintf(res, buf_end - res, "%s %s   dest  ",mmx==1?"MMX":" C ",label);
        for (i = 0; i < SRC_SIZE; i++)
			  res = append_number_to_string(res, buf_end, "%02x ", dst[i]);
		  SDL_Log("%s", number_str);
}

void print_compare(unsigned char *dst1, unsigned char *dst2) 
{ 
	total_count++;
	if (memcmp(dst1,dst2,SRC_SIZE)==0) {
		SDL_Log("OK");
		ok_count++;
	} else {
		SDL_Log("ERROR");
	}
}

void print_line() 
{
	SDL_Log ("------------------------------------------------------------------------");
}

/* ----------- main ---------- */

int main(int argc, char *argv[])
{
	unsigned char src1[SRC_SIZE], src2[SRC_SIZE], dstm[SRC_SIZE], dstc[SRC_SIZE];
    int size = 2*1024*1024;
    unsigned char *t1 = (unsigned char *)SDL_malloc(size), *t2 = (unsigned char *)SDL_malloc(size), *d = (unsigned char *)SDL_malloc(size);
    int i;

	/* Initialize to make valgrind happy */
	srand((unsigned int)time(NULL));
	for (i = 0; i < size; i++) {
			/* use more random lower-order bits (int->char) */
			t1[i] = rand(); t2[i] = rand(); d[i] = rand();
	}

	SDL_Init(0);

	/* SDL_imageFilter Test */

	SDL_Log("TestImageFilter\n\n");
	SDL_Log("Testing an array of 23 bytes - first 16 bytes should be processed\n");
	SDL_Log("by MMX or C code, the last 7 bytes only by C code.\n\n");
	
	print_line();
	
	
#define	TEST_C   0
#define	TEST_MMX 1
        {
#define FUNC(f) { #f, SDL_imageFilter ## f }
		struct func {
			char* name;
			int (*f)(unsigned char*, unsigned char*, unsigned char*, unsigned int);
		};
		struct func funcs[] = {
			FUNC(BitAnd),
			FUNC(BitOr),
			FUNC(Add),
			FUNC(AbsDiff),
			FUNC(Mean),
			FUNC(Sub),
			FUNC(Mult),
			FUNC(MultNor),
			FUNC(MultDivby2),
			FUNC(MultDivby4),
			FUNC(Div),
		};
		
		int k;
		for (k = 0; k < sizeof(funcs)/sizeof(struct func); k++) {
			Uint64 start;
			int i;
			
			setup_src(src1, src2);

			SDL_imageFilterMMXon();
			funcs[k].f(src1, src2, dstm, SRC_SIZE);
			print_result(TEST_MMX, funcs[k].name, src1, src2, dstm);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, t2, d, size);
			}
			SDL_Log("MMX %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			SDL_imageFilterMMXoff();
			funcs[k].f(src1, src2, dstc, SRC_SIZE);
			print_result(TEST_C, funcs[k].name, src1, src2, dstc);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, t2, d, size);
			}
			SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			print_compare(dstm,dstc);
			print_line();
		}
        }

	/* setup_src(src1, src2); */
	/* SDL_imageFilterMultNor(src1, src2, dstc, SRC_SIZE); */
        /* start = SDL_GetTicks(); */
        /* for (i = 0; i < 50; i++) { */
        /*     SDL_imageFilterMultNor(t1, t2, d, size); */
        /* } */
        /* printf(" C  %dx%d: %dms\n", i, size, SDL_GetTicks() - start); */
	/* print_result(TEST_C, "MultNor", src1, src2, dstc); */
        /* { */
        /*     unsigned char expected[] = { 0x01, 0x0c, 0x03, 0x06, 0xac, 0x2d, 0x30, 0x31, */
        /*                                  0x30, 0x2d, 0x28, 0x21, 0x18, 0x0d, 0x50, 0xf1, */
        /*                                  0xe0, 0xcd, 0xb8, 0xa1, 0x88, 0x6d, 0x50 }; */
        /*     print_compare(dstc, expected); */
        /* } */
	/* print_line(); */
        /* exit(0); */


        {
		Uint64 start;
		int i;
		char call[1024];
		SDL_snprintf(call, 1024, "BitNegation");
		
		setup_src(src1, src2);
		
		SDL_imageFilterMMXon();
		SDL_imageFilterBitNegation(src1, dstm, SRC_SIZE);
		print_result(TEST_MMX, call, src1, NULL, dstm);
		start = SDL_GetTicks();
		for (i = 0; i < 50; i++) {
			SDL_imageFilterBitNegation(t1, d, size);
		}
		SDL_Log("MMX %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
		
		SDL_imageFilterMMXoff();
		SDL_imageFilterBitNegation(src1, dstc, SRC_SIZE);
		print_result(TEST_C, call, src1, NULL, dstc);
		start = SDL_GetTicks();
		for (i = 0; i < 50; i++) {
			SDL_imageFilterBitNegation(t1, d, size);
		}
		SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
		
		print_compare(dstm,dstc);
		print_line();
        }

	
        {
#undef FUNC
#define FUNC(f, c) { #f, SDL_imageFilter ## f, c }
		struct func {
			char* name;
			int (*f)(unsigned char*, unsigned char*, unsigned int, unsigned char);
			unsigned char arg;
		};
		struct func funcs[] = {
			FUNC(AddByte,                3),
			FUNC(AddByteToHalf,          3),
			FUNC(SubByte,                3),
			FUNC(ShiftRight,             1),
			FUNC(ShiftRightUint,         4),
			FUNC(MultByByte,             3),
			FUNC(ShiftLeftByte,          3),
			FUNC(ShiftLeft,              3),
			FUNC(ShiftLeftUint,          4),
			FUNC(BinarizeUsingThreshold, 9),
		};
		
		int k;
		for (k = 0; k < sizeof(funcs)/sizeof(struct func); k++) {
			Uint64 start;
			int i;
			char call[1024];
			SDL_snprintf(call, 1024, "%s(%u)", funcs[k].name, funcs[k].arg);
			
			setup_src(src1, src2);

			SDL_imageFilterMMXon();
			funcs[k].f(src1, dstm, SRC_SIZE, funcs[k].arg);
			print_result(TEST_MMX, call, src1, NULL, dstm);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg);
			}
			SDL_Log("MMX %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			SDL_imageFilterMMXoff();
			funcs[k].f(src1, dstc, SRC_SIZE, funcs[k].arg);
			print_result(TEST_C, call, src1, NULL, dstc);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg);
			}
			SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			print_compare(dstm,dstc);
			print_line();
		}
        }

	
        {
#undef FUNC
#define FUNC(f, c1, c2) { #f, SDL_imageFilter ## f, c1, c2 }
		struct func {
			char* name;
			int (*f)(unsigned char*, unsigned char*, unsigned int, unsigned char, unsigned char);
			unsigned char arg1, arg2;
		};
		struct func funcs[] = {
			FUNC(ShiftRightAndMultByByte, 1, 3),
			FUNC(ClipToRange, 3, 8),
		};
		
		int k;
		for (k = 0; k < sizeof(funcs)/sizeof(struct func); k++) {
			Uint64 start;
			int i;
			char call[1024];
			SDL_snprintf(call, 1024, "%s(%u,%u)", funcs[k].name, funcs[k].arg1, funcs[k].arg2);
			
			setup_src(src1, src2);

			SDL_imageFilterMMXon();
			funcs[k].f(src1, dstm, SRC_SIZE, funcs[k].arg1, funcs[k].arg2);
			print_result(TEST_MMX, call, src1, NULL, dstm);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg1, funcs[k].arg2);
			}
			SDL_Log("MMX %dx%dk: %lus\n", i, size/1024, SDL_GetTicks() - start);
			
			SDL_imageFilterMMXoff();
			funcs[k].f(src1, dstc, SRC_SIZE, funcs[k].arg1, funcs[k].arg2);
			print_result(TEST_C, call, src1, NULL, dstc);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg1, funcs[k].arg2);
			}
			SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			print_compare(dstm,dstc);
			print_line();
		}
        }

        
        {
		Uint64 start;
		int i;
		char call[1024];
		SDL_snprintf(call, 1024, "NormalizeLinear(0,33,0,255)");
		
		setup_src(src1, src2);
		
		SDL_imageFilterMMXon();
		SDL_imageFilterNormalizeLinear(src1, dstm, SRC_SIZE, 0,33, 0,255);
		print_result(TEST_MMX, call, src1, NULL, dstm);
		start = SDL_GetTicks();
		for (i = 0; i < 50; i++) {
			SDL_imageFilterNormalizeLinear(t1, d, size, 0,33, 0,255);
		}
		SDL_Log("MMX %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
		
		SDL_imageFilterMMXoff();
		SDL_imageFilterNormalizeLinear(src1, dstc, SRC_SIZE, 0,33, 0,255);
		print_result(TEST_C, call, src1, NULL, dstc);
		start = SDL_GetTicks();
		for (i = 0; i < 50; i++) {
			SDL_imageFilterNormalizeLinear(t1, d, size, 0,33, 0,255);
		}
		SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
		
		print_compare(dstm,dstc);
		print_line();
        }


	/* Uint functions */
	/* Disabled, since broken *//* ??? */
        {
#undef FUNC
#define FUNC(f, c) { #f, SDL_imageFilter ## f, c }
		struct func {
			char* name;
			int (*f)(unsigned char*, unsigned char*, unsigned int, unsigned int);
			unsigned int arg;
		};
		struct func funcs[] = {
			FUNC(AddUint,       0x01020304),
			FUNC(SubUint,       0x01020304),
		};
		
		int k;
		for (k = 0; k < sizeof(funcs)/sizeof(struct func); k++) {
			Uint64 start;
			int i;
			char call[1024];
			SDL_snprintf(call, 1024, "%s(%u)", funcs[k].name, funcs[k].arg);
			
			setup_src(src1, src2);

			SDL_imageFilterMMXon();
			funcs[k].f(src1, dstm, SRC_SIZE, funcs[k].arg);
			print_result(TEST_MMX, call, src1, NULL, dstm);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg);
			}
			SDL_Log("MMX %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			SDL_imageFilterMMXoff();
			funcs[k].f(src1, dstc, SRC_SIZE, funcs[k].arg);
			print_result(TEST_C, call, src1, NULL, dstc);
			start = SDL_GetTicks();
			for (i = 0; i < 50; i++) {
				funcs[k].f(t1, d, size, funcs[k].arg);
			}
			SDL_Log(" C  %dx%dk: %lums\n", i, size/1024, SDL_GetTicks() - start);
			
			print_compare(dstm,dstc);
			print_line();
		}
        }


	SDL_imageFilterMMXon();
	if (SDL_imageFilterMMXdetect())
	{
		SDL_Log("MMX was detected\n\n");
	}
	else
	{
		SDL_Log("MMX was NOT detected\n\n");
	}

	SDL_Log("Result: %i of %i passed OK.\n", ok_count, total_count);

	SDL_Quit();
	SDL_free(d);
	SDL_free(t2);
	SDL_free(t1);

	exit(0);
}

char* append_number_to_string(char* text, const char* text_end, const char* fmt, unsigned char i)
{
	if (text >= text_end)
	{
		//Do not keep incrementing return pointer if its already past the end
		return text;
	}

	char* cur = text;
	cur += SDL_snprintf(cur, text_end - cur, fmt, i);
	return cur;
}