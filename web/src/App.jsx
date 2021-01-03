import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { createEngine } from './gradeEngine.js';

const SORT_OPTIONS = [
  { value: 0, label: 'ID' },
  { value: 1, label: 'Name' },
  { value: 2, label: 'GPA (high → low)' },
];

const REPO_URL = 'https://github.com/'; // TODO: replace with the project's GitHub repo URL

function standingClass(standing) {
  if (standing.startsWith('Dean')) return 'badge badge-honor';
  if (standing === 'Good Standing') return 'badge badge-good';
  if (standing === 'Probation') return 'badge badge-warn';
  return 'badge badge-bad';
}

function gradeClass(letter) {
  if (letter === 'F') return 'grade grade-f';
  if (letter.startsWith('A') || letter.startsWith('B')) return 'grade grade-high';
  return 'grade grade-mid';
}

const fmt = (n, d = 2) => Number(n).toFixed(d);

export default function App() {
  const [engine, setEngine] = useState(null);
  const [loadError, setLoadError] = useState('');
  const [version, setVersion] = useState(0);
  const [tab, setTab] = useState('students');
  const [query, setQuery] = useState('');
  const [sortMode, setSortMode] = useState(0);
  const [calls, setCalls] = useState([]);
  const [form, setForm] = useState(null);         // { mode: 'add' | 'edit', student? }
  const [reportId, setReportId] = useState(null);
  const [toast, setToast] = useState(null);

  const logCall = useCallback((entry) => {
    setCalls((prev) => [entry, ...prev].slice(0, 40));
  }, []);

  useEffect(() => {
    let cancelled = false;
    createEngine(logCall)
      .then((e) => !cancelled && setEngine(e))
      .catch((err) => !cancelled && setLoadError(String(err)));
    return () => {
      cancelled = true;
    };
  }, [logCall]);

  const refresh = () => setVersion((v) => v + 1);
  const notify = (message, ok = true) => {
    setToast({ message, ok, id: Date.now() });
  };

  useEffect(() => {
    if (!toast) return undefined;
    const t = setTimeout(() => setToast(null), 3000);
    return () => clearTimeout(t);
  }, [toast]);

  const students = useMemo(
    () => (engine ? engine.list(sortMode, query.trim()) : []),
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [engine, sortMode, query, version]
  );
  const stats = useMemo(() => (engine ? engine.stats() : null), [engine, version]);

  if (loadError) {
    return <div className="center-msg">Could not load the WebAssembly module: {loadError}</div>;
  }
  if (!engine) {
    return <div className="center-msg">Loading C++ engine…</div>;
  }

  const handleDelete = (s) => {
    if (!window.confirm(`Delete ${s.name} (${s.id})?`)) return;
    const r = engine.remove(s.id);
    notify(r.ok ? `Deleted ${s.name}` : r.message, r.ok);
    refresh();
  };

  return (
    <div className="app">
      <header className="header">
        <div className="header-inner">
          <div>
            <p className="eyebrow">CS-114 · Fundamentals of Programming · Semester 1</p>
            <h1>Student Grade Manager</h1>
            <p className="subtitle">
              Grading engine written in <strong>C++</strong>, compiled to <strong>WebAssembly</strong>, with a{' '}
              <strong>React</strong> interface.
            </p>
          </div>
          <nav className="tabs" aria-label="Sections">
            {[
              ['students', 'Students'],
              ['report', 'Class Report'],
              ['about', 'How it works'],
            ].map(([key, label]) => (
              <button
                key={key}
                className={tab === key ? 'tab active' : 'tab'}
                onClick={() => setTab(key)}
                aria-current={tab === key ? 'page' : undefined}
              >
                {label}
              </button>
            ))}
          </nav>
        </div>
      </header>

      <main className="main">
        <div className="layout">
          <section className="content">
            {tab === 'students' && (
              <StudentsView
                engine={engine}
                students={students}
                query={query}
                setQuery={setQuery}
                sortMode={sortMode}
                setSortMode={setSortMode}
                onAdd={() => setForm({ mode: 'add' })}
                onEdit={(s) => setForm({ mode: 'edit', student: s })}
                onDelete={handleDelete}
                onOpen={(s) => setReportId(s.id)}
                refresh={refresh}
                notify={notify}
              />
            )}
            {tab === 'report' && <ClassReport stats={stats} subjects={engine.subjects} onOpen={setReportId} />}
            {tab === 'about' && <About />}
          </section>
          <CallLog calls={calls} />
        </div>
      </main>

      <footer className="footer">
        Part of a rebuilt portfolio of coursework, BE Computer Engineering, NUST CEME (2020–2024) ·{' '}
        <a href={REPO_URL} target="_blank" rel="noreferrer">
          Source on GitHub
        </a>
      </footer>

      {form && (
        <StudentForm
          engine={engine}
          mode={form.mode}
          student={form.student}
          onClose={() => setForm(null)}
          onSaved={(msg) => {
            setForm(null);
            notify(msg);
            refresh();
          }}
        />
      )}
      {reportId !== null && (
        <ReportCard student={engine.get(reportId)} subjects={engine.subjects} onClose={() => setReportId(null)} />
      )}
      {toast && (
        <div key={toast.id} className={toast.ok ? 'toast' : 'toast toast-bad'} role="status">
          {toast.message}
        </div>
      )}
    </div>
  );
}

