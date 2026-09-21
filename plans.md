# Plans

## Near-term, in order

1. Parent path floor. `loadEntriesFrPath` recovers from an empty directory by
   climbing to `parent_path()` with no stop. Add the `E.base_dir` check so it
   refuses to go above `$HOME` and shows a different message instead. Same fix
   in `check_start_path` (`term_set.cpp:260`), which has the same climb.
2. Tests.
3. Rewrite the README by hand so it's mine, not AI.
4. Publish as ready for others to use on Linux/WSL.

## Tests

- `tests/` folder with its own `main()`, built as a second CMake executable.
- Each test calls a real function with known input and compares against the
  answer I already know is right. Returns nonzero when it disagrees.
- Start with `path_handle` functions like `parent_path()` handling. They take a
  path and return a path, no globals to set up.
- Then `searchCurBuffer`: given a fixed set of paths, assert `hits[0]` is the
  path I expect for a given query.
- To make the search testable, pass the paths into `searchCurBuffer` as a
  parameter instead of reading the global `E.all_paths`. Same change that makes
  the matcher liftable into a standalone tool later.
- When expected results change because the scoring changed on purpose, update
  the expected values deliberately, after checking every ranking that moved.

## Before publishing

- Null check on `getenv("HOME")`. Assigning `nullptr` to a `std::string` is
  undefined; flag it when `HOME` isn't set.
- Row budget: the header at row 1 took a row from the entry list and `drawRows`
  still loops `E.screen_rows` times. Only the hidden branch was compensated.
  One `rows_for_entries` value feeding the loop bound at `term_set.cpp:91`, the
  newline test at `:100-108`, and the scroll guards at `inputs.cpp:356` and
  `:370`.
- Truncation at `term_set.cpp:96` and `search.cpp:159` cuts to
  `E.screen_cols - 2` then appends three characters, so it overflows by one and
  wraps.
- Browser `?` at `inputs.cpp:110` doesn't save `E.hidden_holder` the way the
  Search one at `:176` does, so leaving the help screen restores a stale value.
- README note that `/mnt/c` is slow under WSL.

## Keeping `E.all_paths` fresh

Build it in the background at startup, then maintain it incrementally instead
of rebuilding.

- Delete: one pass over `E.all_paths` with `remove_if` on a prefix test,
  erasing everything under the deleted path. One pass, not one lookup per
  removed item. Covers `remove_all` taking a whole subtree.
- Add: `push_back` each level `create_directories` made.
- Rename: same single pass, rewriting the prefix instead of dropping the entry.

## `cur_row` and `E.cx`

- `E.cur_row` is the selection. `E.cx` is the cursor drawing index.
- Make `cur_row` the index directly so the `- 1` disappears from the 11 sites
  that use it.
- Zero-based, the clamps at `inputs.cpp:380`, `:394` and `search.cpp:97` become
  `E.cur_row + 1 < size()`, not `size() - 1`, which wraps on an empty vector.
- Prompt row positions currently set through `E.cx` (Rename, Delete, Add) get
  hardcoded in those draw functions, since they're draw-time facts.
- Browser and Search need to agree on whether Search has a header row. Right
  now they don't: `inputs.cpp:410` guards `E.cx > 2`, `search.cpp:112` guards
  `E.cx > 1`.

## Structure and size

- `state.h` / `state.cpp`. Dispatch to a function per state instead of the whole
  state switch living in `term_set`.
- Pass the write strings in as parameters and append the rest per state, which
  removes the large blocks in `term_set`.
- Collapse the remaining double writes into one write per grouping.
- Take `const fs::path &` and `const std::string &` instead of by value in
  `openCurrentPath`, `deletePath`, `loadEntriesFrPath`, `searchCurBuffer`.
- `const` where things should be const.
- Target: stay under 1000 lines.

## Search, on hold

Matching works well enough to use. Deferred, with the plan already written down:

- Additive scoring on top of the existing substring and subsequence tiers.
- Gap count, consecutive letter runs, match length, space count.
- Case: match case-insensitively but penalize a case mismatch rather than
  rejecting it, and keep tracking which it was.
- Fix the greedy forward scan, which anchors `seq_start` to the earliest
  occurrence of the query's first character, so the span it scores isn't the
  minimum window.
- `pair<uint32_t, uint32_t>` can't hold a signed additive score.

## Layout

- Use the extra horizontal space for the previous and next folder rather than a
  dedicated tree view.
- Preview stops being a state and becomes a toggleable flag. It never changes
  what a key means, so it isn't a mode.
- Separate full preview key that pulls a buffer of the text, or overlays it in
  an ANSI box over the current session. Scrollable, or just the top lines.

## cd on quit

`o` quits and leaves the shell in the selected directory. The program can't
change the parent shell's directory, so it writes the final path out and a zsh
function wrapping the binary reads it and runs `cd`. Same mechanism as lf's
`-last-dir-path`. Decide between stdout and a temp file; stdout means nothing
else can print to stdout, and it currently does.

## Contributing

- `CONTRIBUTING.md`, not the README.
- Written by hand. Contributors can consult AI the way I do, but it doesn't
  touch the project.
- Stated as a review standard: you understand every line you submit and can
  explain why it's written that way.

## After this project

- Contribute to someone else's codebase. Reading other people's code, merging,
  using GitHub as a collaboration tool instead of a personal cloud.
