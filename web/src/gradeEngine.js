// Wraps the C++ functions compiled to WebAssembly (see ../../cpp/wasm_api.cpp).
// Every call is reported to `onCall` so the UI can show the C++ call log.
import createGradeModule from './wasm/grade_manager.js';

const SIGNATURES = {
  gm_subjects: ['string', []],
  gm_add: ['number', ['number', 'string', 'number', 'number', 'number', 'number', 'number']],
  gm_update: ['number', ['number', 'string', 'number', 'number', 'number', 'number', 'number']],
  gm_remove: ['number', ['number']],
  gm_clear: [null, []],
  gm_load_sample: [null, []],
  gm_count: ['number', []],
  gm_status_message: ['string', ['number']],
  gm_letter_grade: ['string', ['number']],
  gm_list: ['string', ['number', 'string']],
  gm_get: ['string', ['number']],
  gm_stats: ['string', []],
  gm_serialize: ['string', []],
  gm_load: ['number', ['string']],
};

// Friendly names matching the C++ core functions each wrapper calls.
const CPP_NAMES = {
  gm_add: 'addStudent',
  gm_update: 'updateStudent',
  gm_remove: 'removeStudent',
  gm_clear: 'clearAll',
  gm_load_sample: 'loadSampleData',
  gm_list: 'sortStudents + nameMatches',
  gm_stats: 'computeStats',
  gm_serialize: 'serialize',
  gm_load: 'deserialize',
};

const STORAGE_KEY = 'sgm.students.v1';

export async function createEngine(onCall = () => {}) {
  const Module = await createGradeModule();
  const fn = {};
  for (const [name, [ret, args]] of Object.entries(SIGNATURES)) {
    fn[name] = Module.cwrap(name, ret, args);
  }

  const status = (code) => ({ ok: code === 0, code, message: fn.gm_status_message(code) });

  const log = (name, args, result, ok = true) => {
    if (CPP_NAMES[name]) onCall({ fn: CPP_NAMES[name], args, result, ok, at: Date.now() });
  };

  const persist = () => {
    try {
      localStorage.setItem(STORAGE_KEY, fn.gm_serialize());
    } catch {
      /* storage unavailable (private mode) - the app still works in memory */
    }
  };

  const mutate = (name, args, logArgs = args) => {
    const code = fn[name](...args);
    const s = status(code);
    log(name, logArgs, s.message, s.ok);
    if (s.ok) persist();
    return s;
  };

  const engine = {
    subjects: JSON.parse(fn.gm_subjects()),
    letterGrade: (mark) => fn.gm_letter_grade(mark),

    add: (id, name, marks) => mutate('gm_add', [id, name, ...marks]),
    update: (id, name, marks) => mutate('gm_update', [id, name, ...marks]),
    remove: (id) => mutate('gm_remove', [id]),

    list: (sortMode, query) => JSON.parse(fn.gm_list(sortMode, query)),
    get: (id) => JSON.parse(fn.gm_get(id)),
    stats: () => JSON.parse(fn.gm_stats()),
    count: () => fn.gm_count(),

    exportText() {
      const text = fn.gm_serialize();
      log('gm_serialize', [], `${engine.count()} records`);
      return text;
    },
    importText(text) {
      const lines = text.split('\n').filter((l) => l.trim()).length;
      return mutate('gm_load', [text], [`<file: ${lines} lines>`]);
    },
    loadSample() {
      fn.gm_load_sample();
      log('gm_load_sample', [], `${engine.count()} students`);
      persist();
    },
    clear() {
      fn.gm_clear();
      log('gm_clear', [], 'Success');
      persist();
    },
  };

  // Restore saved data, or start with the sample class.
  let saved = null;
  try {
    saved = localStorage.getItem(STORAGE_KEY);
  } catch {
    saved = null;
  }
  if (saved !== null && fn.gm_load(saved) === 0) {
    onCall({ fn: 'deserialize', args: ['<localStorage>'], result: `${fn.gm_count()} records`, ok: true, at: Date.now() });
  } else {
    engine.loadSample();
  }

  return engine;
}
