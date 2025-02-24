#ifndef SLAVEFILE_H
#define SLAVEFILE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

// ��������� ������ slave-�����
struct SlaveRecord {
    int KP;       // ���� �������������
    int KD;       // ���� �����
    double price;
    int quantity;
    int next;     // ������ �������� ��������
    bool deleted; // ������ ���������
};

// ���� SlaveFile
class SlaveFile {
private:
    fstream file;
    vector<int> freeList;
    string filename;

    size_t headerSize() const;
    size_t recordSize() const;

public:
    SlaveFile(const string& fname);
    ~SlaveFile();


    void initializeFile();
    int insert(const SlaveRecord& record);
    SlaveRecord get(int pos);
    bool remove(int pos);
    vector<SlaveRecord> getAll();
    void compact();


    // New update method to modify an existing record
    bool update(int pos, const SlaveRecord& record); // Add this line
};

#endif // SLAVEFILE_H