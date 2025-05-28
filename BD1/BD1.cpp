#include "MasterFile.h"
#include "SlaveFile.h"
#include "Global.h"
    
MasterFile master("S.fl", "S.ind");
SlaveFile slave("SP.fl");

// Додає новий запис у головний файл
void insert_m() {
    MasterRecord rec;
    cout << "Enter KP: "; cin >> rec.KP;
    cout << "Surname: "; cin >> rec.surname;
    cout << "Status: "; cin >> rec.status;
    cout << "City: "; cin >> rec.city;
    rec.firstSP = -1;
    rec.countSP = 0;
    rec.deleted = false;
    

    if (master.insert(rec)) {
        cout << "Inserted successfully\n";
    }
    else {
        cout << "Insert failed (duplicate KP)\n";
    }
}

// Додає новий підлеглий запис
void insert_s() {
    int kp;
    cout << "Enter supplier KP: "; cin >> kp;
    try {
        MasterRecord m = master.get(kp);
        SlaveRecord s;
        cout << "Enter KD: "; cin >> s.KD;
        cout << "Price: "; cin >> s.price;
        cout << "Quantity: "; cin >> s.quantity;
        s.KP = kp;
        s.next = m.firstSP; 
        s.deleted = false;  

        int pos = slave.insert(s);
        if (pos != -1) {
            m.firstSP = pos;  
            m.countSP++;     
            int masterPos = master.find(kp);
            if (masterPos != -1) {
                master.update(masterPos, m); // Оновлюємо майстер-запис
                cout << "Inserted successfully\n";
            }
            else {
                cerr << "Error: Master record not found after insertion\n";
            }
        }
        else {
             cerr << "Error: Failed to insert slave record\n";
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}


// Функція для виведення інформації про головний запис
void print_master(const MasterRecord& m) {
    cout << "KP: " << m.KP << "\nSurname: " << m.surname
        << "\nStatus: " << m.status << "\nCity: " << m.city
        << "\nSupplies: " << m.countSP << endl;
}

// Функція для виведення інформації про підлеглий запис
void print_slave(const SlaveRecord& s) {
    cout << "KP: " << s.KP << "\nKD: " << s.KD
        << "\nPrice: " << s.price << "\nQuantity: " << s.quantity << endl;
}

// Отримання та виведення головного запису за KP
void get_m() {
    int kp;
    cout << "Enter KP: "; cin >> kp;
    try {
        MasterRecord m = master.get(kp);
        print_master(m);
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// Отримання та виведення підлеглого запису за ключами KP та KD
void get_s() {
    int kp, kd;
    cout << "Enter KP: "; cin >> kp;
    cout << "Enter KD: "; cin >> kd;
    try {
        MasterRecord m = master.get(kp);
        cout << "Master record found. FirstSP: " << m.firstSP << endl;
        if (m.firstSP == -1) {
            cout << "Not found" << endl;
            return;
        }
        int current = m.firstSP;
        while (current != -1) {
            try {
                SlaveRecord s = slave.get(current);
                cout << "Checking slave record at position: " << current << ", KD: " << s.KD << endl;
                if (s.KD == kd && !s.deleted) {
                    print_slave(s);
                    return;
                }
                current = s.next;
            }
            catch (...) {
                cerr << "Skipping invalid slave record at " << current << endl;
                break;
            }
        }
        cout << "Not found" << endl;
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// Виведення всіх головних записів
void ut_m() {
    for (const auto& m : master.getAll()) {
        print_master(m);
        cout << "----------------\n";
    }
}

// Виведення всіх підлеглих записів
void ut_s() {
    auto slaves = slave.getAll();
    cout << "Total slave records: " << slaves.size() << endl; // 

    for (const auto& s : slaves) {
        cout << "Slave Record - KP: " << s.KP << ", KD: " << s.KD << endl; // 
        print_slave(s);
        cout << "----------------\n";
    }
}


// Підрахунок кількості головних записів
void calc_m() {
    vector<MasterRecord> records = master.getAll();
    cout << "Total master records: " << records.size() << endl;

    for (const auto& m : records) {
        cout << "KP: " << m.KP << " has " << m.countSP << " supplies\n";
    }
}

// Підрахунок загальної вартості поставок для конкретного постачальника
void calc_s() {
    int kp;
    cout << "Enter KP: "; cin >> kp;

    try {
        MasterRecord m = master.get(kp);
        cout << "Master record found. FirstSP: " << m.firstSP << endl;
        if (m.firstSP == -1) {
            cout << "No slave records found." << endl;
            return;
        }

        int current = m.firstSP;
        double total = 0;
        int count = 0;

        while (current != -1) {
            try {
                SlaveRecord s = slave.get(current);
                cout << "Processing slave: KD=" << s.KD << ", Price=" << s.price << ", Quantity=" << s.quantity << endl;
                if (!s.deleted) {
                    total += s.price * s.quantity;
                    count++;
                }
                current = s.next;
            }
            catch (const runtime_error& e) {
                cerr << "Error accessing slave record at " << current << endl;
                break;
            }
        }

        cout << "Total price: " << total << "\nCount: " << count << endl;
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}


// Оновлення даних головного запису
void update_m() {
    int kp;
    cout << "Enter KP to update: "; cin >> kp;

    try {
        MasterRecord oldRecord = master.get(kp);
        MasterRecord newRecord = oldRecord; // Копіюємо старі дані

        cout << "New Surname: "; cin >> newRecord.surname;
        cout << "New Status: "; cin >> newRecord.status;
        cout << "New City: "; cin >> newRecord.city;

        // Видаляємо старий запис з індексу та файлу
        master.remove(kp);

        // Вставляємо оновлений запис
        if (master.insert(newRecord)) {
            cout << "Updated successfully\n";
        }
        else {
            cout << "Update failed (duplicate KP)\n";
        }
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// Оновлення даних підлеглого запису
void update_s() {
    int kp, kd;
    cout << "Enter KP: "; cin >> kp;
    cout << "Enter KD: "; cin >> kd;
    try {
        MasterRecord m = master.get(kp);
        int current = m.firstSP;
        while (current != -1) {
            try {
                SlaveRecord s = slave.get(current);
                if (s.KD == kd && !s.deleted) {
                    cout << "New Price: "; cin >> s.price;
                    cout << "New Quantity: "; cin >> s.quantity;
                    slave.update(current, s);
                    cout << "Updated successfully" << endl;
                    return;
                }
                current = s.next;
            }
            catch (...) {
                cerr << "Skipping invalid slave record at " << current << endl;
                break;
            }
        }
        cout << "Not found" << endl;
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// Видалення головного запису та всіх пов'язаних підлеглих записів
void del_m() {
    int kp;
    cout << "Enter KP: "; cin >> kp;
    try {

        MasterRecord m = master.get(kp);

        m.deleted = true;
        int masterPos = master.find(kp);
        master.update(masterPos, m);
        int current = m.firstSP;
        while (current != -1) {
            SlaveRecord s = slave.get(current);
            s.deleted = true;
            slave.update(current, s);
            current = s.next;
        }
        cout << "Master record KP " << kp << " and associated slaves marked as deleted." << endl;
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// Видалення підлеглого запису та оновлення зв'язків
void del_s() {
    int kp, kd;
    cout << "Enter KP: "; cin >> kp;
    cout << "Enter KD: "; cin >> kd;

    try {
        MasterRecord m = master.get(kp);
        cout << "Master record found. FirstSP: " << m.firstSP << endl;
        if (m.firstSP == -1) {
            cout << "No slave records to delete." << endl;
            return;
        }

        int current = m.firstSP;
        int prev = -1;
        bool found = false;

        while (current != -1) {
            try {
                SlaveRecord s = slave.get(current);
                cout << "Checking slave: KD=" << s.KD << " at position " << current << endl;
                if (s.KD == kd && !s.deleted) {
                    found = true;
                    if (prev == -1) {
                        m.firstSP = s.next;
                    }
                    else {
                        SlaveRecord prevS = slave.get(prev);
                        prevS.next = s.next;
                        slave.update(prev, prevS);
                    }
                    slave.remove(current);
                    m.countSP--;
                    master.update(master.find(kp), m);
                    cout << "Deleted successfully" << endl;
                    return;
                }
                prev = current;
                current = s.next;
            }
            catch (...) {
                cerr << "Skipping invalid slave record at " << current << endl;
                break;
            }
        }
        if (!found) cout << "Not found" << endl;
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

void compact_master() {
    master.compact();

    vector<MasterRecord> masters = master.getAll();
    for (MasterRecord& m : masters) {
        int current = m.firstSP;
        int prevPos = -1;
        m.firstSP = -1; 

        while (current != -1) {
            SlaveRecord s = slave.get(current);
            if (!s.deleted) {
                if (m.firstSP == -1) {
                    m.firstSP = current; 
                }
                else {
                    SlaveRecord prevS = slave.get(prevPos);
                    prevS.next = current;
                    slave.update(prevPos, prevS);
                }
                prevPos = current;
            }
            current = s.next;
        }

       
        int pos = master.find(m.KP);
        master.update(pos, m);
    }

    cout << "Master file compacted and slave references updated." << endl;
}


void compact_slave() {
    slave.compact();
    cout << "Slave file compacted." << endl;
}



int main() {
    string command;
    while (true) {
        cout << "Enter command: ";
        cin >> command;
        if (command == "insert-m") insert_m();
        else if (command == "insert-s") insert_s();
        else if (command == "get-m") get_m();
        else if (command == "get-s") get_s();
        else if (command == "del-m") del_m();
        else if (command == "del-s") del_s();
        else if (command == "update-m") update_m();
        else if (command == "update-s") update_s();
        else if (command == "calc-m") calc_m();
        else if (command == "calc-s") calc_s();
        else if (command == "ut-m") ut_m();
        else if (command == "ut-s") ut_s();
        else if (command == "compact-m") compact_master();
        else if (command == "compact-s") compact_slave();
        else if (command == "exit") break;
        else cout << "Unknown command\n";
    }
    return 0;
}
