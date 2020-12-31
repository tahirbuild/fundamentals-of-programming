// wasm_api.cpp
// Thin bridge that exposes the C++ core to JavaScript via WebAssembly.
// All grading, validation, searching, sorting and file parsing still happens
// in grade_core.cpp - the React UI only displays what these functions return.
//
// Build: see ../build-wasm.sh

#include <emscripten/emscripten.h>
#include <cstdio>
#include <string>
#include "grade_core.h"

static std::string output;   // keeps returned strings alive until the next call

static std::string jsonEscape(const std::string& text) {
    std::string out;
    for (int i = 0; i < (int)text.length(); i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        } else out += (char)c;
    }
    return out;
}

static std::string number(double value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4g", value);
    return buf;
}

static std::string studentJson(const Student& s) {
    std::string out = "{\"id\":" + std::to_string(s.id);
    out += ",\"name\":\"" + jsonEscape(s.name) + "\"";
    out += ",\"marks\":[";
    for (int i = 0; i < NUM_SUBJECTS; i++) out += (i ? "," : "") + number(s.marks[i]);
    out += "],\"grades\":[";
    for (int i = 0; i < NUM_SUBJECTS; i++) out += std::string(i ? "," : "") + "\"" + letterGrade(s.marks[i]) + "\"";
    out += "],\"points\":[";
    for (int i = 0; i < NUM_SUBJECTS; i++) out += (i ? "," : "") + number(gradePoint(s.marks[i]));
    out += "],\"average\":" + number(averageMarks(s));
    out += ",\"gpa\":" + number(computeGpa(s));
    out += ",\"failed\":" + std::to_string(failedSubjects(s));
    out += ",\"standing\":\"" + jsonEscape(standing(s)) + "\"}";
    return out;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* gm_subjects() {
    output = "[";
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        if (i) output += ",";
        output += "{\"code\":\"" + jsonEscape(SUBJECTS[i].code) + "\",\"title\":\"" +
                  jsonEscape(SUBJECTS[i].title) + "\",\"credits\":" + std::to_string(SUBJECTS[i].credits) + "}";
    }
    output += "]";
    return output.c_str();
}

EMSCRIPTEN_KEEPALIVE
int gm_add(int id, const char* name, double m0, double m1, double m2, double m3, double m4) {
    double marks[NUM_SUBJECTS] = {m0, m1, m2, m3, m4};
    return addStudent(id, name, marks);
}

EMSCRIPTEN_KEEPALIVE
int gm_update(int id, const char* name, double m0, double m1, double m2, double m3, double m4) {
    double marks[NUM_SUBJECTS] = {m0, m1, m2, m3, m4};
    return updateStudent(id, name, marks);
}

EMSCRIPTEN_KEEPALIVE
int gm_remove(int id) { return removeStudent(id); }

EMSCRIPTEN_KEEPALIVE
void gm_clear() { clearAll(); }

EMSCRIPTEN_KEEPALIVE
void gm_load_sample() { loadSampleData(); }

EMSCRIPTEN_KEEPALIVE
int gm_count() { return studentCount; }

EMSCRIPTEN_KEEPALIVE
const char* gm_status_message(int status) { return statusMessage(status); }

EMSCRIPTEN_KEEPALIVE
const char* gm_letter_grade(double mark) { return letterGrade(mark); }

EMSCRIPTEN_KEEPALIVE
double gm_grade_point(double mark) { return gradePoint(mark); }

// Sorts the stored array (bubble sort) and returns students whose name matches
// the query (or whose ID starts with it).
EMSCRIPTEN_KEEPALIVE
const char* gm_list(int sortMode, const char* query) {
    sortStudents(sortMode);
    std::string q = query ? query : "";
    output = "[";
    bool first = true;
    for (int i = 0; i < studentCount; i++) {
        std::string idText = std::to_string(students[i].id);
        bool idMatch = q.length() > 0 && idText.compare(0, q.length(), q) == 0;
        if (nameMatches(students[i].name, q) || idMatch) {
            if (!first) output += ",";
            output += studentJson(students[i]);
            first = false;
        }
    }
    output += "]";
    return output.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* gm_get(int id) {
    int index = findIndexById(id);
    output = index == -1 ? "null" : studentJson(students[index]);
    return output.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* gm_stats() {
    ClassStats st = computeStats();
    output = "{\"count\":" + std::to_string(st.count);
    output += ",\"averageGpa\":" + number(st.averageGpa);
    output += ",\"highestGpa\":" + number(st.highestGpa);
    output += ",\"lowestGpa\":" + number(st.lowestGpa);
    output += ",\"goodStanding\":" + std::to_string(st.goodStandingCount);
    output += ",\"topper\":" + (st.topperIndex >= 0 ? studentJson(students[st.topperIndex]) : std::string("null"));
    output += ",\"subjectAverage\":[";
    for (int i = 0; i < NUM_SUBJECTS; i++) output += (i ? "," : "") + number(st.subjectAverage[i]);
    output += "],\"gradeLetters\":[";
    for (int g = 0; g < NUM_GRADES; g++) output += std::string(g ? "," : "") + "\"" + GRADE_LETTERS[g] + "\"";
    output += "],\"gradeCount\":[";
    for (int g = 0; g < NUM_GRADES; g++) output += (g ? "," : "") + std::to_string(st.gradeCount[g]);
    output += "]}";
    return output.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* gm_serialize() {
    output = serialize();
    return output.c_str();
}

EMSCRIPTEN_KEEPALIVE
int gm_load(const char* text) {
    int loaded = 0;
    return deserialize(text ? text : "", &loaded);
}

}  // extern "C"
