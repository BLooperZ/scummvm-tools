/* compress_gob - .stk/.itk archive creation tool, based on a conf file.
 * Copyright (C) 2007 The ScummVM project
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
 * $URL$
 * $Id$
 *
 */

#include "util.h"
#include "compress_gob.h"

struct CompressGob::Chunk {
	char name[64];
	uint32 size, realSize, offset;
	uint8 packed;
	Chunk *replChunk;
	Chunk *next;

	Chunk() : next(0) { }
	~Chunk() { delete next; }
};


CompressGob::CompressGob(const std::string &name) : CompressionTool(name, TOOLTYPE_COMPRESSION) {
	_execMode = MODE_NORMAL;
	_chunks = NULL;

	ToolInput input;
	input.format = "*.stk";
	_inputPaths.push_back(input);

	_shorthelp = "Compresses Gobliiins! data files.";
	_helptext = 
		"\nUsage: " + getName() + " [-f] [-o <output> = out.stk] <conf file>\n"
		"<conf file> is a .gob file generated extract_gob_stk\n"
		"<-f> ignores the compression flag in the .gob file and force compression for all files\n\n"
		"The STK/ITK archive will be created in the current directory.\n";


}

CompressGob::~CompressGob() {
	delete _chunks;
}

void CompressGob::parseExtraArguments() {
	if (_arguments[_arguments_parsed] == "-f") {
		_execMode |= MODE_FORCE;
		++_arguments_parsed;
	}
}

void CompressGob::execute() {
	File stk;
	File gobConf;
	uint16 chunkCount;

	Filename inpath(_inputPaths[0].path);
	Filename &outpath = _outputPath;

	// We output with .stk extension, if there is no specific out file
	if (outpath.empty()) {
		outpath = inpath;
		outpath.setExtension(".stk");
	}

	// Open input (config) file
	gobConf.open(inpath, "r");

	stk.open(outpath, "wb");

	// Read the input into memory
	_chunks = readChunkConf(gobConf, outpath, chunkCount);
	gobConf.close();

	// Output in compressed format
	writeEmptyHeader (stk, chunkCount);
	writeBody(&inpath, stk, _chunks);
	rewriteHeader(stk, chunkCount, _chunks);
}

/*! \brief Config file parser
 * \param gobConf Config file to parse
 * \param stk STK/ITK archive file to be created
 * \param chunkCount Number of chunks to be written in the archive file
 * \return A list of chunks
 *
 * This function reads the '.gob' config file (generated by extract_gob_stk).
 * It creates the output archive file and a list of chunks containing the file
 * and compression information.
 * In order to have a slightly better compression ration in some cases (Playtoons), it
 * also detects duplicate files.
 */
CompressGob::Chunk *CompressGob::readChunkConf(File &gobConf, Filename &stkName, uint16 &chunkCount) {
	Chunk *chunks = new Chunk;
	Chunk *curChunk = chunks;
	Chunk *parseChunk;
	File src1;

	chunkCount = 1;

// First read: Output filename
	stkName.setFullName(gobConf.readString());

// Second read: signature
	std::string signature = gobConf.readString();
	if (signature == confSTK21)
		error("STK21 not yet handled");
	else if (signature != confSTK10)
		error("Unknown format signature");

// All the other reads concern file + compression flag
	std::string fname = gobConf.readString();
	while (!gobConf.eos()) {
		strcpy(curChunk->name, fname.c_str());
		fname = gobConf.readString();
		if ((fname == "1") || (_execMode & MODE_FORCE))
			curChunk->packed = true;
		else
			curChunk->packed = false;

		src1.open(curChunk->name, "rb");
		src1.seek(0, SEEK_END);
// if file is too small, force 'Store' method
		if ((curChunk->realSize = src1.pos()) < 8) 
			curChunk->packed = 0;

		parseChunk = chunks;
		while (parseChunk != curChunk) {
			if ((parseChunk->realSize == curChunk->realSize) & (parseChunk->packed != 2)) {
				if (strcmp(parseChunk->name, curChunk->name) == 0)
					error("Duplicate filename found in conf file: %s", parseChunk->name);
				if (filcmp(src1, parseChunk)) {
// If files are identical, use the same compressed chunk instead of re-compressing the same thing
					curChunk->packed = 2;
					curChunk->replChunk = parseChunk;
					print("Identical files : %s %s (%d bytes)\n", curChunk->name, parseChunk->name, curChunk->realSize);
					break;
				}
			}
			parseChunk = parseChunk->next;
		}
		src1.close();
		
		std::string tmp = gobConf.readString();
		if (!gobConf.eos()) {
			curChunk->next = new Chunk;
			curChunk = curChunk->next;
			chunkCount++;
		}
	}
	return chunks;
}

