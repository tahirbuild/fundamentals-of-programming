// main.cpp
// Console version of the Student Grade Manager.
// Build:  g++ -std=c++17 -Wall -O2 main.cpp grade_core.cpp -o grade_manager
// Run:    ./grade_manager            (data is kept in students.txt)

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include "grade_core.h"

using namespace std;

const string DATA_FILE = "students.txt";

// ---------------- Input helpers ----------------

string readLine(const string& prompt) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) {
        cout << "\nInput closed. Exiting.\n";
        exit(0);
    }
    return line;
}

int readInt(const string& prompt) {
    while (true) {
        string line = readLine(prompt);
        stringstream ss(line);
        int value;
        char extra;
        if (ss >> value && !(ss >> extra)) return value;
        cout << "  Please enter a whole number.\n";
    }
}

double readMark(const string& prompt) {
    while (true) {
        string line = readLine(prompt);
        stringstream ss(line);
        double value;
        char extra;
        if (ss >> value && !(ss >> extra) && value >= 0 && value <= 100) return value;
        cout << "  Marks must be a number from 0 to 100.\n";
    }
}

void readAllMarks(double marks[]) {
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        marks[i] = readMark("  " + string(SUBJECTS[i].code) + " " + SUBJECTS[i].title + ": ");
    }
}

void pause() {
    readLine("\nPress Enter to continue...");
}

// ---------------- Output helpers ----------------

void printLine(char c, int width) {
    for (int i = 0; i < width; i++) cout << c;
    cout << "\n";
}

void printTableHeader() {
    printLine('-', 72);
    cout << left << setw(8) << "ID" << setw(24) << "Name"
         << right << setw(9) << "Average" << setw(7) << "GPA" << "   "
         << left << "Standing\n";
    printLine('-', 72);
}

void printRow(const Student& s) {
    cout << left << setw(8) << s.id << setw(24) << s.name
         << right << fixed << setprecision(1) << setw(9) << averageMarks(s)
         << setprecision(2) << setw(7) << computeGpa(s) << "   "
         << left << standing(s) << "\n";
}

void printReportCard(const Student& s) {
    printLine('=', 64);
    cout << "  REPORT CARD - Semester 1, Bachelor of Computer Engineering\n";
    printLine('=', 64);
    cout << "  Name: " << s.name << "\n  ID:   " << s.id << "\n\n";
    cout << "  " << left << setw(10) << "Code" << setw(32) << "Course"
         << right << setw(4) << "Cr" << setw(8) << "Marks" << setw(7) << "Grade" << "\n";
    cout << "  ";
    printLine('-', 61);
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        cout << "  " << left << setw(10) << SUBJECTS[i].code << setw(32) << SUBJECTS[i].title
             << right << setw(4) << SUBJECTS[i].credits
             << fixed << setprecision(1) << setw(8) << s.marks[i]
             << setw(7) << letterGrade(s.marks[i]) << "\n";
    }
    cout << "  ";
    printLine('-', 61);
    cout << fixed << setprecision(2)
         << "  Average: " << averageMarks(s) << "%    GPA: " << computeGpa(s)
         << "    Standing: " << standing(s) << "\n";
    printLine('=', 64);
}

// ---------------- File handling ----------------

void saveToFile() {
    ofstream file(DATA_FILE.c_str());
    if (!file) {
        cout << "  Could not open " << DATA_FILE << " for writing.\n";
        return;
    }
    file << serialize();
    cout << "  Saved " << studentCount << " record(s) to " << DATA_FILE << ".\n";
}

