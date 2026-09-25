#include "path_handle.h"
#include "term_set.h"
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void loadEntriesFrPath(Paths &paths, Placement &Pos) {
  if (fs::is_directory(paths.full_path)) {
    paths.entries.clear();
    E.new_name = "";
    Pos.cur_row = 1;
    Pos.window_offset = 0;
    for (const auto &entry : fs::directory_iterator(paths.full_path)) {
      paths.entries.push_back(entry);
    }
    paths.full_path.assign(paths.full_path);
    if (E.hidden) {
      E.hidden_count = 0;
      for (int i = paths.entries.size() - 1; i >= 0; i--) {
        if (paths.entries[i].path().filename().string()[0] == '.') {
          paths.entries.erase(paths.entries.begin() + i);
          E.hidden_count++;
        }
      }
    }
    if (paths.entries.size() == 0) {
      std::cout
          << "Folder is empty or only contains hidden - Loading parent path"
          << std::endl;
      sleep(2);
      paths.full_path = paths.full_path.parent_path();
      loadEntriesFrPath(paths, Pos);
    }
  } else {
    checkIfFile(paths.full_path, paths, Pos);
  }
}

void loadPreviousPath(fs::path cur_path, Paths &paths, Placement &Pos) {
  if (fs::exists(cur_path.parent_path())) {
    if (cur_path != paths.base_dir) {
      paths.full_path.assign(cur_path.parent_path());
      loadEntriesFrPath(paths, Pos);
      write(STDOUT_FILENO, "\x1b[1H", 4);
    } else {
      std::cout << "Cant go further back than the home directory" << std::endl;
      sleep(1);
    }
  }
}

void checkIfFile(fs::path path_to_check, Paths &paths, Placement &Pos) {
  if (fs::is_regular_file(path_to_check)) {
    // Check if image or binary or able to be opened in nvim
    std::string EXT = path_to_check.extension();
    if (EXT == ".png" || EXT == ".jpg" || EXT == ".jpeg" || EXT == ".gif" ||
        EXT == ".webp" || EXT == ".bmp") {
      // Open with image viewer
      E.hidden_holder = E.hidden;
      paths.full_path = path_to_check.parent_path();

      openInViewer(path_to_check);
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      refreshScreen(paths, Pos);
    } else if (EXT == ".o" || EXT == ".a" || EXT == ".so" || EXT == ".ko" ||
               EXT == ".elf" || EXT == ".bin" || EXT == ".exe" ||
               EXT == ".dll" || EXT == ".dylib" || EXT == ".pyc" ||
               EXT == ".pyo" || EXT == ".class" || EXT == ".jar" ||
               EXT == ".wasm" || EXT == ".zip" || EXT == ".tar" ||
               EXT == ".gz" || EXT == ".bz2" || EXT == ".xz" || EXT == ".zst" ||
               EXT == ".7z" || EXT == ".rar" || EXT == ".iso" ||
               EXT == ".deb" || EXT == ".rpm" || EXT == ".mp3" ||
               EXT == ".wav" || EXT == ".flac" || EXT == ".ogg" ||
               EXT == ".opus" || EXT == ".m4a" || EXT == ".mp4" ||
               EXT == ".mkv" || EXT == ".avi" || EXT == ".mov" ||
               EXT == ".webm" || EXT == ".ttf" || EXT == ".otf" ||
               EXT == ".ttc" || EXT == ".woff" || EXT == ".woff2" ||
               EXT == ".pdf" || EXT == ".doc" || EXT == ".docx" ||
               EXT == ".xls" || EXT == ".xlsx" || EXT == ".ppt" ||
               EXT == ".pptx" || EXT == ".odt" || EXT == ".db" ||
               EXT == ".sqlite" || EXT == ".sqlite3" || EXT == ".dat" ||
               EXT == ".pack" || EXT == ".idx" || EXT == ".ch8" ||
               EXT == ".nes" || EXT == ".gb" || EXT == ".gbc" ||
               EXT == ".gba" || EXT == ".smc" || EXT == ".sfc" ||
               EXT == ".z64" || EXT == ".n64" || EXT == ".rom" ||
               EXT == ".blend" || EXT == ".stl" || EXT == ".3mf" ||
               EXT == ".fbx" || EXT == ".glb" || EXT == ".dwg") {
      std::cout << "Error this file type can not be opened with an editor"
                << std::endl;

      std::string seq = "\x1b[" + std::to_string(Pos.cur_row + 1) + ";1H";
      write(STDOUT_FILENO, seq.c_str(), seq.size());

      sleep(1);
    } else {
      // Open Nvim to file path
      E.hidden_holder = E.hidden;
      paths.full_path = path_to_check.parent_path();
      openInEditor(path_to_check, paths, Pos);
      loadEntriesFrPath(paths, Pos);
    }
  } else {
    std::cout << "Error this file type can not be opened with an editor"
              << std::endl;

    std::string seq = "\x1b[" + std::to_string(Pos.cur_row + 1) + ";1H";
    write(STDOUT_FILENO, seq.c_str(), seq.size());

    sleep(1);
  }
}

