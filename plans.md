# Plans

## Where its at

Everything since I last updated this. Parent path floor is in so it wont climb past $HOME anymore in both spots. Tests are set up as their own executable in CMake with test_search.cpp. cur_row is the index now and cx is only for drawing. One write per frame so the flicker is gone. Path args work relative or direct. Moved most of the globals into Paths and Placement and E is Global now. Row budget is one rows_for_entry value. The browser ? saves hidden_holder now same as search. Around 950 lines total.

## Small fixes before I call it done

- Null check on getenv("HOME")
- Errors go to std::cerr not cout. Still 11 couts left, 4 in term_set that print at startup and 7 in path_handle that print while raw mode is on. Has to happen before cd on quit since that uses stdout
- Arrow keys come in as esc
- resize handling (the event loop below fixes this)
- const refs instead of by value for fs::path and std::string in path_handle.h, handle_arg and searchCurBuffer

## Config file

There isnt one yet. base_dir goes in it so nobody is stuck with $HOME, hidden files on or off by default, and keybinds later on. Read it from ~/.config on startup and fall back to defaults if its not there. Once it exists the no $HOME error can actually tell people to set base_dir.

## Tests

- More tests for path_handle, they take a path and give a path so theres nothing to set up
- When scoring changes on purpose update the expected values after checking every ranking that moved, not just to make it pass

## The big stuff

These are the ones I actually learn from. If I already know how to build it its maintenance not progress. If I have to go read how ncurses or fzf does it first then its worth doing. Rough order since the first two make everything after them easier.

### 1. Screen diff renderer

Keep a grid of cells for whats on screen and a grid for what should be and only write the cells that changed. This is how ncurses and ratatui work under the hood. Preview, splits and the editor all need it. Might pull it out as its own library and use it in everything I build after.

### 2. Event loop

poll() on stdin and inotify and the resize signal at the same time instead of blocking on a key read. The all_paths walk goes on a background thread and gets cancelled when the query changes. Teaches threads and locking and how real programs wait on more than one thing. Resize handling comes with it and the list updates when files change on disk.

### 3. Proper search

Matching works well enough to use for now. When I come back to it:

- Additive scoring on top of the substring and subsequence tiers. Gap count, consecutive runs, match length, spaces
- Case insensitive but penalize a case mismatch instead of rejecting it
- The greedy scan anchors seq_start to the first match of the first char so the span it scores isnt the smallest window
- pair<uint32_t, uint32_t> cant hold a signed score
- fzf does this with dynamic programming, read how before writing mine

### 4. Preview with coloring

Preview stops being a state and becomes a flag, it never changes what a key means so its not a mode. Top of the file in a side pane and color it inline, which means tokenizing the text as its drawn. Colors add escape bytes so buf.size() stops being the width on screen and I need to track visible width separately. Separate key for full preview that opens the whole file scrollable or overlays it in a box. Extra horizontal space goes to the previous and next folder not a tree view.

### 5. Image rendering

Kitty graphics protocol, the file gets base64d and wrapped in escape codes. That part is mostly plumbing. Where it gets real is decoding the PNG myself which means writing inflate, and that could be its own project that plugs in here.

### 6. Diff tool

Myers algorithm for diffing text. Build it as its own library first then hook it into the browser.

### 7. Git over the network

Learn what actually happens when git talks to GitHub. Start with raw sockets and plain HTTP to something local so I see the requests by hand. HTTPS needs TLS which Im not writing myself so thats libcurl or OpenSSL. Other route is SSH with libssh2. End goal is repo status or pulling from inside the browser.

### 8. Editor

Turn preview into an editor with a tree explorer next to it. The real learning is the text buffer, gap buffer or piece table or rope, not the UI. Biggest one here and waits until 1 and 2 are solid. Could honestly be a 2 year thing.

## Keeping all_paths fresh

Build it in the background at startup then keep it updated instead of rebuilding.

- Delete: one remove_if pass with a prefix test, erases everything under the deleted path. Covers remove_all taking a whole subtree
- Add: push_back each level create_directories made
- Rename: same single pass but rewrite the prefix instead of dropping it

## cd on quit

o quits and leaves the shell in the selected folder. The program cant change the parent shells directory so it writes the path out and a zsh function wrapping the binary reads it and runs cd. Same thing lf does with -last-dir-path. Stdout or a temp file, stdout only works once all the couts are gone.

## Contributing

- CONTRIBUTING.md not the README
- Written by hand. People can consult AI the way I do but it doesnt touch the project
- The rule is you understand every line you submit and can explain why its written that way

## After this project

- Contribute to someone elses codebase. Reading other peoples code, merging, using GitHub to work with people instead of as a personal cloud
