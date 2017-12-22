#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

const uint32_t HEADER_SZ = 14;

inline uint32_t mk_uint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
	return ((b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
}

streamoff get_file_len(ifstream& file) {
	file.seekg(0, file.end);
	streamoff fileLen = file.tellg();
	file.seekg(0, file.beg);
	return fileLen;
}

inline uint32_t read_uint32(istreambuf_iterator<char>& ptr) {
	uint32_t res = 0;
	for (int i = 0; i < 4; ++i) {
		uint8_t byte = *ptr;
		res |= byte << (i * 8);
		ptr++;
	}
	return res;
}

vector<uint8_t> read_vector(istreambuf_iterator<char>& ptr, uint32_t size) {
	vector<uint8_t> res(size);
	for (uint32_t i = 0; i < size; ++i) {
		uint8_t val = *ptr;
		res[i] = val;
		ptr++;
	}
	return res;
}

bool ck_enc_type(uint32_t encType) {
	return (encType == 0);
}

bool save(const string& path, vector<uint8_t>& data) {
	ofstream file(path, ios::binary);

	if (!file.good()) { return false; }

	for (auto x : data) { file << x; }

	return true;
}

bool unpack(const string& inPath, vector<uint8_t>& unpacked) {
	ifstream file(inPath, ios::binary);

	if (!file.good()) {
		return false;
	}

	streamoff fileLen = get_file_len(file);
	if (fileLen < HEADER_SZ) { return false; }

	auto ptr = istreambuf_iterator<char>(file);

	uint32_t fourCC = read_uint32(ptr);
	if (fourCC != mk_uint32('S', 'P', 'C', 'K')) {
		return false;
	}
	uint32_t origSz = read_uint32(ptr);
	if (origSz == 0) { return false; }

	uint32_t packedSz = read_uint32(ptr);
	if (packedSz == 0) { return false; }

	uint8_t encType = *ptr;
	if (!ck_enc_type(encType)) {
		return false;
	}
	ptr++;

	uint8_t decodeTblSz = *ptr;
	decodeTblSz++;
	ptr++;

	if (fileLen < HEADER_SZ + decodeTblSz + packedSz) { return false; }

	vector<uint8_t> decode = read_vector(ptr, decodeTblSz);
	vector<uint8_t> packed = read_vector(ptr, packedSz);
	packed.push_back(0); // extra byte
	unpacked.reserve(origSz);

	uint32_t byteIdx = 0;
	uint32_t bitIdx = 0;

	for (uint32_t bitNo = 0, i = 0; i < origSz; ++i) {
		byteIdx = (bitNo >> 3);
		uint8_t bitIdx = bitNo & 7;
		uint8_t bitlen = packed[byteIdx] >> bitIdx;
		bitlen |= packed[byteIdx + 1] << (8 - bitIdx);
		bitlen &= 7;
		bitlen += 1; // 0..7 ==> 1..8

		bitNo += 3;

		uint8_t bitsTaken = bitlen;
		if (bitlen > 1) {
			--bitsTaken;
		}
		byteIdx = (bitNo >> 3);
		bitIdx = bitNo & 7;
		uint8_t code = packed[byteIdx] >> bitIdx;
		code |= packed[byteIdx + 1] << (8 - bitIdx);
		code &= (uint8_t)(((uint32_t)1 << bitsTaken) - 1);
		if (bitlen > 1) {
			code += (1 << (bitlen - 1));
		}

		bitNo += bitsTaken;

		uint8_t decoded = decode[code];
		unpacked.push_back(decoded);
	}

	return true;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Please provide a file path\n";
		return -1;
	}

	string outFile;
	if (argc >= 3) {
		outFile = argv[2];
	} else {
		outFile = string(argv[1]) + ".spck";
	}

	cout << "Unpacking " << argv[1] <<"\n";
	vector<uint8_t> unpacked;

	if (unpack(argv[1], unpacked)) {
		if (save(outFile, unpacked)) {
			cout << "Done\n";
		} else {
			cout << "Can't save the unpacked file\n";
		}
	} else {
		cout << "Couldn't unpack the file\n";
	}

	return 0;
}
