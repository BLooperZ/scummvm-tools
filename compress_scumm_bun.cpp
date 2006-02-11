/* compress_scumm_bun - compressor for bundle files
 * Copyright (C) 2004-2005  The ScummVM Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$ * $Id$
 *
 */

#include "compress.h"

inline uint16 READ_LE_UINT16(const void *ptr) {
	const byte *b = (const byte *)ptr;
	return (b[1] << 8) + b[0];
}

inline void WRITE_LE_UINT16(void *ptr, uint16 value) {
	byte *b = (byte *)ptr;
	b[0] = (byte)(value >> 0);
	b[1] = (byte)(value >> 8);
}

inline uint16 READ_BE_UINT16(const void *ptr) {
	const byte *b = (const byte *)ptr;
	return (b[0] << 8) + b[1];
}

inline uint32 READ_BE_UINT32(const void *ptr) {
	const byte *b = (const byte*)ptr;
	return (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + (b[3]);
}

inline void WRITE_BE_UINT16(void *ptr, uint16 value) {
	byte *b = (byte *)ptr;
	b[0] = (byte)(value >> 8);
	b[1] = (byte)(value >> 0);
}


static byte _destImcTable[89];
static uint32 _destImcTable2[89 * 64];

static const int16 imcTable[] = {
	0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010, 0x0011,
	0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, 0x0022, 0x0025, 0x0029, 0x002D,
	0x0032, 0x0037, 0x003C, 0x0042, 0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076,
	0x0082, 0x008F, 0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133,
	0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292, 0x02D4, 0x031C,
	0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583, 0x0610, 0x06AB, 0x0756, 0x0812,
	0x08E0, 0x09C3, 0x0ABD, 0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE,
	0x1706, 0x1954, 0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B,
	0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462, 0x7FFF
};

static const byte imxOtherTable[6][128] = {
	{
		0xFF, 0x04, 0xFF, 0x04
	},
	
	{
		0xFF, 0xFF, 0x02, 0x08, 0xFF, 0xFF, 0x02, 0x08
	},
	
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x02, 0x04, 0x06,
		0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x02, 0x04, 0x06
	},
	
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x04, 0x06, 0x08, 0x0C, 0x10, 0x20,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x04, 0x06, 0x08, 0x0C, 0x10, 0x20
	},
	
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
		0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x20,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
		0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x20
	},
	
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
		0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
		0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
	}
};

static const byte imxShortTable[] = {
	0, 0, 1, 3, 7, 15, 31, 63
};

void initializeImcTables() {
	int32 destTablePos = 0;
	int32 pos = 0;
	
	do {
		byte put = 1;
		int32 tableValue = ((imcTable[pos] * 4) / 7) / 2;
		while (tableValue != 0) {
			tableValue /= 2;
			put++;
		}
		if (put < 3) {
			put = 3;
		}
		if (put > 8) {
			put = 8;
		}
		put--;
		assert(pos < 89);
		_destImcTable[pos] = put;
	} while (++pos <= 88);
	_destImcTable[89] = 0;

	for (int n = 0; n < 64; n++) {
		pos = 0;
		destTablePos = n;
		do {
			int32 count = 32;
			int32 put = 0;
			int32 tableValue = imcTable[pos];
	 		do {
				if ((count & n) != 0) {
					put += tableValue;
				}
				count /= 2;
				tableValue /= 2;
			} while (count != 0);
			assert(destTablePos < 89 * 64);
			_destImcTable2[destTablePos] = put;
			destTablePos += 64;
		} while (++pos <= 88);
	}
}
#define NextBit                            \
	do {                                   \
		bit = mask & 1;                    \
		mask >>= 1;                        \
		if (!--bitsleft) {                 \
			mask = READ_LE_UINT16(srcptr); \
			srcptr += 2;                   \
			bitsleft = 16;                 \
		}                                  \
	} while (0)

static int32 compDecode(byte *src, byte *dst) {
	byte *result, *srcptr = src, *dstptr = dst;
	int data, size, bit, bitsleft = 16, mask = READ_LE_UINT16(srcptr);
	srcptr += 2;

	for (;;) {
		NextBit;
		if (bit) {
			*dstptr++ = *srcptr++;
		} else {
			NextBit;
			if (!bit) {
				NextBit;
				size = bit << 1;
				NextBit;
				size = (size | bit) + 3;
				data = *srcptr++ | 0xffffff00;
			} else {
				data = *srcptr++;
				size = *srcptr++;

				data |= 0xfffff000 + ((size & 0xf0) << 4);
				size = (size & 0x0f) + 3;

				if (size == 3)
					if (((*srcptr++) + 1) == 1)
						return (int32)(dstptr - dst);
			}
			result = dstptr + data;
			while (size--)
				*dstptr++ = *result++;
		}
	}
}
#undef NextBit

