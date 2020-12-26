# Student Grade Manager

**Course:** CS-114 Fundamentals of Programming · Semester 1 · BE Computer Engineering, NUST CEME
**Stack:** C++17 · WebAssembly (Emscripten) · React + Vite
**Live demo:** _add your Vercel URL here_

A grade management system written in C++ that stores students, computes letter grades and credit-weighted GPA, searches and sorts records, and saves them to a text file. The same C++ source builds two ways:

- **Console app** (`g++`): the classic menu-driven program.
- **Web app** (`em++` → WebAssembly): the C++ runs in your browser behind a React interface, so the site is fully static.

> This repository is part of a rebuilt portfolio of my 2020–2024 university coursework.

## Features

- Add, update, delete and list student records (max 100, fixed-size array)
- Letter grades and credit-hour-weighted GPA across 5 first-semester courses
- Academic standing: Dean's Honor List / Good Standing / Probation / Repeat Required
- Search by name (case-insensitive substring) or ID, implemented as a linear search
- Sort by ID, name or GPA, implemented as a bubble sort
- Class report: average / highest / lowest GPA, subject averages, grade distribution
- File persistence with a plain-text format shared by both versions (`id|name|m1|m2|m3|m4|m5`)
- Input validation everywhere (IDs, names, marks 0–100, all-or-nothing file loading)

## FoP concepts used

Structs · fixed-size arrays · functions · loops and conditionals · `switch` menus · linear search · bubble sort · string handling · file I/O (`ifstream` / `ofstream`) · input validation. There are deliberately no classes or STL containers: that's the next course (OOP).

## Project structure

```
cpp/
  grade_core.h / .cpp   all program logic (shared by both builds)
  main.cpp              console menu program
  wasm_api.cpp          exports the core to JavaScript
web/                    React UI (Vite)
  src/wasm/             compiled WebAssembly module (committed)
build-wasm.sh           rebuilds web/src/wasm from the C++ source
```

## Run the console version

```bash
cd cpp
g++ -std=c++17 -Wall -O2 -static main.cpp grade_core.cpp -o grade_manager
./grade_manager
```

`-static` avoids DLL conflicts on Windows (MinGW). Choose option **9** to load sample data.

## Run the web version

```bash
cd web
npm install
npm run dev
```

The compiled C++ is already in `web/src/wasm/`, so you only need Node.js. To rebuild it after changing the C++, install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html), then:

```bash
./build-wasm.sh
```

## Deploy (Vercel)

Import the GitHub repo in Vercel and set **Root Directory** to `web`. Vercel detects Vite automatically (build command `npm run build`, output directory `dist`).

## Grading scale

| Grade | A | B+ | B | C+ | C | D+ | D | F |
|---|---|---|---|---|---|---|---|---|
| Marks | ≥85 | ≥80 | ≥75 | ≥70 | ≥65 | ≥60 | ≥55 | <55 |
| Points | 4.0 | 3.5 | 3.0 | 2.5 | 2.0 | 1.5 | 1.0 | 0.0 |

GPA = Σ(grade point × credit hours) ÷ total credit hours.
