#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

bool isProcessRunning(pid_t pid) {
    return (kill(pid, 0) == 0);
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

bool IsProcessRunning(const std::string& name) {
    std::vector<pid_t> pids = getPidsByName(name);
    return !pids.empty();
}

void RunKiller(const std::string& args) {
    pid_t pid = fork();
    if (pid == 0) {
        std::string cmd = "./Killer " + args;
        std::vector<std::string> tokens = {"./Killer"};
        std::istringstream iss(args);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        tokens.push_back("");

        std::vector<char*> argv_tokens;
        for (auto& t : tokens) {
            argv_tokens.push_back(&t[0]);
        }
        argv_tokens.back() = nullptr;

        execv("./Killer", argv_tokens.data());
        perror("execv failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        std::cout << "Killer (args: " << args << ") finished." << std::endl;
    } else {
        perror("fork failed");
    }
}

int main() {
    std::string proc_names = "gedit, xcalc";

    std::cout << "--- Setting Environment Variable PROC_TO_KILL ---" << std::endl;
    if (setenv("PROC_TO_KILL", proc_names.c_str(), 1) != 0) {
         std::cerr << "setenv failed" << std::endl;
         return 1;
    }

    std::cout << "--- Launching test processes ---" << std::endl;
    system("gedit &");
    system("xcalc &");
    sleep(2);

    std::cout << "\n--- Checking if processes are running before Killer ---" << std::endl;
    std::vector<std::string> target_processes = {"gedit", "xcalc"};
    for (const auto& name : target_processes) {
        bool running = IsProcessRunning(name);
        std::cout << name << " running before: " << (running ? "Yes" : "No") << std::endl;
    }

    std::cout << "\n--- Running Killer with --name 'xterm' ---" << std::endl;
    RunKiller("--name xterm");
    sleep(1);

    std::cout << "\n--- Running Killer using PROC_TO_KILL variable ---" << std::endl;
    RunKiller("");
    sleep(2);

    std::cout << "\n--- Checking if processes are running after Killer ---" << std::endl;
    for (const auto& name : target_processes) {
        bool running = IsProcessRunning(name);
        std::cout << name << " running after: " << (running ? "Yes" : "No") << std::endl;
    }

    std::cout << "\n--- Removing EV PROC_TO_KILL ---" << std::endl;
    if (unsetenv("PROC_TO_KILL") != 0) {
         std::cerr << "unsetenv failed" << std::endl;
    }
    return 0;
}