#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <filesystem>
#include <iomanip>
#include <cstdlib>

using namespace std;
namespace fs = filesystem;

#define ANSI_RESET   "\033[0m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"

enum ColorCode { COLOR_CYAN = 0, COLOR_GREEN = 1, COLOR_YELLOW = 2 };

const char* color_to_ansi(int code) {
    switch (code) {
        case COLOR_CYAN:   return ANSI_CYAN;
        case COLOR_GREEN:  return ANSI_GREEN;
        case COLOR_YELLOW: return ANSI_YELLOW;
        default:           return ANSI_RESET;
    }
}

int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    const char* cols = getenv("COLUMNS");
    if (cols && *cols) return stoi(cols);
    return 80;
}

string exec_cmd(const string& cmd, bool multiline = false) {
    string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
        if (!multiline) break;
    }
    pclose(pipe);
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

string read_first_line(const string& path) {
    ifstream f(path);
    string line;
    if (getline(f, line)) return line;
    return "";
}

vector<pair<string, int>> get_system_info() {
    vector<pair<string, int>> items;
    
    string distro = "Unknown";
    if (fs::exists("/etc/os-release")) {
        ifstream f("/etc/os-release");
        string line;
        while (getline(f, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                distro = line.substr(13);
                if (distro.front() == '"' && distro.back() == '"')
                    distro = distro.substr(1, distro.size()-2);
                break;
            }
        }
    } else if (fs::exists("/etc/lsb-release")) {
        ifstream f("/etc/lsb-release");
        string line;
        while (getline(f, line)) {
            if (line.find("DISTRIB_DESCRIPTION=") == 0) {
                distro = line.substr(21);
                if (distro.front() == '"' && distro.back() == '"')
                    distro = distro.substr(1, distro.size()-2);
                break;
            }
        }
    } else {
        struct utsname uts;
        if (uname(&uts) == 0) distro = string(uts.sysname) + " " + uts.release;
    }
    items.emplace_back("Distro: " + distro, COLOR_CYAN);
    
    struct utsname uts;
    if (uname(&uts) == 0)
        items.emplace_back("Kernel: " + string(uts.release), COLOR_CYAN);
    
    ifstream uptime_f("/proc/uptime");
    double uptime_sec = 0;
    if (uptime_f >> uptime_sec) {
        int days = uptime_sec / 86400;
        int hours = (int(uptime_sec) % 86400) / 3600;
        int minutes = (int(uptime_sec) % 3600) / 60;
        char buf[64];
        if (days > 0)
            snprintf(buf, sizeof(buf), "Uptime: %d days, %02d:%02d", days, hours, minutes);
        else
            snprintf(buf, sizeof(buf), "Uptime: %02d:%02d", hours, minutes);
        items.emplace_back(buf, COLOR_CYAN);
    }
    
    ifstream load_f("/proc/loadavg");
    string load;
    if (getline(load_f, load)) {
        size_t pos = load.find(' ');
        if (pos != string::npos)
            items.emplace_back("Load average: " + load.substr(0, pos), COLOR_CYAN);
    }
    
    const char* user = getenv("USER");
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        string user_host = user ? string(user) + "@" + hostname : hostname;
        items.emplace_back("User: " + user_host, COLOR_CYAN);
    } else if (user) {
        items.emplace_back("User: " + string(user), COLOR_CYAN);
    }
    
    const char* shell = getenv("SHELL");
    if (shell) {
        string s = shell;
        size_t pos = s.rfind('/');
        if (pos != string::npos)
            items.emplace_back("Shell: " + s.substr(pos+1), COLOR_CYAN);
        else
            items.emplace_back("Shell: " + s, COLOR_CYAN);
    }
    
    int proc_count = 0;
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            string name = entry.path().filename().string();
            if (all_of(name.begin(), name.end(), ::isdigit))
                proc_count++;
        }
    }
    items.emplace_back("Processes: " + to_string(proc_count), COLOR_CYAN);
    
    return items;
}