int32 decompressCodec(int32 codec, byte *comp_input, byte *comp_output, int32 input_size) {
	int32 output_size, channels;
	int32 offset1, offset2, offset3, length, k, c, s, j, r, t, z;
	byte *src, *t_table, *p, *ptr;
	byte t_tmp1, t_tmp2;

	switch (codec) {
	case 0:
		memcpy(comp_output, comp_input, input_size);
		output_size = input_size;
		break;

	case 1:
		output_size = compDecode(comp_input, comp_output);
		break;

	case 2:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];
		break;

	case 3:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];
		break;

	case 4:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memset(t_table, 0, output_size);

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 0;
		if (length > 0) {
			c = -12;
			s = 0;
			j = 0;
			do {
				ptr = src + length + (k >> 1);
				t_tmp2 = src[j];
				if (k & 1) {
					r = c >> 3;
					t_table[r + 2] = ((t_tmp2 & 0x0f) << 4) | (ptr[1] >> 4);
					t_table[r + 1] = (t_tmp2 & 0xf0) | (t_table[r + 1]);
				} else {
					r = s >> 3;
					t_table[r + 0] = ((t_tmp2 & 0x0f) << 4) | (ptr[0] & 0x0f);
					t_table[r + 1] = t_tmp2 >> 4;
				}
				s += 12;
				c += 12;
				k++;
				j++;
			} while (k < length);
		}
		offset1 = ((length - 1) * 3) >> 1;
		t_table[offset1 + 1] = (t_table[offset1 + 1]) | (src[length - 1] & 0xf0);
		memcpy(src, t_table, output_size);
		free(t_table);
		break;

	case 5:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memset(t_table, 0, output_size);

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 1;
		c = 0;
		s = 12;
		t_table[0] = src[length] >> 4;
		t = length + k;
		j = 1;
		if (t > k) {
			do {
				t_tmp1 = *(src + length + (k >> 1));
				t_tmp2 = src[j - 1];
				if (k & 1) {
					r = c >> 3;
					t_table[r + 0] = (t_tmp2 & 0xf0) | t_table[r];
					t_table[r + 1] = ((t_tmp2 & 0x0f) << 4) | (t_tmp1 & 0x0f);
				} else {
					r = s >> 3;
					t_table[r + 0] = t_tmp2 >> 4;
					t_table[r - 1] = ((t_tmp2 & 0x0f) << 4) | (t_tmp1 >> 4);
				}
				s += 12;
				c += 12;
				k++;
				j++;
			} while (k < t);
		}
		memcpy(src, t_table, output_size);
		free(t_table);
		break;

	case 6:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memset(t_table, 0, output_size);

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 0;
		c = 0;
		j = 0;
		s = -12;
		t_table[0] = src[output_size - 1];
		t_table[output_size - 1] = src[length - 1];
		t = length - 1;
		if (t > 0) {
			do {
				t_tmp1 = *(src + length + (k >> 1));
				t_tmp2 = src[j];
				if (k & 1) {
					r = s >> 3;
					t_table[r + 2] = (t_tmp2 & 0xf0) | t_table[r + 2];
					t_table[r + 3] = ((t_tmp2 & 0x0f) << 4) | (t_tmp1 >> 4);
				} else {
					r = c >> 3;
					t_table[r + 2] = t_tmp2 >> 4;
					t_table[r + 1] = ((t_tmp2 & 0x0f) << 4) | (t_tmp1 & 0x0f);
				}
				s += 12;
				c += 12;
				k++;
				j++;
			} while (k < t);
		}
		memcpy(src, t_table, output_size);
		free(t_table);
		break;

	case 10:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memcpy(t_table, p, output_size);

		offset1 = output_size / 3;
		offset2 = offset1 << 1;
		offset3 = offset2;
		src = comp_output;

		while (offset1--) {
			offset2 -= 2;
			offset3--;
			t_table[offset2 + 0] = src[offset1];
			t_table[offset2 + 1] = src[offset3];
		}

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 0;
		if (length > 0) {
			c = -12;
			s = 0;
			do {
				j = length + (k >> 1);
				t_tmp1 = t_table[k];
				if (k & 1) {
					r = c >> 3;
					t_tmp2 = t_table[j + 1];
					src[r + 2] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 >> 4);
					src[r + 1] = (src[r + 1]) | (t_tmp1 & 0xf0);
				} else {
					r = s >> 3;
					t_tmp2 = t_table[j];
					src[r + 0] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 & 0x0f);
					src[r + 1] = t_tmp1 >> 4;
				}
				s += 12;
				c += 12;
				k++;
			} while (k < length);
		}
		offset1 = ((length - 1) * 3) >> 1;
		src[offset1 + 1] = (t_table[length] & 0xf0) | src[offset1 + 1];
		free(t_table);
		break;

	case 11:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memcpy(t_table, p, output_size);

		offset1 = output_size / 3;
		offset2 = offset1 << 1;
		offset3 = offset2;
		src = comp_output;

		while (offset1--) {
			offset2 -= 2;
			offset3--;
			t_table[offset2 + 0] = src[offset1];
			t_table[offset2 + 1] = src[offset3];
		}

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 1;
		c = 0;
		s = 12;
		t_tmp1 = t_table[length] >> 4;
		src[0] = t_tmp1;
		t = length + k;
		if (t > k) {
			do {
				j = length + (k >> 1);
				t_tmp1 = t_table[k - 1];
				t_tmp2 = t_table[j];
				if (k & 1) {
					r = c >> 3;
					src[r + 0] = (src[r]) | (t_tmp1 & 0xf0);
					src[r + 1] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 & 0x0f);
				} else {
					r = s >> 3;
					src[r + 0] = t_tmp1 >> 4;
					src[r - 1] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 >> 4);
				}
				s += 12;
				c += 12;
				k++;
			} while (k < t);
		}
		free(t_table);
		break;

	case 12:
		output_size = compDecode(comp_input, comp_output);
		p = comp_output;
		for (z = 2; z < output_size; z++)
			p[z] += p[z - 1];
		for (z = 1; z < output_size; z++)
			p[z] += p[z - 1];

		t_table = (byte *)malloc(output_size);
		memcpy(t_table, p, output_size);

		offset1 = output_size / 3;
		offset2 = offset1 << 1;
		offset3 = offset2;
		src = comp_output;

		while (offset1--) {
			offset2 -= 2;
			offset3--;
			t_table[offset2 + 0] = src[offset1];
			t_table[offset2 + 1] = src[offset3];
		}

		src = comp_output;
		length = (output_size << 3) / 12;
		k = 0;
		c = 0;
		s = -12;
		src[0] = t_table[output_size - 1];
		src[output_size - 1] = t_table[length - 1];
		t = length - 1;
		if (t > 0) {
			do {
				j = length + (k >> 1);
				t_tmp1 = t_table[k];
				t_tmp2 = t_table[j];
				if (k & 1) {
					r = s >> 3;
					src[r + 2] = (src[r + 2]) | (t_tmp1 & 0xf0);
					src[r + 3] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 >> 4);
				} else {
					r = c >> 3;
					src[r + 2] = t_tmp1 >> 4;
					src[r + 1] = ((t_tmp1 & 0x0f) << 4) | (t_tmp2 & 0x0f);
				}
				s += 12;
				c += 12;
				k++;
			} while (k < t);
		}
		free(t_table);
		break;

	case 13:
	case 15:
		if (codec == 13) {
			channels = 1;
		} else {
			channels = 2;
		}

		{
			const int MAX_CHANNELS = 2;
			int32 left, startPos, origLeft, curTableEntry, destPos, esiReg;
			int16 firstWord;
			byte sByte[MAX_CHANNELS] = {0, 0};
			int32 sDWord1[MAX_CHANNELS] = {0, 0};
			int32 sDWord2[MAX_CHANNELS] = {0, 0};
			int32 tableEntrySum, imcTableEntry, curTablePos, outputWord, adder;
			byte decompTable, otherTablePos, bitMask;
			byte *readPos, *dst;
			uint16 readWord;
			
			assert(0 <= channels && channels <= MAX_CHANNELS);

			src = comp_input;
			dst = comp_output;
			if (channels == 2) {
				output_size = left = 0x2000;
			} else {
				left = 0x1000;
				output_size = 0x2000;
			}
			firstWord = READ_BE_UINT16(src);
			src += 2;
			if (firstWord != 0) {
				memcpy(dst, src, firstWord);
				dst += firstWord;
				src += firstWord;
				startPos = 0;
				if (channels == 2) {
					left = 0x2000 - firstWord;
					output_size = left;
				} else {
					left = 0x1000 - (firstWord >> 1);
					output_size = left << 1;
				}
				output_size += firstWord;
			} else {
				startPos = 1;
				for (int i = 0; i < channels; i++) {
					sByte[i] = *(src++);
					sDWord1[i] = READ_BE_UINT32(src);
					src += 4;
					sDWord2[i] = READ_BE_UINT32(src);
					src += 4;
				}
			}

			origLeft = left >> (channels - 1);
			tableEntrySum = 0;
			for (int l = 0; l < channels; l++) {
				if (startPos != 0) {
					curTablePos = sByte[l];
					imcTableEntry = sDWord1[l];
					outputWord = sDWord2[l];
				} else {
					curTablePos = 0;
					imcTableEntry = 7;
					outputWord = 0;
				}

				left = origLeft;
				destPos = l << 1;

				if (channels == 2) {
					if (l == 0)
						left++;
					left >>= 1;
				}

				while (left--) {
					curTableEntry = _destImcTable[curTablePos];
					decompTable = (byte)(curTableEntry - 2);
					bitMask = 2 << decompTable;
					readPos = src + (tableEntrySum >> 3);
					readWord = (uint16)(READ_BE_UINT16(readPos) << (tableEntrySum & 7));
					otherTablePos = (byte)(readWord >> (16 - curTableEntry));
					tableEntrySum += curTableEntry;
					esiReg = ((imxShortTable[curTableEntry] & otherTablePos)
						<< (7 - curTableEntry)) + (curTablePos * 64);
					imcTableEntry >>= (curTableEntry - 1);
					adder = imcTableEntry + _destImcTable2[esiReg];
					if ((otherTablePos & bitMask) != 0) {
						adder = -adder;
					}
					outputWord += adder;

					// Clip outputWord to 16 bit signed, and write it into the destination stream
					if (outputWord > 0x7fff)
						outputWord = 0x7fff;
					if (outputWord < -0x8000)
						outputWord = -0x8000;
					WRITE_BE_UINT16(dst + destPos, outputWord);

					// Adjust the curTablePos / imcTableEntry
					assert(decompTable < 6);
					curTablePos += (signed char)imxOtherTable[decompTable][otherTablePos];
					if (curTablePos > 88)
						curTablePos = 88;
					if (curTablePos < 0)
						curTablePos = 0;
					imcTableEntry = imcTable[curTablePos];

					destPos += channels << 1;
				}
			}
		}
		break;

	default:
		printf("decompressCodec() Unknown codec %d!", (int)codec);
		output_size = 0;
		break;
	}

	return output_size;
}

