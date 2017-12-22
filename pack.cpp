#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

const uint32_t HEADER_SZ = 14;

uint8_t get_bit_len(uint8_t x) {
	for (uint8_t i = 8; --i > 0;) {
		if (x & (1 << i)) return i + 1;
	}
	return 1;
}

vector<uint8_t> load(const string& path) {
	ifstream file(path, ios::binary);
	istreambuf_iterator<char> start = istreambuf_iterator<char>(file);
	return vector<uint8_t>(start, istreambuf_iterator<char>());
}

bool save(const string& path, vector<uint8_t>& data) {
	ofstream file(path, ios::binary);
	if (!file.good()) { return false; }
	for (auto x : data) { file << x; }
	return true;
}

void push_to_vector(vector<uint8_t>& vec, const uint8_t* pPacked, uint32_t byteNo) {
	for (uint32_t i = 0; i < byteNo; ++i) {
		vec.push_back(pPacked[i]);
	}
}

void push_to_vector(vector<uint8_t>& vec, uint32_t val) {
	vec.push_back(val & 0xFF);
	vec.push_back((val>>8) & 0xFF);
	vec.push_back((val >> 16) & 0xFF);
	vec.push_back((val >> 24) & 0xFF);
}

void pack(vector<uint8_t>& data, vector<uint8_t>& packed) {
	struct FreqTblEntry {
		uint8_t id;
		uint32_t freq;
	} tbl[0x100];
	char xlat[0x100];
	uint8_t rev[0x100];
	size_t dataSz = data.size();

	// The worst case - the allocated size is 10 bit per item (3 : length + 7 : encoded value) ==> dataSz * 1.25
	uint32_t maxPackedSz = dataSz + (dataSz >> 2) + 1;
	uint8_t* pPacked = new uint8_t[maxPackedSz];
	fill(pPacked, pPacked + maxPackedSz, 0);

	for (int i = 0; i < 0x100; ++i) {
		tbl[i].id = i;
		tbl[i].freq = 0;
	}
	for (auto x : data) { ++tbl[x].freq; }
	sort(tbl, tbl + 0x100, [](FreqTblEntry& t1, FreqTblEntry& t2) { return t1.freq > t2.freq; });

	uint32_t revTblSz = 0;
	for (int i = 0; i < 0x100; ++i) {
		if (i == 0xFF) {
			cout << "dbg stop\n";
		}
		xlat[tbl[i].id] = char(i);
		if (tbl[i].freq > 0) {
			rev[i] = tbl[i].id;
			++revTblSz;
		}
	}

	uint32_t bitCnt = 0;
	uint32_t byteIdx = 0;
	uint8_t bitIdx = 0;
	uint32_t sizeDiff = 0;

	for (auto sym : data) {
		uint8_t code = xlat[sym];
		uint32_t len = get_bit_len(code);

		byteIdx = bitCnt >> 3;
		bitIdx = bitCnt & 7;

		pPacked[byteIdx] |= (len - 1) << bitIdx;
		pPacked[byteIdx + 1] |= (len - 1) >> (8 - bitIdx);
		bitCnt += 3;

		uint8_t codeToWrite = code;
		uint8_t bitsTaken = len;
		if (len > 1) {
			codeToWrite -= (1 << (len - 1));
			--bitsTaken;
			++sizeDiff;
		}

		byteIdx = bitCnt >> 3;
		bitIdx = bitCnt & 7;
		pPacked[byteIdx] |= codeToWrite << bitIdx;
		pPacked[byteIdx + 1] |= codeToWrite >> (8 - bitIdx);
		bitCnt += bitsTaken;
	}

	uint32_t packedSz = byteIdx + 1;
	packed.reserve(HEADER_SZ + revTblSz + packedSz);

	packed.push_back('S');
	packed.push_back('P');
	packed.push_back('C');
	packed.push_back('K');
	push_to_vector(packed, dataSz);
	push_to_vector(packed, packedSz);
	packed.push_back(0); // RFU : encoding type
	
	// Store the table size - 1 to fit into a byte
	packed.push_back((uint8_t)((revTblSz - 1) & 0xFF));
	push_to_vector(packed, rev, revTblSz);
	push_to_vector(packed, pPacked, packedSz);

	delete[] pPacked;

	cout << "Packed size : " << packedSz << "\n";
	cout << "Pack ratio : " << (float)packedSz / (float)dataSz << "\n";
	uint32_t altBitSz = bitCnt + sizeDiff;
	uint32_t altByteSz = (altBitSz >> 3) + 1;
	cout << "Alt packed size : " << altByteSz << "\n";
}


int main(int argc, char* argv[]) {

	vector<uint8_t> packed;

	if (argc < 2) {
		cout << "Please provide a file path.\n";
		return -1;
	}

	cout << "Packing " << argv[1] << "\n";
	vector<uint8_t> data = load(argv[1]);
	if (data.size() > 0) {
		cout << "Loaded " << data.size() << " bytes\n";
	} else {
		cout << "Can't load the file\n";
		return -1;
	}

	pack(data, packed);

	string outFile;
	if (argc >= 3) {
		outFile = argv[2];
	} else {
		outFile = string(argv[1]) + ".spck";
	}
	if (!save(outFile, packed)) {
		cout << "Can't save the packed file\n";
		return -1;
	}
	cout << "Done\n";
	return 0;
}