vector<pair<string, int>> get_hardware_info() {
    vector<pair<string, int>> items;
    
    ifstream cpuinfo("/proc/cpuinfo");
    string cpu_model;
    int cores = 0;
    string cpu_freq, cpu_cache;
    if (cpuinfo.is_open()) {
        string line;
        bool model_found = false;
        while (getline(cpuinfo, line)) {
            if (line.find("model name") == 0 && !model_found) {
                size_t pos = line.find(':');
                if (pos != string::npos) {
                    cpu_model = line.substr(pos+2);
                    model_found = true;
                }
            } else if (line.find("processor") == 0) {
                cores++;
            } else if (line.find("cpu MHz") == 0) {
                size_t pos = line.find(':');
                if (pos != string::npos) cpu_freq = line.substr(pos+2) + " MHz";
            } else if (line.find("cache size") == 0) {
                size_t pos = line.find(':');
                if (pos != string::npos) cpu_cache = line.substr(pos+2);
            }
        }
    }
    if (!cpu_model.empty()) {
        items.emplace_back("CPU: " + cpu_model + " (" + to_string(cores) + " cores)", COLOR_GREEN);
        if (!cpu_freq.empty()) items.emplace_back("  Freq: " + cpu_freq, COLOR_GREEN);
        if (!cpu_cache.empty()) items.emplace_back("  Cache: " + cpu_cache, COLOR_GREEN);
    } else {
        items.emplace_back("CPU: N/A", COLOR_GREEN);
    }
    
    long mem_total = 0, mem_avail = 0, swap_total = 0, swap_free = 0;
    ifstream meminfo("/proc/meminfo");
    string line;
    while (getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) mem_total = stol(line.substr(line.find_first_of("0123456789")));
        else if (line.find("MemAvailable:") == 0) mem_avail = stol(line.substr(line.find_first_of("0123456789")));
        else if (line.find("SwapTotal:") == 0) swap_total = stol(line.substr(line.find_first_of("0123456789")));
        else if (line.find("SwapFree:") == 0) swap_free = stol(line.substr(line.find_first_of("0123456789")));
    }
    if (mem_total > 0) {
        double total_gb = mem_total / 1024.0 / 1024.0;
        double used_gb = (mem_total - mem_avail) / 1024.0 / 1024.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "Memory: %.1f GiB / %.1f GiB (%.1f%%)", used_gb, total_gb, (used_gb/total_gb)*100);
        items.emplace_back(buf, COLOR_GREEN);
    }
    if (swap_total > 0) {
        double swap_used_gb = (swap_total - swap_free) / 1024.0 / 1024.0;
        double swap_total_gb = swap_total / 1024.0 / 1024.0;
        char buf[64];
        snprintf(buf, sizeof(buf), "Swap:   %.1f GiB / %.1f GiB (%.1f%%)", swap_used_gb, swap_total_gb, (swap_used_gb/swap_total_gb)*100);
        items.emplace_back(buf, COLOR_GREEN);
    } else {
        items.emplace_back("Swap: N/A", COLOR_GREEN);
    }
    
    string gpu_line = exec_cmd("lspci | grep -i 'vga\\|3d\\|display' | head -1", false);
    if (!gpu_line.empty()) {
        size_t pos = gpu_line.find(':');
        if (pos != string::npos) {
            string gpu_name = gpu_line.substr(pos+2);
            items.emplace_back("GPU: " + gpu_name, COLOR_GREEN);
            string gpu_id = gpu_line.substr(0, pos);
            string driver = exec_cmd("lspci -v -s " + gpu_id + " | grep 'Kernel driver' | awk -F': ' '{print $2}'", false);
            if (!driver.empty()) items.emplace_back("  Driver: " + driver, COLOR_GREEN);
        }
    } else {
        items.emplace_back("GPU: N/A", COLOR_GREEN);
    }
    
    string disks = exec_cmd("lsblk -o NAME,MODEL,SIZE,MOUNTPOINT,FSTYPE,USE% -l 2>/dev/null | grep -v '^loop' | head -10", true);
    if (!disks.empty()) {
        items.emplace_back("Disks:", COLOR_GREEN);
        istringstream iss(disks);
        string disk_line;
        bool first = true;
        while (getline(iss, disk_line)) {
            if (first) { first = false; continue; }
            if (!disk_line.empty()) items.emplace_back("  " + disk_line, COLOR_GREEN);
        }
    } else {
        string df_out = exec_cmd("df -h | head -5", true);
        if (!df_out.empty()) {
            items.emplace_back("Disk usage:", COLOR_GREEN);
            istringstream iss(df_out);
            string line;
            while (getline(iss, line)) items.emplace_back("  " + line, COLOR_GREEN);
        }
    }
    
    string mb = exec_cmd("dmidecode -s baseboard-product-name 2>/dev/null", false);
    if (mb.empty()) mb = read_first_line("/sys/class/dmi/id/board_name");
    if (!mb.empty() && mb != "N/A") items.emplace_back("Motherboard: " + mb, COLOR_GREEN);
    
    string bios = exec_cmd("dmidecode -s bios-version 2>/dev/null", false);
    if (bios.empty()) bios = read_first_line("/sys/class/dmi/id/bios_version");
    if (!bios.empty() && bios != "N/A") items.emplace_back("BIOS: " + bios, COLOR_GREEN);
    
    vector<string> temps;
    if (fs::exists("/sys/class/thermal/")) {
        for (const auto& entry : fs::directory_iterator("/sys/class/thermal/")) {
            string name = entry.path().filename().string();
            if (name.find("thermal_zone") == 0) {
                string type = read_first_line(entry.path().string() + "/type");
                string temp_str = read_first_line(entry.path().string() + "/temp");
                if (!temp_str.empty()) {
                    long temp_val = stol(temp_str);
                    double temp_c = temp_val / 1000.0;
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Temp %s: %.1f°C", type.c_str(), temp_c);
                    temps.emplace_back(buf);
                }
            }
        }
    }
    if (temps.empty()) {
        string sensor_out = exec_cmd("sensors 2>/dev/null | grep -E 'Core|Package|temp' | head -3", true);
        istringstream iss(sensor_out);
        string line;
        while (getline(iss, line)) if (!line.empty()) temps.push_back(line);
    }
    if (!temps.empty()) {
        items.emplace_back("Temperatures:", COLOR_GREEN);
        for (const auto& t : temps) items.emplace_back("  " + t, COLOR_GREEN);
    }
    
    if (fs::exists("/sys/class/power_supply/BAT0")) {
        string capacity = read_first_line("/sys/class/power_supply/BAT0/capacity");
        string status = read_first_line("/sys/class/power_supply/BAT0/status");
        if (!capacity.empty()) {
            string bat_info = "Battery: " + (status.empty() ? "" : status + " ") + capacity + "%";
            items.emplace_back(bat_info, COLOR_GREEN);
        }
    }
    
    return items;
}