/*! \brief Write an empty header to the STK archive
 * \param stk STK/ITK archive file
 * \param chunkCount Number of chunks to be written in the archive file
 *
 * This function writes an empty header in the STK archive. This is required as
 * the header length is variable and depends on the number of chunks to be written
 * in the archive file. 
 *
 * This header will be overwritten just before the end of the program execution
 */
void CompressGob::writeEmptyHeader(File &stk, uint16 chunkCount) {
	for (uint32 count = 0; count < 2 + (uint32) (chunkCount * 22); count++)
		stk.writeByte(0);

	return;
}

/*! \brief Write the body of the STK archive
 * \param stk STK/ITK archive file
 * \param chunks Chunk list
 *
 * This function writes the body of the STK archive by storing or compressing
 * (or skipping duplicate files) the files. It also updates the chunk information
 * with the size of the chunk in the archive, the compression method (if modified),
 * ...
 */
void CompressGob::writeBody(Filename *inpath, File &stk, Chunk *chunks) {
	Chunk *curChunk = chunks;
	File src;
	uint32 tmpSize;
	
	while (curChunk) {
		inpath->setFullName(curChunk->name);
		src.open(*inpath, "rb");

		if (curChunk->packed == 2)
			print("Identical file %12s\t(compressed size %d bytes)\n", curChunk->name, curChunk->replChunk->size);

		curChunk->offset = stk.pos();
		if (curChunk->packed == 1) {
			print("Compressing %12s\t", curChunk->name);
			curChunk->size = writeBodyPackFile(stk, src);
			print("%d -> %d bytes", curChunk->realSize, curChunk->size);
			if (curChunk->size >= curChunk->realSize) {
// If compressed size >= realsize, compression is useless
// => Store instead
				curChunk->packed = 0;
				stk.seek(curChunk->offset, SEEK_SET);
				src.rewind();
				print("!!!");
			}
			print("\n");
		} 

		if (curChunk->packed == 0) {
			tmpSize = 0;
			print("Storing %12s\t", curChunk->name);
			curChunk->size = writeBodyStoreFile(stk, src);
			print("%d bytes\n", curChunk->size);
		}
		curChunk = curChunk->next;
	}
}