function StudentsView({
  engine, students, query, setQuery, sortMode, setSortMode, onAdd, onEdit, onDelete, onOpen, refresh, notify,
}) {
  const fileRef = useRef(null);

  const handleExport = () => {
    const text = engine.exportText();
    const blob = new Blob([text], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'students.txt';
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleImport = async (e) => {
    const file = e.target.files?.[0];
    e.target.value = '';
    if (!file) return;
    const r = engine.importText(await file.text());
    notify(r.ok ? `Loaded ${engine.count()} students from ${file.name}` : r.message, r.ok);
    refresh();
  };

  return (
    <>
      <div className="toolbar">
        <input
          className="search"
          type="search"
          placeholder="Search by name or ID…"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          aria-label="Search students"
        />
        <label className="sort">
          <span>Sort</span>
          <select value={sortMode} onChange={(e) => setSortMode(Number(e.target.value))}>
            {SORT_OPTIONS.map((o) => (
              <option key={o.value} value={o.value}>
                {o.label}
              </option>
            ))}
          </select>
        </label>
        <button className="btn btn-primary" onClick={onAdd}>
          + Add student
        </button>
      </div>

      <div className="file-actions">
        <button className="btn-link" onClick={handleExport}>Export students.txt</button>
        <button className="btn-link" onClick={() => fileRef.current?.click()}>Import students.txt</button>
        <button
          className="btn-link"
          onClick={() => {
            engine.loadSample();
            notify('Loaded sample class');
            refresh();
          }}
        >
          Reset to sample data
        </button>
        <button
          className="btn-link danger"
          onClick={() => {
            if (!window.confirm('Remove all students?')) return;
            engine.clear();
            notify('All records cleared');
            refresh();
          }}
        >
          Clear all
        </button>
        <input ref={fileRef} type="file" accept=".txt,text/plain" hidden onChange={handleImport} />
        <span className="hint">Same file format as the console version, so files work in both.</span>
      </div>

      {students.length === 0 ? (
        <div className="empty">
          {query ? `No students match “${query}”.` : 'No students yet. Add one or reset to the sample data.'}
        </div>
      ) : (
        <div className="table-wrap">
          <table className="table">
            <thead>
              <tr>
                <th>ID</th>
                <th>Name</th>
                {engine.subjects.map((s) => (
                  <th key={s.code} className="num hide-sm" title={`${s.title} (${s.credits} Cr)`}>
                    {s.code}
                  </th>
                ))}
                <th className="num">GPA</th>
                <th className="hide-xs">Standing</th>
                <th aria-label="Actions" />
              </tr>
            </thead>
            <tbody>
              {students.map((s) => (
                <tr key={s.id}>
                  <td className="mono">{s.id}</td>
                  <td>
                    <button className="name-link" onClick={() => onOpen(s)}>
                      {s.name}
                    </button>
                  </td>
                  {s.marks.map((m, i) => (
                    <td key={i} className="num hide-sm">
                      <span className="mark">{fmt(m, 0)}</span>
                      <span className={gradeClass(s.grades[i])}>{s.grades[i]}</span>
                    </td>
                  ))}
                  <td className="num mono strong">{fmt(s.gpa)}</td>
                  <td className="hide-xs">
                    <span className={standingClass(s.standing)}>{s.standing}</span>
                  </td>
                  <td className="actions">
                    <button className="icon-btn" onClick={() => onEdit(s)} aria-label={`Edit ${s.name}`} title="Edit">
                      ✎
                    </button>
                    <button
                      className="icon-btn danger"
                      onClick={() => onDelete(s)}
                      aria-label={`Delete ${s.name}`}
                      title="Delete"
                    >
                      ✕
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
      <p className="count-note">
        {students.length} of {engine.count()} students shown · max 100 (fixed-size array)
      </p>
    </>
  );
}

function StudentForm({ engine, mode, student, onClose, onSaved }) {
  const [id, setId] = useState(student ? String(student.id) : '');
  const [name, setName] = useState(student ? student.name : '');
  const [marks, setMarks] = useState(student ? student.marks.map(String) : engine.subjects.map(() => ''));
  const [error, setError] = useState('');

  const submit = (e) => {
    e.preventDefault();
    const idNum = Number(id);
    if (!Number.isInteger(idNum)) {
      setError('ID must be a whole number');
      return;
    }
    // Empty fields become NaN, which the C++ validation rejects.
    const nums = marks.map((m) => (m.trim() === '' ? NaN : Number(m)));
    const r = mode === 'add' ? engine.add(idNum, name, nums) : engine.update(idNum, name, nums);
    if (r.ok) onSaved(mode === 'add' ? `Added ${name.trim()}` : `Updated ${name.trim()}`);
    else setError(r.message);
  };

  return (
    <Modal title={mode === 'add' ? 'Add student' : `Edit ${student.name}`} onClose={onClose}>
      <form onSubmit={submit} className="form" noValidate>
        <div className="form-row">
          <label>
            <span>Student ID</span>
            <input
              inputMode="numeric"
              value={id}
              onChange={(e) => setId(e.target.value)}
              disabled={mode === 'edit'}
              autoFocus={mode === 'add'}
              placeholder="e.g. 1009"
            />
          </label>
          <label className="grow">
            <span>Full name</span>
            <input
              value={name}
              onChange={(e) => setName(e.target.value)}
              maxLength={40}
              autoFocus={mode === 'edit'}
              placeholder="e.g. Sara Iqbal"
            />
          </label>
        </div>
        <fieldset className="marks">
          <legend>Marks (0–100)</legend>
          {engine.subjects.map((s, i) => {
            const v = marks[i];
            const n = Number(v);
            const valid = v.trim() !== '' && n >= 0 && n <= 100;
            return (
              <label key={s.code} className="mark-row">
                <span className="mark-label">
                  <span className="mono">{s.code}</span> {s.title} <em>{s.credits} Cr</em>
                </span>
                <input
                  inputMode="decimal"
                  value={v}
                  onChange={(e) => setMarks((prev) => prev.map((p, j) => (j === i ? e.target.value : p)))}
                  aria-label={`${s.title} marks`}
                />
                <span className={valid ? gradeClass(engine.letterGrade(n)) : 'grade grade-empty'}>
                  {valid ? engine.letterGrade(n) : '–'}
                </span>
              </label>
            );
          })}
        </fieldset>
        {error && (
          <p className="form-error" role="alert">
            C++ says: {error}
          </p>
        )}
        <div className="form-actions">
          <button type="button" className="btn" onClick={onClose}>
            Cancel
          </button>
          <button type="submit" className="btn btn-primary">
            {mode === 'add' ? 'Add student' : 'Save changes'}
          </button>
        </div>
      </form>
    </Modal>
  );
}

function ReportCard({ student, subjects, onClose }) {
  if (!student) return null;
  const totalCredits = subjects.reduce((sum, s) => sum + s.credits, 0);
  return (
    <Modal title="Report Card" onClose={onClose} wide>
      <div className="report">
        <div className="report-head">
          <div>
            <div className="report-name">{student.name}</div>
            <div className="muted">ID {student.id} · Semester 1 · BE Computer Engineering</div>
          </div>
          <span className={standingClass(student.standing)}>{student.standing}</span>
        </div>
        <div className="table-wrap">
          <table className="table compact">
            <thead>
              <tr>
                <th>Code</th>
                <th>Course</th>
                <th className="num">Cr</th>
                <th className="num">Marks</th>
                <th className="num">Grade</th>
                <th className="num">GP × Cr</th>
              </tr>
            </thead>
            <tbody>
              {subjects.map((s, i) => (
                <tr key={s.code}>
                  <td className="mono">{s.code}</td>
                  <td>{s.title}</td>
                  <td className="num">{s.credits}</td>
                  <td className="num">{fmt(student.marks[i], 1)}</td>
                  <td className="num">
                    <span className={gradeClass(student.grades[i])}>{student.grades[i]}</span>
                  </td>
                  <td className="num mono">{fmt(student.points[i] * s.credits, 1)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <div className="report-summary">
          <div>
            <span className="muted">Average</span>
            <strong>{fmt(student.average, 1)}%</strong>
          </div>
          <div>
            <span className="muted">Credit hours</span>
            <strong>{totalCredits}</strong>
          </div>
          <div>
            <span className="muted">GPA</span>
            <strong className="gpa-big">{fmt(student.gpa)}</strong>
          </div>
          <div>
            <span className="muted">Failed courses</span>
            <strong>{student.failed}</strong>
          </div>
        </div>
        <p className="muted small">GPA = Σ(grade point × credit hours) ÷ total credit hours, computed in C++.</p>
      </div>
    </Modal>
  );
}

function ClassReport({ stats, subjects, onOpen }) {
  if (!stats || stats.count === 0) {
    return <div className="empty">No students yet, so there's nothing to report.</div>;
  }
  const maxGrade = Math.max(1, ...stats.gradeCount);
  return (
    <div className="report-grid">
      <div className="stat-row">
        <Stat label="Students" value={stats.count} />
        <Stat label="Average GPA" value={fmt(stats.averageGpa)} />
        <Stat label="Highest GPA" value={fmt(stats.highestGpa)} sub={stats.topper?.name} onClick={() => onOpen(stats.topper.id)} />
        <Stat label="Good standing" value={`${stats.goodStanding}/${stats.count}`} sub={`${Math.round((stats.goodStanding / stats.count) * 100)}% of class`} />
      </div>

      <div className="panel">
        <h2>Subject averages</h2>
        <div className="bars">
          {subjects.map((s, i) => (
            <div className="bar-row" key={s.code}>
              <span className="bar-label" title={s.title}>
                <span className="mono">{s.code}</span>
                <span className="bar-sub">{s.title}</span>
              </span>
              <div className="bar-track">
                <div className="bar-fill" style={{ width: `${stats.subjectAverage[i]}%` }} />
              </div>
              <span className="bar-value mono">{fmt(stats.subjectAverage[i], 1)}</span>
            </div>
          ))}
        </div>
      </div>

      <div className="panel">
        <h2>Grade distribution</h2>
        <p className="muted small">Across every subject result in the class.</p>
        <div className="columns" role="img" aria-label="Grade distribution chart">
          {stats.gradeLetters.map((g, i) => (
            <div className="col" key={g}>
              <span className="col-value">{stats.gradeCount[i]}</span>
              <div className="col-track">
                <div
                  className={g === 'F' ? 'col-fill col-fail' : 'col-fill'}
                  style={{ height: `${(stats.gradeCount[i] / maxGrade) * 100}%` }}
                />
              </div>
              <span className="col-label">{g}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

function Stat({ label, value, sub, onClick }) {
  const Tag = onClick ? 'button' : 'div';
  return (
    <Tag className={onClick ? 'stat stat-click' : 'stat'} onClick={onClick}>
      <span className="stat-label">{label}</span>
      <span className="stat-value">{value}</span>
      {sub && <span className="stat-sub">{sub}</span>}
    </Tag>
  );
}

function About() {
  return (
    <div className="about">
      <div className="panel">
        <h2>What this is</h2>
        <p>
          A grade management system written for <strong>CS-114 Fundamentals of Programming</strong>. It stores students
          in a fixed-size array of <code>struct</code>s, computes letter grades and credit-weighted GPA, searches by name,
          sorts records and saves them to a text file.
        </p>
        <p>
          The program uses only FoP concepts: structs, arrays, functions, loops, linear search, bubble sort and file I/O.
          There are no classes and no STL containers.
        </p>
      </div>

      <div className="panel">
        <h2>Architecture</h2>
        <div className="arch">
          <div className="arch-box">
            <strong>grade_core.cpp</strong>
            <span>All logic: validation, grading, GPA, search, bubble sort, file format</span>
          </div>
          <div className="arch-split">
            <div className="arch-path">
              <span className="arch-arrow">g++</span>
              <div className="arch-box">
                <strong>main.cpp</strong>
                <span>Console menu program, saves to students.txt</span>
              </div>
            </div>
            <div className="arch-path">
              <span className="arch-arrow">em++ → .wasm</span>
              <div className="arch-box">
                <strong>wasm_api.cpp</strong>
                <span>Exports functions to JavaScript; React renders the results</span>
              </div>
            </div>
          </div>
        </div>
        <p className="muted small">
          The same C++ source runs in both versions. The website is static and needs no server, because the C++
          runs inside your browser.
        </p>
      </div>

      <div className="panel">
        <h2>Grading scale</h2>
        <div className="table-wrap">
          <table className="table compact">
            <thead>
              <tr>
                {['A', 'B+', 'B', 'C+', 'C', 'D+', 'D', 'F'].map((g) => (
                  <th key={g} className="num">{g}</th>
                ))}
              </tr>
            </thead>
            <tbody>
              <tr>
                {['≥85', '≥80', '≥75', '≥70', '≥65', '≥60', '≥55', '<55'].map((m) => (
                  <td key={m} className="num mono">{m}</td>
                ))}
              </tr>
              <tr>
                {['4.0', '3.5', '3.0', '2.5', '2.0', '1.5', '1.0', '0.0'].map((p, i) => (
                  <td key={i} className="num mono">{p}</td>
                ))}
              </tr>
            </tbody>
          </table>
        </div>
        <p className="muted small">
          Standing: any F → Repeat Required; GPA &lt; 2.0 → Probation; GPA ≥ 3.5 → Dean&apos;s Honor List.
        </p>
      </div>

      <div className="panel">
        <h2>Bubble sort from grade_core.cpp</h2>
        <pre className="code">{`void sortStudents(int mode) {
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
}`}</pre>
      </div>
    </div>
  );
}

function CallLog({ calls }) {
  return (
    <aside className="calllog" aria-label="C++ call log">
      <div className="calllog-head">
        <span className="dot" /> C++ call log
      </div>
      <ol className="calllog-body">
        {calls.length === 0 && <li className="muted">No calls yet.</li>}
        {calls.map((c, i) => (
          <li key={c.at + '-' + i}>
            <span className="fn">{c.fn}</span>
            <span className="args">({c.args.map((a) => (typeof a === 'string' && !a.startsWith('<') ? `"${a}"` : String(a))).join(', ')})</span>
            <span className={c.ok ? 'res ok' : 'res bad'}>→ {c.result}</span>
          </li>
        ))}
      </ol>
    </aside>
  );
}

function Modal({ title, children, onClose, wide }) {
  useEffect(() => {
    const onKey = (e) => e.key === 'Escape' && onClose();
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onClose]);

  return (
    <div className="modal-backdrop" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div className={wide ? 'modal modal-wide' : 'modal'} role="dialog" aria-modal="true" aria-label={title}>
        <div className="modal-head">
          <h2>{title}</h2>
          <button className="icon-btn" onClick={onClose} aria-label="Close">
            ✕
          </button>
        </div>
        {children}
      </div>
    </div>
  );
}
