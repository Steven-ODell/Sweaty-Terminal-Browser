#include "path_handle.h"
#include "term_set.h"
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

void loadEntriesFrPath(Paths &paths, Placement &Pos) {

  if (fs::is_directory(paths.full_path)) {

    paths.entries.clear();
    Global.new_name = "";
    Pos.cur_row = 0;
    Pos.window_offset = 0;

    for (const auto &entry : fs::directory_iterator(paths.full_path)) {

      paths.entries.push_back(entry);
    }

    paths.full_path.assign(paths.full_path);

    if (Global.hidden) {

      Global.hidden_count = 0;

      for (int i = paths.entries.size() - 1; i >= 0; i--) {

        if (paths.entries[i].path().filename().string()[0] == '.') {

          paths.entries.erase(paths.entries.begin() + i);
          Global.hidden_count++;
        }
      }
    }

    if (paths.entries.empty()) {

      if (paths.full_path != paths.base_dir) {

        std::string error_mes = "» Folder is empty or only contains hidden"
                                "\x1b[";
        error_mes +=
            std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
        write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

        sleep(1);
      }
    }
  } else if (!fs::is_directory(paths.full_path)) {

    checkIfFile(paths.full_path, paths, Pos);
  }
}

void loadPreviousPath(Paths &paths, Placement &Pos) {

  if (fs::exists(paths.full_path.parent_path())) {

    if (paths.full_path != paths.base_dir) {

      paths.full_path.assign(paths.full_path.parent_path());
      loadEntriesFrPath(paths, Pos);

    } else {

      std::string error_mes =
          "» Cant go further back than the home directory \x1b[";
      error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

      sleep(1);
    }
  }
}

void checkIfFile(const fs::path &path_to_check, Paths &paths, Placement &Pos) {

  if (fs::is_regular_file(path_to_check)) {

    // Check if image or binary or able to be opened in nvim
    std::string EXT = path_to_check.extension();

    if (EXT == ".png" || EXT == ".jpg" || EXT == ".jpeg" || EXT == ".gif" ||
        EXT == ".webp" || EXT == ".bmp") {

      // Open with image viewer
      Global.hidden_holder = Global.hidden;
      paths.full_path = path_to_check.parent_path();

      openInViewer(path_to_check);
      Global.hidden = Global.hidden_holder;
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

      std::string error_mes =
          "» Error this file type can not be opened with an editor \x1b[";
      error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

      sleep(1);

      paths.full_path = path_to_check.parent_path();
      loadEntriesFrPath(paths, Pos);

    } else {

      // Open Nvim to file path
      Global.hidden_holder = Global.hidden;
      paths.full_path = path_to_check.parent_path();
      openInEditor(path_to_check, paths, Pos);
      loadEntriesFrPath(paths, Pos);
    }
  } else {

    std::string error_mes =
        "» Error this file type can not be opened with an editor \x1b[";
    error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
    write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

    sleep(1);
  }
}

// Open Nvim to file path
void openInEditor(const fs::path &file, Paths &paths, Placement &Pos) {

  // Restore termios + leave alt screen
  disableRawMode();

  pid_t pid = fork();

  if (pid == -1)
    die("fork");

  if (pid == 0) {
    execlp("nvim", "nvim", file.c_str(), nullptr);
    // Only reached if exec failed
    _exit(127);
  }

  // Block until nvim quits
  waitpid(pid, nullptr, 0);

  // Back to alt screen + raw
  enableRawMode();

  getWinSize(&Pos.screen_rows, &Pos.screen_cols); // They may have resized
  Global.hidden = Global.hidden_holder;
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
  // No waitpid — imv is a Wayland window, TUI keeps running
}

void openCurrentPath(const fs::path &cur_path, Paths &paths, Placement &Pos) {

  paths.full_path = cur_path;
  loadEntriesFrPath(paths, Pos);

  write(STDOUT_FILENO, "\x1b[H", 3);
}

void renamePath(Paths &paths, Placement &Pos) {

  if (Global.new_name == "") {

    std::string error_mes = "Error: Field was empty \x1b[";
    error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
    write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

    sleep(1);

  } else {

    try {

      fs::rename(paths.entries[Pos.cur_row].path(),
                 paths.entries[Pos.cur_row].path().parent_path() /
                     Global.new_name);
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.new_name = "";

    } catch (const fs::filesystem_error &e) {

      std::string err_what = e.what();
      std::string error_mes = "» Error: " + err_what + "\x1b[";
      error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

      sleep(2);
    }
  }
}

void deletePath(Paths &paths, Placement &Pos) {
  try {

    uintmax_t total_removed = fs::remove_all(paths.entries[Pos.cur_row]);

    if (total_removed == 1) {

      std::string del_mes = " Folder deleted \x1b[";
      del_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, del_mes.c_str(), del_mes.size());

      sleep(1);

    } else {

      std::string del_mes = " Folder/files deleted \x1b[";
      del_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, del_mes.c_str(), del_mes.size());

      sleep(1);
    }

  } catch (const fs::filesystem_error &e) {

    std::string err_what = e.what();
    std::string error_mes = " Error: " + err_what + "\x1b[";
    error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
    write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

    sleep(2);
  }

  Global.state = State::Browser;
  loadEntriesFrPath(paths, Pos);
  Global.del_choice = "";
}

void addNewPath(Paths &paths, Placement &Pos) {
  if (Global.brand_new_name == "") {

    std::string error_mes = "Error: Field was empty \x1b[";
    error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
    write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

    sleep(1);

  } else {

    try {

      std::string new_path =
          paths.full_path.string() + "/" + Global.brand_new_name;

      fs::create_directories(new_path);
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.brand_new_name = "";

    } catch (const fs::filesystem_error &e) {

      std::string err_what = e.what();

      std::string error_mes = "Error: " + err_what + "\x1b[";
      error_mes += std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
      write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

      sleep(2);
    }
  }
}
