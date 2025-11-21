#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            tokens.push_back(token.substr(start, end - start + 1));
        } else if (start != std::string::npos) {
            tokens.push_back("");
        }
    }
    return tokens;
}

std::vector<pid_t> getPidsByName(const std::string& name) {
    std::vector<pid_t> pids;
    std::string command = "pgrep -f \"" + name + "\"";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error executing pgrep command for '" << name << "'" << std::endl;
        return pids;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        try {
            pid_t pid = std::stoi(buffer);
            pids.push_back(pid);
        } catch (const std::invalid_argument& e) {
            continue;
        }
    }
    pclose(pipe);
    return pids;
}

bool killProcessById(pid_t pid) {
    if (kill(pid, SIGTERM) == -1) {
        perror("kill (SIGTERM) failed");
        return false;
    }
    std::cout << "Sent SIGTERM to PID " << pid << ", waiting..." << std::endl;

    sleep(1);

    if (kill(pid, 0) == 0) {
        std::cout << "PID " << pid << " did not terminate with SIGTERM" << std::endl;
        if (kill(pid, SIGKILL) == -1) {
            perror("kill (SIGKILL) failed");
            return false;
        }
    }
    std::cout << "Process with PID " << pid << " terminated successfully." << std::endl;
    return true;
}

void killProcessesByName(const std::string& name) {
    std::cout << "Looking for processes named: " << name << std::endl;
    std::vector<pid_t> pids = getPidsByName(name);

    if (pids.empty()) {
        std::cout << "No processes found matching name: " << name << std::endl;
        return;
    }

    for (pid_t pid : pids) {
        std::cout << "Found process: " << name << " (PID: " << pid << ")" << std::endl;
        killProcessById(pid);
    }
}

int main(int argc, char* argv[]) {
    std::vector<pid_t> pids_to_kill;
    std::vector<std::string> names_to_kill;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--id" && i + 1 < argc) {
            try {
                pid_t pid = std::stoi(argv[++i]);
                pids_to_kill.push_back(pid);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid PID provided with --id: " << argv[i] << std::endl;
                return 1;
            }
        } else if (arg == "--name" && i + 1 < argc) {
            names_to_kill.push_back(argv[++i]);
        }
    }

    for (pid_t pid : pids_to_kill) {
        killProcessById(pid);
    }

    for (const std::string& name : names_to_kill) {
        killProcessesByName(name);
    }

    const char* env_names = std::getenv("PROC_TO_KILL");
    if (env_names != nullptr) {
        std::string env_names_str(env_names);
        std::vector<std::string> env_names_vec = split(env_names_str, ',');
        for (const std::string& name : env_names_vec) {
            if (!name.empty()) {
                std::cout << "EV for process: " << name << std::endl;
                killProcessesByName(name);
            }
        }
    } else {
        std::cout << "Environment variable PROC_TO_KILL not found." << std::endl;
    }

    return 0;
}