void showhelp(char *exename) {
	printf("\nUsage: %s <inputfile> <inputdir> <outputdir> [--ogg] [encoder params]\n", exename);
	printf("\nMP3 mode params:\n");
	printf(" -b <rate>    <rate> is the target bitrate(ABR)/minimal bitrate(VBR) (default:%d)\n", minBitrDef);
	printf(" -B <rate>    <rate> is the maximum VBR/ABR bitrate (default:%d)\n", maxBitrDef);
	printf(" --vbr        LAME uses the VBR mode (default)\n");
	printf(" --abr        LAME uses the ABR mode\n");
	printf(" -V <value>   specifies the value (0 - 9) of VBR quality (0=best) (default:%d)\n", vbrqualDef);
	printf(" -q <value>   specifies the MPEG algorithm quality (0-9; 0=best) (default:%d)\n", algqualDef);
	printf(" --silent     the output of LAME is hidden (default:disabled)\n");

	printf("\nVorbis mode params:\n");
	printf(" -b <rate>    <rate> is the nominal bitrate (default:unset)\n");
	printf(" -m <rate>    <rate> is the minimum bitrate (default:unset)\n");
	printf(" -M <rate>    <rate> is the maximum bitrate (default:unset)\n");
	printf(" -q <value>   specifies the value (0 - 10) of VBR quality (10=best) (default:%d)\n", oggqualDef);
	printf(" --silent     the output of oggenc is hidden (default:disabled)\n");
	exit(2);
}