/*! \brief Rewrites the header of the archive file
 * \param stk STK/ITK archive file
 * \param chunkCount Number of chunks
 * \param chunks List of chunks
 *
 * This function rewrites the header of the archive, replacing dummy values 
 * by the one computed during execution.
 * The structure of the header is the following :
 * + 2 bytes : numbers of files archived in the .stk/.itk
 * Then, for each files :
 * + 13 bytes : the filename, terminated by '\0'. In original, there's
 *   garbage after if the filename has not the maximum length
 * + 4  bytes : size of the chunk
 * + 4  bytes : start position of the chunk in the file
 * + 1  byte  : If 0 : not compressed, if 1 : compressed
 * 
 * The duplicate files are defined using the same information
 * as the one of the replacement file.
*/
void CompressGob::rewriteHeader(File &stk, uint16 chunkCount, Chunk *chunks) {
	uint16 i;
	char buffer[1024];
	Chunk *curChunk = chunks;

	stk.rewind();

	buffer[0] = chunkCount & 0xFF;
	buffer[1] = chunkCount >> 8;
	stk.write(buffer, 1, 2);
// TODO : Implement STK21
	while (curChunk) {
		for (i = 0; i < 13; i++)
			if (i < strlen(curChunk->name))
				buffer[i] = curChunk->name[i];
			else
				buffer[i] = '\0';
		stk.write(buffer, 1, 13);

		if (curChunk->packed == 2)
		{
			buffer[0] = curChunk->replChunk->size;
			buffer[1] = curChunk->replChunk->size >> 8;
			buffer[2] = curChunk->replChunk->size >> 16;
			buffer[3] = curChunk->replChunk->size >> 24;
			buffer[4] = curChunk->replChunk->offset;
			buffer[5] = curChunk->replChunk->offset >> 8;
			buffer[6] = curChunk->replChunk->offset >> 16;
			buffer[7] = curChunk->replChunk->offset >> 24;
			buffer[8] = curChunk->replChunk->packed;
		} else {
			buffer[0] = curChunk->size;
			buffer[1] = curChunk->size >> 8;
			buffer[2] = curChunk->size >> 16;
			buffer[3] = curChunk->size >> 24;
			buffer[4] = curChunk->offset;
			buffer[5] = curChunk->offset >> 8;
			buffer[6] = curChunk->offset >> 16;
			buffer[7] = curChunk->offset >> 24;
			buffer[8] = curChunk->packed;
		}
		stk.write(buffer, 1, 9);
		curChunk = curChunk->next;
	}
}

/*! \brief Stores a file in the archive file
 * \param stk STK/ITK archive file
 * \param src File to be stored
 * \return Size of the file stored
 *
 * This function stores a file in the STK archive
 */
uint32 CompressGob::writeBodyStoreFile(File &stk, File &src) {
	int count;
	char buffer[4096];
	uint32 tmpSize = 0;

	do {
		count = src.readN(buffer, 1, 4096);
		stk.write(buffer, 1, count);
		tmpSize += count;
	} while (count == 4096);
	return tmpSize;
}

/*! \brief Compress a file in the archive file
 * \param stk STK/ITK archive file
 * \param src File to be stored
 * \return Size of the resulting compressed chunk
 *
 * This function compress a file in the STK archive
 */
uint32 CompressGob::writeBodyPackFile(File &stk, File &src) {
	byte dico[4114];
	byte writeBuffer[17];
	uint32 counter;
	uint16 dicoIndex;
	uint32 unpackedIndex, size;
	uint8 cmd;
	uint8 buffIndex, cpt;
	uint16 resultcheckpos;
	byte resultchecklength;

	size = src.size();

	byte *unpacked = new byte [size + 1];

	memset(dico, 0x20, 4114);
	memset(unpacked, 0, size + 1);

	src.read(unpacked, 1, size);

	writeBuffer[0] = size & 0xFF;
	writeBuffer[1] = size >> 8;
	writeBuffer[2] = size >> 16;
	writeBuffer[3] = size >> 24;
	stk.write(writeBuffer, 1, 4);

// Size is already checked : small files (less than 8 characters) 
// are not compressed, so copying the first three bytes is safe.
	dicoIndex = 4078;
	dico[dicoIndex] = unpacked[0];
	dico[dicoIndex+1] = unpacked[1];
	dico[dicoIndex+2] = unpacked[2];
	dicoIndex += 3;

// writeBuffer[0] is reserved for the command byte
	writeBuffer[1] = unpacked[0];
	writeBuffer[2] = unpacked[1];
	writeBuffer[3] = unpacked[2];
// Force the 3 first operation bits to 'copy character'
	cmd = (1 << 3) - 1;

	counter = size - 3;
	unpackedIndex = 3;
	cpt = 3;
	buffIndex = 4;

	size=4;
	resultcheckpos = 0;
	resultchecklength = 0;

	while (counter>0) {
		if (!checkDico(unpacked, unpackedIndex, counter, dico, dicoIndex, resultcheckpos, resultchecklength)) {
			dico[dicoIndex] = unpacked[unpackedIndex];
			writeBuffer[buffIndex] = unpacked[unpackedIndex];
// set the operation bit : copy character
			cmd |= (1 << cpt);
			unpackedIndex++;
			dicoIndex = (dicoIndex + 1) % 4096;
			buffIndex++;
			counter--;
		} else {
// Copy the string in the dictionary
			for (int i = 0; i < resultchecklength; i++)
				dico[((dicoIndex + i) % 4096)] = dico[((resultcheckpos + i) % 4096)];

// Write the copy string command
			writeBuffer[buffIndex] = resultcheckpos & 0xFF;
			writeBuffer[buffIndex + 1] = ((resultcheckpos & 0x0F00) >> 4) + (resultchecklength - 3);

// Do not set the operation bit : copy string from dictionary
//			cmd |= (0 << cpt);

			unpackedIndex += resultchecklength;
			dicoIndex = (dicoIndex + resultchecklength) % 4096;
			resultcheckpos = (resultcheckpos + resultchecklength) % 4096;

			buffIndex += 2;
			counter -= resultchecklength;
		}

// The command byte is complete when the file is entirely compressed, or 
// when the 8 operation bits are set.
		if ((cpt == 7) | (counter == 0)) {
			writeBuffer[0] = cmd;
			stk.write(writeBuffer, 1, buffIndex);
			size += buffIndex;
			buffIndex = 1;
			cmd = 0;
			cpt = 0;
		} else
			cpt++;
	}

	delete[] unpacked;
	return size;
}

