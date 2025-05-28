#ifndef MASTERFILE_H
#define MASTERFILE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <functional>

using namespace std;


// Структура запису майстер-файлу
struct MasterRecord {
    int KP;
    char surname[50];
    int status;
    char city[50];
    int firstSP;
    int countSP;
    bool deleted;
};

// Елемент індексної таблиці
struct IndexEntry {
    int key;
    int address;
};


struct LambdaFunctions {
    function<bool(const IndexEntry&, const IndexEntry&)> sortByKey =
        [](const IndexEntry& a, const IndexEntry& b) { return a.key < b.key; };
    function<bool(const IndexEntry&, int)> searchByKey =
        [](const IndexEntry& e, int key) { return e.key < key; };
    function<bool(const IndexEntry&, int)> insertByKey =
        [](const IndexEntry& e, int key) { return e.key < key; };
    function<bool(const IndexEntry&, int)> removeByKey =
        [](const IndexEntry& e, int key) { return e.key == key; };
};


// Клас MasterFile
class MasterFile {
private:
    fstream file;
    vector<IndexEntry> index;
    vector<int> freeList;
    string filename;
    string indexFile;
    LambdaFunctions lambdas;

    size_t headerSize() const;
    size_t recordSize() const;

public:
    MasterFile(const string& fname, const string& idxFile);
    ~MasterFile();

    void initializeFile();
    bool insert(const MasterRecord& record);
    int find(int KP) const;
    MasterRecord get(int KP);
    bool remove(int KP);
    vector<MasterRecord> getAll();
    void compact();
    bool update(int pos, const MasterRecord& record);

};

#endif // MASTERFILE_H