struct BundleAudioTable {
	char filename[24];
	int size;
	int offset;
};

static FILE *_waveTmpFile;
static int32 _waveDataSize;
static BundleAudioTable *bundleTable;
static BundleAudioTable cbundleTable[10000]; // difficult to calculate
static int32 cbundleCurIndex = 0;

void encodeWaveWithOgg(char *filename) {
	char fbuf[2048];
	char fbuf2[2048];
	sprintf(fbuf, "\"%s\".wav", filename);
	sprintf(fbuf2, "\"%s\".ogg", filename);
	encodeAudio(fbuf, false, -1, fbuf2, kVorbisMode);
}

void encodeWaveWithLame(char *filename) {
	char fbuf[2048];
	char fbuf2[2048];

	sprintf(fbuf, "\"%s\".wav", filename);
	sprintf(fbuf2, "\"%s\".mp3", filename);
	encodeAudio(fbuf, false, -1, fbuf2, kMP3Mode);
}

void writeWaveHeader(int s_size, int rate, int chan) {
	int bits = 16;
	byte wav[44];
	memset(wav, 0, 44);
	wav[0] = 'R';
	wav[1] = 'I';
	wav[2] = 'F';
	wav[3] = 'F';
	wav[4] = (s_size + 36) & 0xff;
	wav[5] = ((s_size +	36)	>> 8) &	0xff;
	wav[6] = ((s_size +	36)	>> 16) & 0xff;
	wav[7] = ((s_size +	36)	>> 24) & 0xff;
	wav[8] = 'W';
	wav[9] = 'A';
	wav[10]	= 'V';
	wav[11]	= 'E';
	wav[12]	= 'f';
	wav[13]	= 'm';
	wav[14]	= 't';
	wav[15]	= 0x20;
	wav[16]	= 16;
	wav[20]	= 1;
	wav[22]	= chan;
	wav[24]	= rate & 0xff;
	wav[25]	= (rate	>> 8) &	0xff;
	wav[26]	= (rate	>> 16) & 0xff;
	wav[27]	= (rate	>> 24) & 0xff;
	wav[28]	= (rate	* chan * (bits / 8)) & 0xff;
	wav[29]	= ((rate * chan	* (bits	/ 8))>>	8) & 0xff;
	wav[30]	= ((rate * chan	* (bits	/ 8)) >> 16) & 0xff;
	wav[31]	= ((rate * chan	* (bits	/ 8)) >> 24) & 0xff;
	wav[32]	= (chan	* (bits	/ 8)) &	0xff;
	wav[33]	= ((chan * (bits / 8)) >> 8) & 0xff;
	wav[34]	= bits;
	wav[36]	= 'd';
	wav[37]	= 'a';
	wav[38]	= 't';
	wav[39]	= 'a';
	wav[40]	= s_size & 0xff;
	wav[41]	= (s_size >> 8)	& 0xff;
	wav[42]	= (s_size >> 16) & 0xff;
	wav[43]	= (s_size >> 24) & 0xff;

	fseek(_waveTmpFile, 0, SEEK_SET);
	if (fwrite(wav, 1, 44, _waveTmpFile) != 44) {
		printf("error write temp wave file");
		exit(1);
	}
	fclose(_waveTmpFile);
	_waveTmpFile = NULL;
}

