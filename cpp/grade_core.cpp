// grade_core.cpp
// Implementation of the Student Grade Manager core logic.

#include "grade_core.h"
#include <cstdio>
#include <cstdlib>
#include <cctype>

// The five courses from a first-semester Computer Engineering timetable.
const Subject SUBJECTS[NUM_SUBJECTS] = {
    {"CS-114",   "Fundamentals of Programming",   3},
    {"EE-111",   "Linear Circuit Analysis",       4},
    {"MATH-101", "Calculus & Analytical Geometry", 3},
    {"PHY-102",  "Applied Physics",               3},
    {"HU-100",   "English",                       2}
};

const char* GRADE_LETTERS[NUM_GRADES] = {"A", "B+", "B", "C+", "C", "D+", "D", "F"};

// Lower bound (inclusive) of each grade band and its grade point.
static const double GRADE_MIN[NUM_GRADES]    = {85, 80, 75, 70, 65, 60, 55, 0};
static const double GRADE_POINTS[NUM_GRADES] = {4.0, 3.5, 3.0, 2.5, 2.0, 1.5, 1.0, 0.0};

Student students[MAX_STUDENTS];
int studentCount = 0;

// ------------------------------------------------------------------
// Validation helpers
// ------------------------------------------------------------------

static bool validId(int id) {
    return id >= 1 && id <= 999999;
}

static std::string trim(const std::string& text) {
    int start = 0;
    int end = (int)text.length() - 1;
    while (start <= end && isspace((unsigned char)text[start])) start++;
    while (end >= start && isspace((unsigned char)text[end])) end--;
    return text.substr(start, end - start + 1);
}

static bool validName(const std::string& name) {
    if (name.length() == 0 || name.length() > MAX_NAME_LEN) return false;
    for (int i = 0; i < (int)name.length(); i++) {
        char c = name[i];
        // '|' is our file separator and control characters break the file format.
        if (c == '|' || c == '\n' || c == '\r' || c == '\t') return false;
    }
    return true;
}

static bool validMarks(const double marks[]) {
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        // NaN fails both comparisons, so it is rejected too.
        if (!(marks[i] >= 0.0 && marks[i] <= 100.0)) return false;
    }
    return true;
}

const char* statusMessage(int status) {
    switch (status) {
        case STATUS_OK:         return "Success";
        case ERR_FULL:          return "Record limit reached (100 students)";
        case ERR_DUPLICATE_ID:  return "A student with this ID already exists";
        case ERR_NOT_FOUND:     return "Student not found";
        case ERR_INVALID_ID:    return "ID must be a number between 1 and 999999";
        case ERR_INVALID_NAME:  return "Name must be 1-40 characters and cannot contain '|'";
        case ERR_INVALID_MARKS: return "Marks must be between 0 and 100";
        case ERR_PARSE:         return "Could not read the data file";
        default:                return "Unknown error";
    }
}

// ------------------------------------------------------------------
// Record management
// ------------------------------------------------------------------

void clearAll() {
    studentCount = 0;
}

int findIndexById(int id) {
    for (int i = 0; i < studentCount; i++) {
        if (students[i].id == id) return i;
    }
    return -1;
}

int addStudent(int id, const std::string& rawName, const double marks[]) {
    std::string name = trim(rawName);
    if (!validId(id)) return ERR_INVALID_ID;
    if (!validName(name)) return ERR_INVALID_NAME;
    if (!validMarks(marks)) return ERR_INVALID_MARKS;
    if (findIndexById(id) != -1) return ERR_DUPLICATE_ID;
    if (studentCount >= MAX_STUDENTS) return ERR_FULL;

    Student& s = students[studentCount];
    s.id = id;
    s.name = name;
    for (int i = 0; i < NUM_SUBJECTS; i++) s.marks[i] = marks[i];
    studentCount++;
    return STATUS_OK;
}

int updateStudent(int id, const std::string& rawName, const double marks[]) {
    std::string name = trim(rawName);
    int index = findIndexById(id);
    if (index == -1) return ERR_NOT_FOUND;
    if (!validName(name)) return ERR_INVALID_NAME;
    if (!validMarks(marks)) return ERR_INVALID_MARKS;

    students[index].name = name;
    for (int i = 0; i < NUM_SUBJECTS; i++) students[index].marks[i] = marks[i];
    return STATUS_OK;
}

