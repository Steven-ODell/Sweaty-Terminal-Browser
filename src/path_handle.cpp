#include "path_handle.h"
#include "term_set.h"
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void loadEntriesFrPath(fs::path new_path) {
  if (fs::is_directory(new_path)) {
    E.entries.clear();
    E.new_name = "";
    E.cur_row = 1;
    E.window_offset = 0;
    for (const auto &entry : fs::directory_iterator(new_path)) {
      E.entries.push_back(entry);
    }
    E.full_path.assign(new_path);
    if (E.hidden) {
      E.hidden_count = 0;
      for (int i = E.entries.size() - 1; i >= 0; i--) {
        if (E.entries[i].path().filename().string()[0] == '.') {
          E.entries.erase(E.entries.begin() + i);
          E.hidden_count++;
        }
      }
    }
    if (E.entries.size() == 0) {
      std::cout
          << "Folder is empty or only contains hidden - Loading parent path"
          << std::endl;
      sleep(2);
      E.full_path = E.full_path.parent_path();
      loadEntriesFrPath(E.full_path);
    }
  } else {
    checkIfFile(new_path);
  }
}

void loadPreviousPath(fs::path cur_path) {
  if (fs::exists(cur_path.parent_path())) {
    if (cur_path != E.base_dir) {
      E.full_path.assign(cur_path.parent_path());
      loadEntriesFrPath(E.full_path);
      write(STDOUT_FILENO, "\x1b[1H", 4);
    } else {
      std::cout << "Cant go further back than the home directory" << std::endl;
      sleep(1);
    }
  }
}

void checkIfFile(fs::path path_to_check) {
  if (fs::is_regular_file(path_to_check)) {
    // Check if image or binary or able to be opened in nvim
    std::string EXT = path_to_check.extension();
    if (EXT == ".png" || EXT == ".jpg" || EXT == ".jpeg" || EXT == ".gif" ||
        EXT == ".webp" || EXT == ".bmp") {
      // Open with image viewer
      E.hidden_holder = E.hidden;
      fs::path previous_path = path_to_check.parent_path();
      openInViewer(path_to_check);
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(previous_path);
      refreshScreen();
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

      std::string seq = "\x1b[" + std::to_string(E.cur_row + 1) + ";1H";
      write(STDOUT_FILENO, seq.c_str(), seq.size());

      sleep(1);
    } else {
      // Open Nvim to file path
      E.hidden_holder = E.hidden;
      fs::path previous_path = path_to_check.parent_path();
      openInEditor(path_to_check);
      loadEntriesFrPath(previous_path);
    }
  } else {
    std::cout << "Error this file type can not be opened with an editor"
              << std::endl;

    std::string seq = "\x1b[" + std::to_string(E.cur_row + 1) + ";1H";
    write(STDOUT_FILENO, seq.c_str(), seq.size());

    sleep(1);
  }
}

// Open Nvim to file path
void openInEditor(const fs::path &file) {
  disableRawMode(); // Restore termios + leave alt screen

  pid_t pid = fork();
  if (pid == -1)
    die("fork");
  if (pid == 0) {
    execlp("nvim", "nvim", file.c_str(), nullptr);
    _exit(127); // Only reached if exec failed
  }
  waitpid(pid, nullptr, 0); // Block until nvim quits

  enableRawMode();                            // Back to alt screen + raw
  getWinSize(&E.screen_rows, &E.screen_cols); // They may have resized
  E.hidden = E.hidden_holder;
  refreshScreen();
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

void openCurrentPath(fs::path path) {
  fs::path previous_path = path.parent_path();
  loadEntriesFrPath(path);
  if (E.entries.size() > 0) {
    write(STDOUT_FILENO, "\x1b[H", 3);
  } else {
    std::cout << "This folder is empty" << std::endl;
    write(STDOUT_FILENO, "\x1b[H", 3);
    sleep(1);
    loadEntriesFrPath(previous_path);
  }
}

void renamePath() {
  if (E.new_name == "") {
    std::cout << "Error: Field was empty" << std::endl;
    sleep(1);
  } else {
    try {
      fs::rename(E.entries[E.cur_row - 1].path(),
                 E.entries[E.cur_row - 1].path().parent_path() / E.new_name);
      loadEntriesFrPath(E.full_path);
      E.state = Config::State::Browser;
      E.new_name = "";
    } catch (const fs::filesystem_error &e) {
      std::cout << "Error: " << e.what() << std::endl;
    }
  }
}

void deletePath(fs::path incoming_path) {
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
  E.state = Config::State::Browser;
  loadEntriesFrPath(E.full_path);
  E.del_choice = "";
}

void addNewPath(fs::path incoming_path) {
  if (E.brand_new_name == "") {
    std::cout << "Error: Field was empty" << std::endl;
    sleep(1);
  } else {
    try {
      std::string new_path = E.full_path.string() + "/" + E.brand_new_name;
      fs::create_directories(new_path);
      loadEntriesFrPath(E.full_path);
      E.state = Config::State::Browser;
      E.brand_new_name = "";
    } catch (const fs::filesystem_error &e) {
      std::cout << "Error: " << e.what() << std::endl;
    }
  }
}