void writeToTempWave(char *fileName, byte *output_data, unsigned int size) {
	if (!_waveTmpFile) {
		_waveTmpFile = fopen(fileName, "wb");
		if (!_waveTmpFile) {
			printf("error write temp wave file");
			exit(1);
		}
		byte wav[44];
		memset(wav, 0, 44);
		if (fwrite(output_data, 1, 44, _waveTmpFile) != 44) {
			printf("error write temp wave file");
			exit(1);
		}
		_waveDataSize = 0;
	}
	for (unsigned int j = 0; j < size - 1; j += 2) {
		byte tmp = output_data[j + 0];
		output_data[j + 0] = output_data[j + 1];
		output_data[j + 1] = tmp;
	}
	if (fwrite(output_data, 1, size, _waveTmpFile) != size) {
		printf("error write temp wave file");
		exit(1);
	}
	_waveDataSize += size;
}

static bool _oggMode = false; // mp3 default

typedef struct { int offset, size, codec; } CompTable;

byte *decompressBundleSound(int index, FILE *input, int32 &finalSize) {
	byte compOutput[0x2000];
	int i;

	fseek(input, bundleTable[index].offset, SEEK_SET);

	uint32 tag = readUint32BE(input);
	assert(tag == 'COMP');
	int numCompItems = readUint32BE(input);
	fseek(input, 8, SEEK_CUR);

	CompTable *compTable = (CompTable *)malloc(sizeof(CompTable) * numCompItems);
	int32 maxSize = 0;
	for (i = 0; i < numCompItems; i++) {
		compTable[i].offset = readUint32BE(input);
		compTable[i].size = readUint32BE(input);
		compTable[i].codec = readUint32BE(input);
		fseek(input, 4, SEEK_CUR);
		if (compTable[i].size > maxSize)
			maxSize = compTable[i].size;
	}
	// CMI hack: one more byte at the end of input buffer
	byte *compInput = (byte *)malloc(maxSize + 1);
	byte *compFinal = (byte *)malloc(numCompItems * 0x2000);

	finalSize = 0;

	for (i = 0; i < numCompItems; i++) {
		compInput[compTable[i].size] = 0;
		fseek(input, bundleTable[index].offset + compTable[i].offset, SEEK_SET);
		fread(compInput, 1, compTable[i].size, input);
		int outputSize = decompressCodec(compTable[i].codec, compInput, compOutput, compTable[i].size);
		assert(outputSize <= 0x2000);
		memcpy(compFinal + finalSize, compOutput, outputSize);
		finalSize += outputSize;
	}

	free(compInput);
	free(compTable);

	return compFinal;
}