int removeStudent(int id) {
    int index = findIndexById(id);
    if (index == -1) return ERR_NOT_FOUND;
    // Shift everything after it one place to the left.
    for (int i = index; i < studentCount - 1; i++) {
        students[i] = students[i + 1];
    }
    studentCount--;
    return STATUS_OK;
}

bool nameMatches(const std::string& name, const std::string& query) {
    if (query.length() == 0) return true;
    if (query.length() > name.length()) return false;
    for (int start = 0; start + (int)query.length() <= (int)name.length(); start++) {
        bool match = true;
        for (int j = 0; j < (int)query.length(); j++) {
            if (tolower((unsigned char)name[start + j]) != tolower((unsigned char)query[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static std::string lowerCase(const std::string& text) {
    std::string out = text;
    for (int i = 0; i < (int)out.length(); i++) out[i] = (char)tolower((unsigned char)out[i]);
    return out;
}

// Returns true if a should come after b for the chosen sort mode.
static bool shouldSwap(const Student& a, const Student& b, int mode) {
    if (mode == SORT_BY_NAME) {
        std::string na = lowerCase(a.name), nb = lowerCase(b.name);
        if (na != nb) return na > nb;
        return a.id > b.id;
    }
    if (mode == SORT_BY_GPA) {
        double ga = computeGpa(a), gb = computeGpa(b);
        if (ga != gb) return ga < gb;   // highest GPA first
        return a.id > b.id;
    }
    return a.id > b.id;
}

void sortStudents(int mode) {
    // Bubble sort with early exit.
    for (int pass = 0; pass < studentCount - 1; pass++) {
        bool swapped = false;
        for (int i = 0; i < studentCount - 1 - pass; i++) {
            if (shouldSwap(students[i], students[i + 1], mode)) {
                Student temp = students[i];
                students[i] = students[i + 1];
                students[i + 1] = temp;
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

void loadSampleData() {
    clearAll();
    const int ids[] = {1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008};
    const char* names[] = {"Ayesha Khan", "Hamza Ali", "Fatima Noor", "Bilal Ahmed",
                           "Zainab Raza", "Usman Tariq", "Hira Saleem", "Omar Farooq"};
    const double marks[][NUM_SUBJECTS] = {
        {92, 88, 85, 90, 81},
        {78, 72, 66, 74, 70},
        {86, 91, 89, 84, 88},
        {58, 49, 62, 55, 68},
        {81, 77, 79, 83, 86},
        {67, 63, 71, 60, 75},
        {74, 80, 76, 72, 79},
        {88, 69, 54, 77, 73}
    };
    for (int i = 0; i < 8; i++) addStudent(ids[i], names[i], marks[i]);
}

// ------------------------------------------------------------------
// Grading
// ------------------------------------------------------------------

int gradeIndex(double mark) {
    for (int g = 0; g < NUM_GRADES; g++) {
        if (mark >= GRADE_MIN[g]) return g;
    }
    return NUM_GRADES - 1;
}

const char* letterGrade(double mark) {
    return GRADE_LETTERS[gradeIndex(mark)];
}

double gradePoint(double mark) {
    return GRADE_POINTS[gradeIndex(mark)];
}

double averageMarks(const Student& s) {
    double total = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) total += s.marks[i];
    return total / NUM_SUBJECTS;
}

// GPA = sum(grade point x credit hours) / total credit hours
double computeGpa(const Student& s) {
    double weighted = 0;
    int credits = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        weighted += gradePoint(s.marks[i]) * SUBJECTS[i].credits;
        credits += SUBJECTS[i].credits;
    }
    return weighted / credits;
}

int failedSubjects(const Student& s) {
    int failed = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        if (gradeIndex(s.marks[i]) == NUM_GRADES - 1) failed++;
    }
    return failed;
}

const char* standing(const Student& s) {
    double gpa = computeGpa(s);
    if (failedSubjects(s) > 0) return "Repeat Required";
    if (gpa < 2.0) return "Probation";
    if (gpa >= 3.5) return "Dean's Honor List";
    return "Good Standing";
}

ClassStats computeStats() {
    ClassStats stats;
    stats.count = studentCount;
    stats.averageGpa = 0;
    stats.highestGpa = 0;
    stats.lowestGpa = 0;
    stats.topperIndex = -1;
    stats.goodStandingCount = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++) stats.subjectAverage[i] = 0;
    for (int g = 0; g < NUM_GRADES; g++) stats.gradeCount[g] = 0;

    if (studentCount == 0) return stats;

    double gpaTotal = 0;
    stats.lowestGpa = 4.0;
    for (int i = 0; i < studentCount; i++) {
        double gpa = computeGpa(students[i]);
        gpaTotal += gpa;
        if (stats.topperIndex == -1 || gpa > stats.highestGpa) {
            stats.highestGpa = gpa;
            stats.topperIndex = i;
        }
        if (gpa < stats.lowestGpa) stats.lowestGpa = gpa;
        if (failedSubjects(students[i]) == 0 && gpa >= 2.0) stats.goodStandingCount++;

        for (int j = 0; j < NUM_SUBJECTS; j++) {
            stats.subjectAverage[j] += students[i].marks[j];
            stats.gradeCount[gradeIndex(students[i].marks[j])]++;
        }
    }
    stats.averageGpa = gpaTotal / studentCount;
    for (int j = 0; j < NUM_SUBJECTS; j++) stats.subjectAverage[j] /= studentCount;
    return stats;
}

// ------------------------------------------------------------------
// Persistence
// ------------------------------------------------------------------

std::string serialize() {
    std::string out;
    char buffer[64];
    for (int i = 0; i < studentCount; i++) {
        snprintf(buffer, sizeof(buffer), "%d", students[i].id);
        out += buffer;
        out += "|";
        out += students[i].name;
        for (int j = 0; j < NUM_SUBJECTS; j++) {
            snprintf(buffer, sizeof(buffer), "|%g", students[i].marks[j]);
            out += buffer;
        }
        out += "\n";
    }
    return out;
}

// Splits one line on '|' into at most maxParts fields. Returns number of fields.
static int splitLine(const std::string& line, std::string parts[], int maxParts) {
    int count = 0;
    std::string current;
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] == '|') {
            if (count < maxParts) parts[count] = current;
            count++;
            current = "";
        } else {
            current += line[i];
        }
    }
    if (count < maxParts) parts[count] = current;
    return count + 1;
}

static bool parseNumber(const std::string& text, double* value) {
    std::string t = trim(text);
    if (t.length() == 0) return false;
    char* end = 0;
    *value = strtod(t.c_str(), &end);
    return *end == '\0';
}

// Loads all-or-nothing: if any line is invalid, the current data is kept.
int deserialize(const std::string& text, int* loadedCount) {
    static Student backup[MAX_STUDENTS];
    int backupCount = studentCount;
    for (int i = 0; i < studentCount; i++) backup[i] = students[i];

    clearAll();
    const int FIELDS = 2 + NUM_SUBJECTS;
    std::string line;
    int result = STATUS_OK;

    for (int pos = 0; pos <= (int)text.length() && result == STATUS_OK; pos++) {
        if (pos < (int)text.length() && text[pos] != '\n') {
            if (text[pos] != '\r') line += text[pos];
            continue;
        }
        // End of a line.
        if (trim(line).length() > 0) {
            std::string parts[FIELDS];
            double id = 0;
            double marks[NUM_SUBJECTS];
            if (splitLine(line, parts, FIELDS) != FIELDS || !parseNumber(parts[0], &id)) {
                result = ERR_PARSE;
            } else {
                for (int j = 0; j < NUM_SUBJECTS && result == STATUS_OK; j++) {
                    if (!parseNumber(parts[2 + j], &marks[j])) result = ERR_PARSE;
                }
                if (result == STATUS_OK) {
                    if (!(id >= 1 && id <= 999999) || id != (int)id) result = ERR_INVALID_ID;
                    else result = addStudent((int)id, parts[1], marks);
                }
            }
        }
        line = "";
    }

    if (result != STATUS_OK) {
        for (int i = 0; i < backupCount; i++) students[i] = backup[i];
        studentCount = backupCount;
        if (loadedCount) *loadedCount = 0;
        return result;
    }
    if (loadedCount) *loadedCount = studentCount;
    return STATUS_OK;
}