// Open Nvim to file path
void openInEditor(const fs::path &file, Paths &paths, Placement &Pos) {
  disableRawMode(); // Restore termios + leave alt screen

  pid_t pid = fork();
  if (pid == -1)
    die("fork");
  if (pid == 0) {
    execlp("nvim", "nvim", file.c_str(), nullptr);
    _exit(127); // Only reached if exec failed
  }
  waitpid(pid, nullptr, 0); // Block until nvim quits

  enableRawMode();                                // Back to alt screen + raw
  getWinSize(&Pos.screen_rows, &Pos.screen_cols); // They may have resized
  E.hidden = E.hidden_holder;
  refreshScreen(paths, Pos);
}

void openInViewer(const fs::path &file) {
  pid_t pid = fork();
  if (pid == -1)
    die("fork");
  if (pid == 0) {
    int null = open("/dev/null", O_WRONLY);
    dup2(null, STDOUT_FILENO);
    dup2(null, STDERR_FILENO);
    execlp("imv", "imv", file.c_str(), nullptr);
    _exit(127);
  }
  // No waitpid — imv is a Wayland window, your TUI keeps running
}

void openCurrentPath(fs::path cur_path, Paths &paths, Placement &Pos) {
  paths.full_path = cur_path;
  loadEntriesFrPath(paths, Pos);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void renamePath(Paths &paths, Placement &Pos) {
  if (E.new_name == "") {
    std::cout << "Error: Field was empty" << std::endl;
    sleep(1);
  } else {
    try {
      fs::rename(paths.entries[Pos.cur_row - 1].path(),
                 paths.entries[Pos.cur_row - 1].path().parent_path() /
                     E.new_name);
      loadEntriesFrPath(paths, Pos);
      E.state = State::Browser;
      E.new_name = "";
    } catch (const fs::filesystem_error &e) {
      std::cout << "Error: " << e.what() << std::endl;
    }
  }
}

void deletePath(fs::path incoming_path, Paths &paths, Placement &Pos) {
  try {
    uintmax_t total_removed = fs::remove_all(incoming_path);
    if (total_removed == 1) {
      std::cout << "Folder deleted" << std::endl;
      sleep(1);
    } else {
      std::cout << total_removed << " Folders/files deleted" << std::endl;
      sleep(1);
    }
  } catch (const fs::filesystem_error &e) {
    std::cout << "Error: " << e.what() << std::endl;
    sleep(2);
  }
  E.state = State::Browser;
  loadEntriesFrPath(paths, Pos);
  E.del_choice = "";
}

void addNewPath(fs::path current_dir, Paths &paths, Placement &Pos) {
  if (E.brand_new_name == "") {
    std::cout << "Error: Field was empty" << std::endl;
    sleep(1);
  } else {
    try {
      std::string new_path = paths.full_path.string() + "/" + E.brand_new_name;
      fs::create_directories(new_path);
      loadEntriesFrPath(paths, Pos);
      E.state = State::Browser;
      E.brand_new_name = "";
    } catch (const fs::filesystem_error &e) {
      std::cout << "Error: " << e.what() << std::endl;
    }
  }
}