byte *convertTo16bit(byte *ptr, int inputSize, int &outputSize, int bits, int freq, int channels) {
	outputSize = inputSize;
	if (bits == 8)
		outputSize *= 2;
	if (bits == 12)
		outputSize = (outputSize / 3) * 4;

	byte *outputBuf = (byte *)malloc(outputSize);
	if (bits == 8) {
		byte *buf = outputBuf;
		byte *src = ptr;
		for (int i = 0; i < inputSize; i++) {
			uint16 val = (*src++ - 0x80) << 8;
			*buf++ = (byte)(val >> 8);
			*buf++ = (byte)val;
		}
	}
	if (bits == 12) {
		int loop_size = inputSize / 3;
		byte *decoded = outputBuf;
		byte *source = ptr;
		uint32 value;

		while (loop_size--) {
			byte v1 = *source++;
			byte v2 = *source++;
			byte v3 = *source++;
			value = ((((v2 & 0x0f) << 8) | v1) << 4) - 0x8000;
			*decoded++ = (byte)((value >> 8) & 0xff);
			*decoded++ = (byte)(value & 0xff);
			value = ((((v2 & 0xf0) << 4) | v3) << 4) - 0x8000;
			*decoded++ = (byte)((value >> 8) & 0xff);
			*decoded++ = (byte)(value & 0xff);
		}
	}
	if (bits == 16) {
		int loop_size = inputSize / 2;
		byte *buf = outputBuf;
		byte *src = ptr;
		while (loop_size--) {
			*buf++ = *src++;
			*buf++ = *src++;
		}
	}

	return outputBuf;
}

void countMapElements(byte *ptr, int &numRegions, int &numJumps, int &numSyncs) {
	uint32 tag;
	int32 size = 0;

	do {
		tag = READ_BE_UINT32(ptr); ptr += 4;
		switch(tag) {
		case 'TEXT':
		case 'STOP':
		case 'FRMT':
		case 'DATA':
			size = READ_BE_UINT32(ptr); ptr += size + 4;
			break;
		case 'REGN':
			numRegions++;
			size = READ_BE_UINT32(ptr); ptr += size + 4;
			break;
		case 'JUMP':
			numJumps++;
			size = READ_BE_UINT32(ptr); ptr += size + 4;
			break;
		case 'SYNC':
			numSyncs++;
			size = READ_BE_UINT32(ptr); ptr += size + 4;
			break;
		default:
			error("countMapElements() Unknown tag of Map");
		}
	} while (tag != 'DATA');
}

struct Region {
	int32 offset;
	int32 length;
};

struct Jump {
	int32 offset;
	int32 dest;
	byte hookId;
	int16 fadeDelay;
};

struct Sync {
	int32 size;
	byte *ptr;
};

static Region *_region;
static int _numRegions;

void writeRegions(byte *ptr, int bits, int freq, int channels, char *dir, char *filename, FILE *output) {
	char tmpPath[200];

	for (int l = 0; l < _numRegions; l++) {
		int outputSize = 0;
		int size = _region[l].length;
		int offset = _region[l].offset;
		byte *outputData = convertTo16bit(ptr + offset, size, outputSize, bits, freq, channels);
		sprintf(tmpPath, "%s/%s_reg%03d.wav", dir, filename, l);
		writeToTempWave(tmpPath, outputData, outputSize);
		writeWaveHeader(_waveDataSize, freq, channels);
		free(outputData);
		sprintf(tmpPath, "%s/%s_reg%03d", dir, filename, l);
		if (_oggMode)
			encodeWaveWithOgg(tmpPath);
		else
			encodeWaveWithLame(tmpPath);
		sprintf(tmpPath, "%s/%s_reg%03d.wav", dir, filename, l);
		unlink(tmpPath);

		int32 startPos = ftell(output);
		if (_oggMode)
			sprintf(cbundleTable[cbundleCurIndex].filename, "%s_reg%03d.ogg", filename, l);
		else
			sprintf(cbundleTable[cbundleCurIndex].filename, "%s_reg%03d.mp3", filename, l);
		cbundleTable[cbundleCurIndex].offset = startPos;

		if (_oggMode)
			sprintf(tmpPath, "%s/%s_reg%03d.ogg", dir, filename, l);
		else
			sprintf(tmpPath, "%s/%s_reg%03d.mp3", dir, filename, l);
		FILE *cmpFile = fopen(tmpPath, "rb");
		fseek(cmpFile, 0, SEEK_END);
		size = ftell(cmpFile);
		fseek(cmpFile, 0, SEEK_SET);
		byte *tmpBuf = (byte *)malloc(size);
		fread(tmpBuf, size, 1, cmpFile);
		fclose(cmpFile);
		unlink(tmpPath);
		fwrite(tmpBuf, size, 1, output);
		free(tmpBuf);
		cbundleTable[cbundleCurIndex].size = ftell(output) - startPos;
		cbundleCurIndex++;
	}
	free(_region);
}