void loadFromFile(bool quiet) {
    ifstream file(DATA_FILE.c_str());
    if (!file) {
        if (!quiet) cout << "  " << DATA_FILE << " not found.\n";
        return;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    int loaded = 0;
    int status = deserialize(buffer.str(), &loaded);
    if (status == STATUS_OK) cout << "  Loaded " << loaded << " record(s) from " << DATA_FILE << ".\n";
    else cout << "  Load failed: " << statusMessage(status) << "\n";
}

// ---------------- Menu actions ----------------

void addMenu() {
    cout << "\n== Add Student ==\n";
    int id = readInt("  Student ID: ");
    string name = readLine("  Full name: ");
    double marks[NUM_SUBJECTS];
    cout << "  Enter marks (0-100):\n";
    readAllMarks(marks);
    int status = addStudent(id, name, marks);
    cout << "  " << statusMessage(status) << "\n";
}

void listMenu() {
    cout << "\n== All Students (" << studentCount << ") ==\n";
    if (studentCount == 0) {
        cout << "  No records yet.\n";
        return;
    }
    printTableHeader();
    for (int i = 0; i < studentCount; i++) printRow(students[i]);
}

void searchMenu() {
    cout << "\n== Search ==\n  1. By ID\n  2. By name\n";
    int choice = readInt("  Choice: ");
    if (choice == 1) {
        int index = findIndexById(readInt("  Student ID: "));
        if (index == -1) cout << "  " << statusMessage(ERR_NOT_FOUND) << "\n";
        else printReportCard(students[index]);
    } else if (choice == 2) {
        string query = readLine("  Name contains: ");
        int found = 0;
        for (int i = 0; i < studentCount; i++) {
            if (nameMatches(students[i].name, query)) {
                if (found == 0) printTableHeader();
                printRow(students[i]);
                found++;
            }
        }
        if (found == 0) cout << "  No matching students.\n";
    } else {
        cout << "  Invalid choice.\n";
    }
}

void updateMenu() {
    cout << "\n== Update Student ==\n";
    int id = readInt("  Student ID: ");
    int index = findIndexById(id);
    if (index == -1) {
        cout << "  " << statusMessage(ERR_NOT_FOUND) << "\n";
        return;
    }
    string name = readLine("  New name (Enter to keep \"" + students[index].name + "\"): ");
    if (name.length() == 0) name = students[index].name;
    double marks[NUM_SUBJECTS];
    cout << "  Enter new marks (0-100):\n";
    readAllMarks(marks);
    cout << "  " << statusMessage(updateStudent(id, name, marks)) << "\n";
}

void deleteMenu() {
    cout << "\n== Delete Student ==\n";
    int id = readInt("  Student ID: ");
    int index = findIndexById(id);
    if (index == -1) {
        cout << "  " << statusMessage(ERR_NOT_FOUND) << "\n";
        return;
    }
    string confirm = readLine("  Delete " + students[index].name + "? (y/n): ");
    if (confirm == "y" || confirm == "Y") cout << "  " << statusMessage(removeStudent(id)) << "\n";
    else cout << "  Cancelled.\n";
}

void statsMenu() {
    cout << "\n== Class Report ==\n";
    ClassStats st = computeStats();
    if (st.count == 0) {
        cout << "  No records yet.\n";
        return;
    }
    cout << fixed << setprecision(2);
    cout << "  Students:        " << st.count << "\n";
    cout << "  Average GPA:     " << st.averageGpa << "\n";
    cout << "  Highest GPA:     " << st.highestGpa << "  (" << students[st.topperIndex].name << ")\n";
    cout << "  Lowest GPA:      " << st.lowestGpa << "\n";
    cout << "  Good standing:   " << st.goodStandingCount << " / " << st.count << "\n\n";

    cout << "  Subject averages:\n";
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        cout << "    " << left << setw(10) << SUBJECTS[i].code << right << setw(6)
             << setprecision(1) << st.subjectAverage[i] << "  ";
        int bar = (int)(st.subjectAverage[i] / 4);
        for (int b = 0; b < bar; b++) cout << '#';
        cout << "\n";
    }

    cout << "\n  Grade distribution (all subjects):\n";
    for (int g = 0; g < NUM_GRADES; g++) {
        cout << "    " << left << setw(4) << GRADE_LETTERS[g] << right << setw(3) << st.gradeCount[g] << "  ";
        for (int b = 0; b < st.gradeCount[g]; b++) cout << '*';
        cout << "\n";
    }
}

void sortMenu() {
    cout << "\n== Sort ==\n  1. By ID\n  2. By name\n  3. By GPA (highest first)\n";
    int choice = readInt("  Choice: ");
    if (choice < 1 || choice > 3) {
        cout << "  Invalid choice.\n";
        return;
    }
    sortStudents(choice - 1);
    listMenu();
}

int main() {
    cout << "Student Grade Manager - CS-114 Fundamentals of Programming\n";
    loadFromFile(true);

    while (true) {
        cout << "\n";
        printLine('=', 40);
        cout << "  1. Add student\n"
             << "  2. View all students\n"
             << "  3. Search / report card\n"
             << "  4. Update student\n"
             << "  5. Delete student\n"
             << "  6. Class report\n"
             << "  7. Sort records\n"
             << "  8. Save to file\n"
             << "  9. Load sample data\n"
             << "  0. Save & exit\n";
        printLine('=', 40);
        int choice = readInt("Choose an option: ");

        switch (choice) {
            case 1: addMenu(); break;
            case 2: listMenu(); break;
            case 3: searchMenu(); break;
            case 4: updateMenu(); break;
            case 5: deleteMenu(); break;
            case 6: statsMenu(); break;
            case 7: sortMenu(); break;
            case 8: saveToFile(); break;
            case 9: loadSampleData(); cout << "  Loaded 8 sample students.\n"; break;
            case 0: saveToFile(); cout << "Goodbye!\n"; return 0;
            default: cout << "  Invalid option.\n";
        }
        if (choice != 0) pause();
    }
}