vector<pair<string, int>> get_periphery_info() {
    vector<pair<string, int>> items;
    
    string usb = exec_cmd("lsusb | head -10", true);
    if (!usb.empty()) {
        items.emplace_back("USB devices:", COLOR_YELLOW);
        istringstream iss(usb);
        string line;
        while (getline(iss, line)) items.emplace_back("  " + line, COLOR_YELLOW);
    }
    
    string pci = exec_cmd("lspci | grep -E 'Network|Audio|Storage|USB|SATA' | head -10", true);
    if (!pci.empty()) {
        items.emplace_back("PCI devices:", COLOR_YELLOW);
        istringstream iss(pci);
        string line;
        while (getline(iss, line)) {
            size_t pos = line.find(':');
            if (pos != string::npos) items.emplace_back("  " + line.substr(pos+2), COLOR_YELLOW);
            else items.emplace_back("  " + line, COLOR_YELLOW);
        }
    }
    
    string audio = exec_cmd("aplay -l 2>/dev/null | grep 'card' | head -5", true);
    if (!audio.empty()) {
        items.emplace_back("Audio devices:", COLOR_YELLOW);
        istringstream iss(audio);
        string line;
        while (getline(iss, line)) items.emplace_back("  " + line, COLOR_YELLOW);
    }
    
    vector<string> ifaces;
    string ip_link = exec_cmd("ip -o link show | awk -F': ' '{print $2}'", true);
    istringstream iss_link(ip_link);
    string ifname;
    while (getline(iss_link, ifname)) {
        if (ifname == "lo") continue;
        string mac = read_first_line("/sys/class/net/" + ifname + "/address");
        string speed = exec_cmd("ethtool " + ifname + " 2>/dev/null | grep Speed | awk '{print $2}'", false);
        if (speed.empty()) speed = "?";
        if (!mac.empty()) ifaces.push_back(ifname + " (" + mac + ", " + speed + ")");
        else ifaces.push_back(ifname);
    }
    if (!ifaces.empty()) {
        items.emplace_back("Network interfaces:", COLOR_YELLOW);
        for (const auto& iface : ifaces) items.emplace_back("  " + iface, COLOR_YELLOW);
    }
    
    vector<string> ips;
    string ipv4 = exec_cmd("ip -4 -o addr show | awk '{print $2 \": \" $4}'", true);
    string ipv6 = exec_cmd("ip -6 -o addr show | awk '{print $2 \": \" $4}'", true);
    istringstream iss4(ipv4), iss6(ipv6);
    string line;
    while (getline(iss4, line)) if (!line.empty()) ips.push_back(line);
    while (getline(iss6, line)) if (!line.empty()) ips.push_back(line);
    if (!ips.empty()) {
        items.emplace_back("IP addresses:", COLOR_YELLOW);
        for (const auto& ip : ips) items.emplace_back("  " + ip, COLOR_YELLOW);
    }
    
    string wifi_ssid = exec_cmd("iwgetid -r 2>/dev/null", false);
    if (wifi_ssid.empty()) wifi_ssid = exec_cmd("nmcli -t -f active,ssid dev wifi | grep '^yes' | cut -d: -f2 2>/dev/null", false);
    if (!wifi_ssid.empty()) {
        items.emplace_back("Wi-Fi SSID: " + wifi_ssid, COLOR_YELLOW);
    } else {
        string eth_check = exec_cmd("ip -o link show | grep -v lo | grep -v wl | grep -v 'state DOWN' | head -1", false);
        if (!eth_check.empty()) items.emplace_back("Network: Ethernet (cable)", COLOR_YELLOW);
        else items.emplace_back("Network: No active connection", COLOR_YELLOW);
    }
    
    string arp_table = exec_cmd("arp -n | tail -n +2 | head -10", true);
    if (arp_table.empty()) arp_table = exec_cmd("ip neigh show | head -10", true);
    if (!arp_table.empty()) {
        items.emplace_back("Local network devices (ARP/neigh):", COLOR_YELLOW); // std::std::std::
        istringstream iss(arp_table);
        string line;
        while (getline(iss, line)) if (!line.empty()) items.emplace_back("  " + line, COLOR_YELLOW);
    }
    
    vector<string> resolutions;
    string xrandr_out = exec_cmd("xrandr --current 2>/dev/null | grep '*' | awk '{print $1}'", true);
    if (!xrandr_out.empty()) {
        istringstream iss(xrandr_out);
        string res;
        while (getline(iss, res)) resolutions.push_back(res);
    }
    if (resolutions.empty()) {
        for (const auto& entry : fs::directory_iterator("/sys/class/drm/")) {
            string modes = read_first_line(entry.path().string() + "/modes");
            if (!modes.empty()) resolutions.push_back(modes);
        }
    }
    if (!resolutions.empty()) {
        items.emplace_back("Resolutions:", COLOR_YELLOW);
        for (const auto& res : resolutions) items.emplace_back("  " + res, COLOR_YELLOW);
    }
    
    return items;
}