/*! \brief Compare a file to a file defined in a chunk
 * \param src1 File to be compared
 * \param compChunk Chunk containing information on second file to be compared
 * \return whether they are identical or not.
 *
 * This function compares a file to another defined in a chunk. The file sizes 
 * are already tested outside the function.
 */
bool CompressGob::filcmp(File &src1, Chunk *compChunk) {
	uint16 readCount;
	bool checkFl = true;
	char buf1[4096]; 
	char buf2[4096];
	File src2;

	src1.rewind();
	src2.open(compChunk->name, "rb");
	
	do {
		readCount = src1.readN(buf1, 1, 4096);
		src2.read(buf2, 1, 4096);
		for (int i = 0; checkFl & (i < readCount); i++)
			if (buf1[i] != buf2[i])
				checkFl = false;
	} while (checkFl & (readCount == 4096));

	return checkFl;
}

/*! \brief Compare a file to a file defined in a chunk
 * \param unpacked Buffer being compressed
 * \param unpackedIndex Current 'read' position in this buffer
 * \param counter Number of bytes still to be compressed in the file
 * \param dico Dictionary
 * \param currIndex Current 'write' position in the dictionary (used to avoid dictionary collision)
 * \param pos Position of the better match found, if any
 * \param length Length of the better match found, if any
 * \return whether a match has been found or not or not.
 *
 * This function search in the dictionary for matches with the characters still to be compressed. 
 * 'A match' is when at least three characters of the buffer (comparing from the current 'read' position)
 * are found in the dictionary. The match lengths are limited to 18 characters, as the 
 * length (minus 3) is stored on 4 bits.
 */
bool CompressGob::checkDico(byte *unpacked, uint32 unpackedIndex, int32 counter, byte *dico, uint16 currIndex, uint16 &pos, uint8 &length) {
	uint16 tmpPos, bestPos;
	uint8 tmpLength, bestLength, i;

	bestPos = 0;
	bestLength = 2;

	if (counter < 3)
		return false;

	for (tmpPos = 0; tmpPos < 0x1000; tmpPos++) {
		tmpLength = 0;
		for (i = 0; ((i < 18) & (i < counter)); i++)
			if ((unpacked[unpackedIndex + i] == dico[(tmpPos + i) % 4096]) & 
				// avoid dictionary collision
				(((tmpPos + i) % 4096 != currIndex) | (i == 0)))
				tmpLength++;
			else
				break;
		if (tmpLength > bestLength)
		{
			bestPos = tmpPos;
			if ((bestLength = tmpLength) == 18)
				break;
		}
	}

	pos = bestPos;
	length = bestLength;

	if (bestLength > 2)
		return true;
	else {
		length = 0;
		return false;
	}
}

#ifdef STANDALONE_MAIN
int main(int argc, char *argv[]) {
	CompressGob gob(argv[0]);
	return gob.run(argc, argv);
}
#endif
