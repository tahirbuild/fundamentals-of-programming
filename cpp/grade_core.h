// grade_core.h
// Core logic for the Student Grade Manager (CS-114 Fundamentals of Programming).
// Deliberately written in "FoP style": structs, fixed-size arrays, functions,
// loops, linear search and bubble sort - no classes or STL containers.

#ifndef GRADE_CORE_H
#define GRADE_CORE_H

#include <string>

const int MAX_STUDENTS = 100;
const int NUM_SUBJECTS = 5;
const int NUM_GRADES = 8;   // A, B+, B, C+, C, D+, D, F
const int MAX_NAME_LEN = 40;

struct Subject {
    const char* code;
    const char* title;
    int credits;
};

struct Student {
    int id;
    std::string name;
    double marks[NUM_SUBJECTS];
};

struct ClassStats {
    int count;
    double averageGpa;
    double highestGpa;
    double lowestGpa;
    int topperIndex;                       // index into students[], -1 if empty
    double subjectAverage[NUM_SUBJECTS];
    int gradeCount[NUM_GRADES];            // how many subject grades fell in each band
    int goodStandingCount;                 // no F and GPA >= 2.0
};

enum Status {
    STATUS_OK = 0,
    ERR_FULL,
    ERR_DUPLICATE_ID,
    ERR_NOT_FOUND,
    ERR_INVALID_ID,
    ERR_INVALID_NAME,
    ERR_INVALID_MARKS,
    ERR_PARSE
};

enum SortMode {
    SORT_BY_ID = 0,
    SORT_BY_NAME = 1,
    SORT_BY_GPA = 2
};

// ---- Global storage (a plain array, as taught in FoP) ----
extern const Subject SUBJECTS[NUM_SUBJECTS];
extern const char* GRADE_LETTERS[NUM_GRADES];
extern Student students[MAX_STUDENTS];
extern int studentCount;

// ---- Record management ----
const char* statusMessage(int status);
void clearAll();
int addStudent(int id, const std::string& name, const double marks[]);
int updateStudent(int id, const std::string& name, const double marks[]);
int removeStudent(int id);
int findIndexById(int id);                 // linear search, -1 if absent
bool nameMatches(const std::string& name, const std::string& query); // case-insensitive substring
void sortStudents(int mode);               // bubble sort
void loadSampleData();

// ---- Grading ----
int gradeIndex(double mark);               // 0..7 into GRADE_LETTERS
const char* letterGrade(double mark);
double gradePoint(double mark);
double averageMarks(const Student& s);
double computeGpa(const Student& s);
int failedSubjects(const Student& s);
const char* standing(const Student& s);
ClassStats computeStats();

// ---- Persistence: one student per line -> id|name|m1|m2|m3|m4|m5 ----
std::string serialize();
int deserialize(const std::string& text, int* loadedCount);

#endif
