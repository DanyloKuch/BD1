#include "SlaveFile.h"
#include "Global.h"
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

size_t SlaveFile::headerSize() const {
    return sizeof(int) + MAX_FREE * sizeof(int);
}

size_t SlaveFile::recordSize() const {
    return sizeof(SlaveRecord);
}

// Конструктор: Відкриває або створює підлеглий файл
SlaveFile::SlaveFile(const string& fname) : filename(fname) {
    file.open(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        cout << "File does not exist. Initializing..." << endl;  
        initializeFile();
    }
    else {
        cout << "File opened successfully: " << filename << endl;  

        int count;
        file.read((char*)&count, sizeof(int));
        if (file.fail()) {
            cerr << "Error reading free list count from file!" << endl;
        }
        else {
            freeList.resize(count);
            file.read((char*)freeList.data(), count * sizeof(int));
            cout << "Free list loaded. Count: " << count << endl;  
        }
    }
}

// Деструктор: Зберігає список вільних місць перед закриттям
SlaveFile::~SlaveFile() {
    file.seekp(0);
    int count = freeList.size();
    file.write((char*)&count, sizeof(int));
    vector<int> freeSlots(MAX_FREE, -1);
    copy(freeList.begin(), freeList.end(), freeSlots.begin());
    file.write((char*)freeSlots.data(), MAX_FREE * sizeof(int));
    file.close();
}

// Ініціалізує новий підлеглий файл
void SlaveFile::initializeFile() {
    file.open(filename, ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Could not initialize file: " << filename << endl;
        return;
    }

    int count = 0;  // Порожній список вільних позицій
    file.write(reinterpret_cast<char*>(&count), sizeof(int));

    vector<int> freeSlots(MAX_FREE, -1);
    file.write(reinterpret_cast<char*>(freeSlots.data()), MAX_FREE * sizeof(int));

    file.close();
    file.clear();  // Скидаємо стан 

    file.open(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Could not reopen file for read/write!" << endl;
    }
    else {
        cout << "File initialized successfully: " << filename << endl;  
    }
}


// Додає новий запис у головний файл
int SlaveFile::insert(const SlaveRecord& record) {
    int pos;
    if (!freeList.empty()) {
        pos = freeList.back();
        freeList.pop_back();
        cout << "Reusing free position: " << pos << endl;  
    }
    else {
        file.seekg(0, ios::end);
        std::streamoff currentPos = file.tellg();
        if (currentPos == -1) {
            cerr << "Error: Unable to get file position\n";
            return -1;
        }

        pos = (currentPos - static_cast<std::streamoff>(headerSize())) / static_cast<std::streamoff>(recordSize());
        cout << "New record position: " << pos << endl;
    }

    file.seekp(headerSize() + pos * recordSize());
    file.write((char*)&record, recordSize());
    file.flush();

    if (file.fail()) {
        cerr << "Error: Failed to write record at position " << pos << endl;
        return -1;
    }
    else {
        cout << "Inserted record: KP=" << record.KP << ", KD=" << record.KD << " at position " << pos << endl;
    }
    return pos;
}


SlaveRecord SlaveFile::get(int pos) {
    SlaveRecord rec;
    file.seekg(headerSize() + pos * recordSize());
    file.read((char*)&rec, recordSize());
    if (rec.deleted) throw runtime_error("Deleted");
    return rec;  // Повертаємо копію, а не посилання
}

// Видаляє запис за вказаною позицією
bool SlaveFile::remove(int pos) {
    if (pos < 0) return false;

    SlaveRecord rec = get(pos);  

    rec.deleted = true;  
    file.seekp(headerSize() + pos * recordSize());
    file.write(reinterpret_cast<char*>(&rec), recordSize());

    if (file.fail()) {
        cerr << "Error: Could not mark record as deleted." << endl;
        return false;
    }

    freeList.push_back(pos);
    return true;
}


// Оновлює запис за вказаною позицією
// Записує новий вміст запису у файл
bool SlaveFile::update(int pos, const SlaveRecord& record) {
    if (pos < 0) return false;

    // Remove the const qualifier here before writing to the file
    SlaveRecord nonConstRecord = record;  

    file.seekp(headerSize() + pos * recordSize());
    file.write(reinterpret_cast<char*>(&nonConstRecord), recordSize());

    if (file.fail()) {
        cerr << "Error: Could not update record at position " << pos << endl;
        return false;
    }

    cout << "Updated record: KP=" << nonConstRecord.KP << ", KD=" << nonConstRecord.KD << " at position " << pos << endl;
    return true;
}


// Отримує всі невидалені записи з файлу
// Читає записи послідовно та додає до результату тільки ті,що не позначені як видалені
vector<SlaveRecord> SlaveFile::getAll() {
    vector<SlaveRecord> records;
    file.clear();
    file.seekg(headerSize(), ios::beg);

    SlaveRecord rec;
    while (file.read(reinterpret_cast<char*>(&rec), recordSize())) {
        cout << "Read record: KP=" << rec.KP << ", KD=" << rec.KD << ", Deleted=" << rec.deleted << endl;
        if (!rec.deleted) {
            records.push_back(rec);
        }
    }

    if (records.empty()) {
        cout << "No valid records found in file.\n";
    }
    else {
        cout << "Total valid records: " << records.size() << endl;
    }

    return records;
}