void recalcRegions(int32 &value, int bits, int freq, int channels) {
	int size = value;
	if (bits == 8)
		size *= 2;
	if (bits == 12)
		size = (size / 3) * 4;
	value = size;
}

void writeToRMAPFile(byte *ptr, FILE *output, char *filename, int &offsetData, int &bits, int &freq, int &channels) {
	byte *s_ptr = ptr;
	int32 size = 0;
	int l;

	uint32 tag = READ_BE_UINT32(ptr);
	assert(tag == 'iMUS');
	ptr += 16;

	int curIndexRegion = 0;
	int curIndexJump = 0;
	int curIndexSync = 0;

	int numRegions = 0, numJumps = 0, numSyncs = 0;
	countMapElements(ptr, numRegions, numJumps, numSyncs);
	Region *region = (Region *)malloc(sizeof(Region) * numRegions);
	_region = (Region *)malloc(sizeof(Region) * numRegions);
	_numRegions = numRegions;
	Jump *jump = (Jump *)malloc(sizeof(Jump) * numJumps);
	Sync *sync = (Sync *)malloc(sizeof(Sync) * numSyncs);

	do {
		tag = READ_BE_UINT32(ptr); ptr += 4;
		switch (tag) {
		case 'FRMT':
			ptr += 12;
			bits = READ_BE_UINT32(ptr); ptr += 4;
			freq = READ_BE_UINT32(ptr); ptr += 4;
			channels = READ_BE_UINT32(ptr); ptr += 4;
			break;
		case 'TEXT':
		case 'STOP':
			size = READ_BE_UINT32(ptr); ptr += size + 4;
			break;
		case 'REGN':
			ptr += 4;
			region[curIndexRegion].offset = READ_BE_UINT32(ptr); ptr += 4;
			region[curIndexRegion].length = READ_BE_UINT32(ptr); ptr += 4;
			curIndexRegion++;
			break;
		case 'JUMP':
			ptr += 4;
			jump[curIndexJump].offset = READ_BE_UINT32(ptr); ptr += 4;
			jump[curIndexJump].dest = READ_BE_UINT32(ptr); ptr += 4;
			jump[curIndexJump].hookId = READ_BE_UINT32(ptr); ptr += 4;
			jump[curIndexJump].fadeDelay = READ_BE_UINT32(ptr); ptr += 4;
			curIndexJump++;
			break;
		case 'SYNC':
			size = READ_BE_UINT32(ptr); ptr += 4;
			sync[curIndexSync].size = size;
			sync[curIndexSync].ptr = (byte *)malloc(size);
			memcpy(sync[curIndexSync].ptr, ptr, size);
			curIndexSync++;
			ptr += size;
			break;
		case 'DATA':
			ptr += 4;
			break;
		default:
			error("writeToRMAPFile() Unknown tag of Map for sound '%s'", filename);
		}
	} while (tag != 'DATA');
	offsetData = (int32)(ptr - s_ptr);

	int32 startPos = ftell(output);
	sprintf(cbundleTable[cbundleCurIndex].filename, "%s.map", filename);
	cbundleTable[cbundleCurIndex].offset = startPos;

	writeUint32BE(output, 'RMAP');
	writeUint32BE(output, 2); // version
	writeUint32BE(output, 16); // bits
	writeUint32BE(output, freq);
	writeUint32BE(output, channels);
	writeUint32BE(output, numRegions);
	writeUint32BE(output, numJumps);
	writeUint32BE(output, numSyncs);
	memcpy(_region, region, sizeof(Region) * numRegions);
	for (l = 0; l < numRegions; l++) {
		_region[l].offset -= offsetData;
		region[l].offset -= offsetData;
		recalcRegions(region[l].offset, bits, freq, channels);
		recalcRegions(region[l].length, bits, freq, channels);
		writeUint32BE(output, region[l].offset);
		writeUint32BE(output, region[l].length);
	}
	for (l = 0; l < numJumps; l++) {
		jump[l].offset -= offsetData;
		jump[l].dest -= offsetData;
		recalcRegions(jump[l].offset, bits, freq, channels);
		recalcRegions(jump[l].dest, bits, freq, channels);
		writeUint32BE(output, jump[l].offset);
		writeUint32BE(output, jump[l].dest);
		writeUint32BE(output, jump[l].hookId);
		writeUint32BE(output, jump[l].fadeDelay);
	}
	for (l = 0; l < numSyncs; l++) {
		writeUint32BE(output, sync[l].size);
		fwrite(sync[l].ptr, sync[l].size, 1, output);
		free(sync[l].ptr);
	}
	free(region);
	free(jump);
	free(sync);

	cbundleTable[cbundleCurIndex].size = ftell(output) - startPos;
	cbundleCurIndex++;
}

