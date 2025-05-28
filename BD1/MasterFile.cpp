#include "MasterFile.h"
#include "Global.h"
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

size_t MasterFile::headerSize() const {
    return sizeof(int) + MAX_FREE * sizeof(int);
}

size_t MasterFile::recordSize() const {
    return sizeof(MasterRecord);
}

// Конструктор
MasterFile::MasterFile(const string& fname, const string& idxFile)
    : filename(fname), indexFile(idxFile) {
    ifstream idxIn(idxFile, ios::binary);
    if (idxIn) {
        IndexEntry entry;
        while (idxIn.read((char*)&entry, sizeof(IndexEntry))) {
            index.push_back(entry);
        }
        sort(index.begin(), index.end(), lambdas.sortByKey);
    }
    else {
        ofstream idxOut(idxFile, ios::binary); 
        idxOut.close();
    }

    file.open(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        initializeFile();
    }
    else {
        int count;
        file.read((char*)&count, sizeof(int));
        freeList.resize(count);
        file.read((char*)freeList.data(), count * sizeof(int));
    }
}

MasterFile::~MasterFile() {
    ofstream idxOut(indexFile, ios::binary);
    for (const auto& entry : index) {
        idxOut.write((char*)&entry, sizeof(IndexEntry));
    }

    file.seekp(0);
    size_t count = freeList.size();
    file.write((char*)&count, sizeof(int));
    vector<int> freeSlots(MAX_FREE, -1);
    copy(freeList.begin(), freeList.end(), freeSlots.begin());
    file.write((char*)freeSlots.data(), MAX_FREE * sizeof(int));
    file.close();
}

void MasterFile::initializeFile() {
    file.open(filename, ios::out | ios::binary);
    int count = 0;
    file.write((char*)&count, sizeof(int));
    vector<int> freeSlots(MAX_FREE, -1);
    file.write((char*)freeSlots.data(), MAX_FREE * sizeof(int));
    file.close();
    file.open(filename, ios::in | ios::out | ios::binary);
}

bool MasterFile::insert(const MasterRecord& record) {
    if (find(record.KP) != -1) return false;

    int pos;
    if (!freeList.empty()) {
        pos = freeList.back();
        freeList.pop_back();
    }
    else {
        file.seekg(0, ios::end);
        std::streamoff currentPos = file.tellg();
        if (currentPos == -1) {
            cerr << "Error: Unable to get file position\n";
            return false;
        }
        pos = (currentPos - static_cast<std::streamoff>(headerSize())) / recordSize();
    }

    file.seekp(headerSize() + pos * recordSize());
    file.write((char*)&record, recordSize());

    IndexEntry entry{ record.KP, pos };
    index.insert(upper_bound(index.begin(), index.end(), entry.key, lambdas.insertByKey), entry);

    return true;
}

int MasterFile::find(int KP) const {
    auto it = lower_bound(index.begin(), index.end(), KP, lambdas.searchByKey);
    return (it != index.end() && it->key == KP) ? it->address : -1;
}

MasterRecord MasterFile::get(int KP) {
    int pos = find(KP);
    if (pos == -1) throw runtime_error("Not found");

    file.clear();
    file.seekg(headerSize() + pos * recordSize());
    MasterRecord record;
    file.read((char*)&record, recordSize());
    if (file.fail()) {
        throw runtime_error("Read error");
    }
    if (record.deleted) {
        cerr << "Record KP=" << KP << " at pos " << pos << " is marked as deleted" << endl;
        throw runtime_error("Deleted");
    }
    return record;
}

bool MasterFile::remove(int KP) {
    int pos = find(KP);
    if (pos == -1) return false;

    auto new_end = remove_if(index.begin(), index.end(), [this, KP](const IndexEntry& e) {
        return lambdas.removeByKey(e, KP);
        });
    index.erase(new_end, index.end());

    file.seekg(headerSize() + pos * recordSize());
    MasterRecord record;
    file.read((char*)&record, recordSize());
    if (!file.fail()) {
        record.deleted = true;
        file.seekp(headerSize() + pos * recordSize());
        file.write((char*)&record, recordSize());
        if (file.fail()) {
            cerr << "Failed to mark record KP=" << KP << " as deleted" << endl;
            return false;
        }
    }

    freeList.push_back(pos);
    cerr << "Deleted master record: KP=" << KP << ", pos=" << pos << endl;
    return true;
}

vector<MasterRecord> MasterFile::getAll() {
    vector<MasterRecord> records;
    file.seekg(headerSize());
    MasterRecord rec;
    while (file.read((char*)&rec, recordSize())) {
        if (!rec.deleted) records.push_back(rec);
    }

    if (records.empty()) {
        cout << "No valid records found in file.\n";
    }
    else {
        cout << "Total valid records: " << records.size() << endl;
    }
    return records;
}

bool MasterFile::update(int pos, const MasterRecord& record) {
    if (pos < 0) return false;

    file.seekp(headerSize() + pos * recordSize());
    file.write((char*)&record, recordSize());
    file.flush();

    auto it = lower_bound(index.begin(), index.end(), record.KP, lambdas.searchByKey);
    if (it != index.end() && it->key == record.KP) {
        it->address = pos;
    }

    cerr << "Updated master record: KP=" << record.KP << ", pos=" << pos << endl;
    return true;
}

void MasterFile::compact() {
    vector<MasterRecord> activeRecords;
    for (const auto& entry : index) {
        file.seekg(headerSize() + entry.address * recordSize());
        MasterRecord rec;
        file.read(reinterpret_cast<char*>(&rec), sizeof(MasterRecord));
        if (!rec.deleted) {
            activeRecords.push_back(rec);
        }
    }

    file.close();
    file.open(filename, ios::out | ios::binary | ios::trunc);
    if (!file) {
        cerr << "Error: Could not open file for compaction." << endl;
        return;
    }

    int count = 0;
    file.write(reinterpret_cast<const char*>(&count), sizeof(int));
    vector<int> freeSlots(MAX_FREE, -1);
    file.write(reinterpret_cast<const char*>(freeSlots.data()), MAX_FREE * sizeof(int));

    index.clear();
    freeList.clear();

    for (MasterRecord& rec : activeRecords) {
        std::streampos currentPos = file.tellp();
        int pos =  static_cast<int>(currentPos - static_cast<std::streamoff>(headerSize())) / recordSize();
        file.write(reinterpret_cast<const char*>(&rec), sizeof(MasterRecord));
        index.push_back(IndexEntry{ rec.KP, pos });
    }

    sort(index.begin(), index.end(), lambdas.sortByKey);
    file.close();
    file.open(filename, ios::in | ios::out | ios::binary);
}