void print_columns(const vector<pair<string, int>>& items, int term_width) {
    if (items.empty()) return;
    vector<string> strings;
    vector<int> colors;
    for (const auto& p : items) {
        strings.push_back(p.first);
        colors.push_back(p.second);
    }
    size_t max_len = 0;
    for (const auto& s : strings) max_len = max(max_len, s.size());
    int gap = 2, cols = 1;
    for (int c = 6; c >= 1; --c) {
        if ((max_len + gap) * c - gap <= term_width) { cols = c; break; }
    }
    int rows = (strings.size() + cols - 1) / cols;
    vector<size_t> col_widths(cols, 0);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            size_t idx = c * rows + r;
            if (idx < strings.size()) col_widths[c] = max(col_widths[c], strings[idx].size());
        }
    }
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            size_t idx = c * rows + r;
            if (idx < strings.size()) {
                cout << color_to_ansi(colors[idx]) << left << setw(col_widths[c] + gap)
                     << strings[idx] << ANSI_RESET;
            } else {
                cout << string(col_widths[c] + gap, ' ');
            }
        }
        cout << '\n';
    }
}

int main() {
    int term_width = get_terminal_width();
    auto sys_info = get_system_info();
    auto hw_info = get_hardware_info();
    auto peri_info = get_periphery_info();
    vector<pair<string, int>> all;
    all.reserve(sys_info.size() + hw_info.size() + peri_info.size());
    all.insert(all.end(), sys_info.begin(), sys_info.end());
    all.insert(all.end(), hw_info.begin(), hw_info.end());
    all.insert(all.end(), peri_info.begin(), peri_info.end());
    print_columns(all, term_width);
    return 0;
}
//для компиляции/for compile::  g++ -std=c++17 -o maxfetch maxfetch.cpp