int main(int argc, char *argv[]) {
	if (argc < 4)
		showhelp(argv[0]);

	char inputDir[200];
	char outputDir[200];
	char inputFilename[200];
	char tmpPath[200];

	uint32 tag;
	int32 numFiles, offset, i;

	strcpy(inputFilename, argv[1]);
	strcpy(inputDir, argv[2]);
	strcpy(outputDir, argv[3]);

	if (argc > 4) {
		i = 4;

		if (strcmp(argv[i], "--ogg") == 0) {
			_oggMode = true;
			i++;
		}

		if (argc > i) {
			// HACK: The functions in compress.c expect the last
			// argument to be a filename. As we don't expect one,
			// we simply add a dummy argument to the list.
			char **args = (char **)malloc((argc + 1) * sizeof(char *));
			char dummyName[] = "dummy";
			int j;

			for (j = 0; j < argc; j++)
				args[j] = argv[j];
			args[j] = dummyName;
		
			int result;

			if (_oggMode)
				result = process_ogg_parms(argc + 1, args, i);
			else
				result = process_mp3_parms(argc + 1, args, i);

			if (!result)
				showhelp(argv[0]);

			free(args);
		}
	}

	char *index = strrchr(inputFilename, '.');
	if (index != NULL) {
		*index = 0;
	}

	sprintf(tmpPath, "%s/%s.bun", inputDir, inputFilename);

	FILE *input = fopen(tmpPath, "rb");
	if (!input) {
		printf("Cannot open file: %s\n", tmpPath);
		exit(-1);
	}

	sprintf(tmpPath, "%s/%s.bun", outputDir, inputFilename);

	FILE *output = fopen(tmpPath, "wb");
	if (!output) {
		printf("Cannot open file: %s\n", tmpPath);
		exit(-1);
	}

	writeUint32BE(output, 'LB23');
	writeUint32BE(output, 0); // will be later
	writeUint32BE(output, 0); // will be later

	initializeImcTables();

	tag = readUint32BE(input);
	assert(tag == 'LB83');
	offset = readUint32BE(input);
	numFiles = readUint32BE(input);

	bundleTable = (BundleAudioTable *)malloc(numFiles * sizeof(BundleAudioTable));
	fseek(input, offset, SEEK_SET);

	for (i = 0; i < numFiles; i++) {
		char filename[13], c;
		int z = 0;
		int z2;
			
		for (z2 = 0; z2 < 8; z2++)
			if ((c = readByte(input)) != 0)
				filename[z++] = c;
		filename[z++] = '.';
		for (z2 = 0; z2 < 4; z2++)
			if ((c = readByte(input)) != 0)
				filename[z++] = c;
		filename[z] = '\0';
		strcpy(bundleTable[i].filename, filename);
		bundleTable[i].offset = readUint32BE(input);
		bundleTable[i].size = readUint32BE(input);
	}

	for (i = 0; i < numFiles; i++) {
		if (strcmp(bundleTable[i].filename, "PRELOAD.") == 0)
			continue;
		int offsetData = 0, bits = 0, freq = 0, channels = 0, size = 0;
		byte *compFinal = decompressBundleSound(i, input, size);
		writeToRMAPFile(compFinal, output, bundleTable[i].filename, offsetData, bits, freq, channels);
		writeRegions(compFinal + offsetData, bits, freq, channels, outputDir, bundleTable[i].filename, output);
		free(compFinal);
	}

	int32 curPos = ftell(output);
	for (i = 0; i < cbundleCurIndex; i++) {
		fwrite(cbundleTable[i].filename, 24, 1, output);
		writeUint32BE(output, cbundleTable[i].offset);
		writeUint32BE(output, cbundleTable[i].size);
	}

	fseek(output, 4, SEEK_SET);
	writeUint32BE(output, curPos);
	writeUint32BE(output, cbundleCurIndex);

	free(bundleTable);

	fclose(input);

	printf("compression done.\n");
		
	return 0;
}
