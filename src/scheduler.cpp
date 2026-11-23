#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cctype>
using namespace std;

struct Request {
    int id;
    string name;
    int start, finish;   // minutes since midnight (e.g. 09:30 -> 570)
    int value;
};

// ---------------- Clock time helpers ----------------------------------------
int parseTime(const string &text) {
    size_t colon = text.find(':');
    if (colon == string::npos || colon == 0 || colon + 1 >= text.size()) return -1;

    string hourPart = text.substr(0, colon);
    string minutePart = text.substr(colon + 1);
    if (hourPart.size() > 2 || minutePart.size() != 2) return -1;

    for (size_t i = 0; i < hourPart.size(); i++)
        if (hourPart[i] < '0' || hourPart[i] > '9') return -1;
    for (size_t i = 0; i < minutePart.size(); i++)
        if (minutePart[i] < '0' || minutePart[i] > '9') return -1;

    int hours = atoi(hourPart.c_str());
    int minutes = atoi(minutePart.c_str());
    if (hours > 23 || minutes > 59) return -1;
    return hours * 60 + minutes;
}

// Converts minutes since midnight back to "HH:MM".
string formatTime(int totalMinutes) {
    ostringstream out;
    out << setfill('0') << setw(2) << totalMinutes / 60 << ":"
        << setfill('0') << setw(2) << totalMinutes % 60;
    return out.str();
}

// Asks until the user enters a valid HH:MM time.
int readTime(const string &prompt) {
    string text;
    int minutes;
    while (true) {
        cout << prompt;
        if (!(cin >> text)) { cout << "\nInput ended.\n"; exit(0); }
        minutes = parseTime(text);
        if (minutes >= 0) return minutes;
        cout << "Invalid time. Use 24-hour format HH:MM (e.g. 09:30 or 14:45).\n";
    }
}

// ---------------- Common helpers --------------------------------------------
bool byFinish(const Request &a, const Request &b) {
    if (a.finish != b.finish) return a.finish < b.finish;
    return a.start < b.start;
}

int totalValue(const vector<Request> &v) {
    int s = 0;
    for (size_t i = 0; i < v.size(); i++) s += v[i].value;
    return s;
}

void line() { cout << string(64, '-') << "\n"; }

void printRequests(const vector<Request> &v) {
    line();
    cout << left << setw(6) << "ID" << setw(22) << "Request" << setw(8) << "Start"
         << setw(8) << "Finish" << setw(8) << "Value" << "\n";
    line();
    for (size_t i = 0; i < v.size(); i++)
        cout << left << setw(6) << v[i].id << setw(22) << v[i].name
             << setw(8) << formatTime(v[i].start) << setw(8) << formatTime(v[i].finish)
             << setw(8) << v[i].value << "\n";
    line();
}

// ---------------- Greedy: Earliest Finish Time (unweighted) -----------------
vector<Request> greedy(vector<Request> r) {
    sort(r.begin(), r.end(), byFinish);          // O(n log n)
    vector<Request> chosen;
    int lastFinish = -1000000;
    for (size_t i = 0; i < r.size(); i++) {      // O(n)
        if (r[i].start >= lastFinish) {
            chosen.push_back(r[i]);
            lastFinish = r[i].finish;
        }
    }
    return chosen;
}

// ---------------- Binary search for p(i) ------------------------------------
int findP(const vector<Request> &jobs, int i) {
    int low = 1, high = i - 1, ans = 0;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (jobs[mid].finish <= jobs[i].start) { ans = mid; low = mid + 1; }
        else high = mid - 1;
    }
    return ans;
}

// ---------------- Verify no overlap -----------------------------------------
bool verify(const vector<Request> &sel) {
    for (size_t k = 0; k + 1 < sel.size(); k++) {
        if (sel[k].finish > sel[k + 1].start) {
            cout << "ERROR: Overlap between " << sel[k].name << " and "
                 << sel[k + 1].name << "\n";
            return false;
        }
    }
    cout << "No overlap detected - all selected requests are compatible.\n";
    return true;
}

// ---------------- Weighted DP (p(i) by binary search, DP, backtracking) -----
vector<Request> weightedDP(vector<Request> r) {
    int n = r.size();
    sort(r.begin(), r.end(), byFinish);

    vector<Request> jobs(1);                     // dummy job at index 0
    for (int i = 0; i < n; i++) jobs.push_back(r[i]);

    vector<int> p(n + 1, 0), dp(n + 1, 0);

    for (int i = 1; i <= n; i++) p[i] = findP(jobs, i);   // O(n log n)

    dp[0] = 0;
    for (int i = 1; i <= n; i++) {                        // O(n)
        int include = jobs[i].value + dp[p[i]];
        int exclude = dp[i - 1];
        dp[i] = max(include, exclude);
    }

    // Backtracking from DP[n]
    vector<Request> selected;
    int i = n;
    while (i > 0) {
        if (jobs[i].value + dp[p[i]] > dp[i - 1]) { selected.push_back(jobs[i]); i = p[i]; }
        else i--;
    }
    reverse(selected.begin(), selected.end());
    return selected;
}

// Removes spaces at the start and end of a text.
string trimSpaces(const string &text) {
    size_t first = text.find_first_not_of(' ');
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(' ');
    return text.substr(first, last - first + 1);
}

// A valid appliance/EV name has letters only (spaces allowed between words)
bool isValidName(const string &name) {
    bool hasLetter = false;
    for (size_t i = 0; i < name.size(); i++) {
        if (isalpha((unsigned char)name[i])) hasLetter = true;
        else if (name[i] != ' ') return false;
    }
    return hasLetter;
}

// Returns true if a request with this ID has already been entered.
bool idExists(const vector<Request> &reqs, int id) {
    for (size_t i = 0; i < reqs.size(); i++)
        if (reqs[i].id == id) return true;
    return false;
}

// ---------------- Input -----------------------------------------------------
vector<Request> readRequests() {
    vector<Request> reqs;
    int n = 0;
    cout << "Enter number of requests: ";
    while (!(cin >> n) || n <= 0) {
        if (cin.eof()) { cout << "\nInput ended.\n"; exit(0); }
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Please enter a positive number: ";
    }

    for (int i = 0; i < n; i++) {
        Request r;
        cout << "\n--- Request " << i + 1 << " ---\n";
        while (true) {
            cout << "Request ID: ";
            if (cin >> r.id) {
                if (idExists(reqs, r.id)) {
                    cout << "Request ID " << r.id << " is already used. "
                         << "The request ID must not be the same as an already entered one. "
                         << "Please enter a different ID.\n";
                    continue;
                }
                break;                           
            }
            if (cin.eof()) { cout << "\nInput ended.\n"; exit(0); }
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Please enter a number for the ID.\n";
        }
        cin.ignore(10000, '\n');
        while (true) {
            cout << "Appliance/EV name: ";
            if (!getline(cin, r.name)) { cout << "\nInput ended.\n"; exit(0); }
            r.name = trimSpaces(r.name);
            if (isValidName(r.name)) break;
            cout << "Invalid name. Use letters only (spaces allowed), e.g. Washing Machine.\n";
        }

        r.start = readTime("Start time  (HH:MM, 24-hour): ");
        r.finish = readTime("Finish time (HH:MM, 24-hour): ");
        cout << "Value: ";
        while (!(cin >> r.value) || r.value < 0) {
            if (cin.eof()) { cout << "\nInput ended.\n"; exit(0); }
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Please enter a non-negative value: ";
        }

        if (r.start >= r.finish) {
            cout << "Invalid request: finish time must be later than start time. Re-enter.\n";
            i--;
            continue;
        }
        reqs.push_back(r);
    }
    return reqs;
}

int main() {
    cout << "=== Appliance and EV Load Scheduling with Weighted Intervals ===\n";
    cout << "Enter times in 24-hour clock format HH:MM (e.g. 09:30, 14:45).\n";
    cout << "Compatibility rule: requests are compatible when start >= previous_finish\n\n";

    vector<Request> reqs = readRequests();

    cout << "\n=========== INPUT REQUESTS ===========\n";
    printRequests(reqs);

    // ============ 1. GREEDY ============
    cout << "\n=========== 1. UNWEIGHTED GREEDY (Earliest Finish Time) ===========\n";
    vector<Request> g = greedy(reqs);
    printRequests(g);
    cout << "Number of selected requests : " << g.size() << "\n";
    cout << "Total value of selection    : " << totalValue(g) << "  (greedy ignores values)\n";
    verify(g);

    // ============ 2. WEIGHTED DP ============
    cout << "\n=========== 2. WEIGHTED INTERVAL SCHEDULING (DP) ===========\n";
    vector<Request> d = weightedDP(reqs);
    cout << "Selected requests (chronological order):\n";
    printRequests(d);
    cout << "Number of selected requests : " << d.size() << "\n";
    cout << "Maximum total value         : " << totalValue(d) << "\n";
    verify(d);

    // ============ 3. GREEDY FAILURE CHECK (from the user's input) ============
    cout << "\n=========== 3. GREEDY FAILURE CASE (from your input) ===========\n";
    int greedyValue = totalValue(g);
    int dpValue = totalValue(d);
    cout << "Greedy total value : " << greedyValue << "  (" << g.size() << " requests)\n";
    cout << "DP optimal value   : " << dpValue << "  (" << d.size() << " requests)\n";

    if (dpValue > greedyValue) {
        cout << "\nGREEDY FAILS on your input.\n";
        cout << "\nGreedy solution (earliest finish time):\n";
        printRequests(g);
        cout << "Greedy total value  = " << greedyValue << "\n";
        cout << "\nOptimal solution (DP):\n";
        printRequests(d);
        cout << "Optimal total value = " << dpValue << "\n";
        cout << "\nValue lost by greedy = " << dpValue - greedyValue << "\n";
        cout << "\nEarliest finish time maximizes the NUMBER of compatible requests,\n"
             << "but it does not necessarily maximize their TOTAL VALUE.\n"
             << "So the unweighted greedy algorithm cannot directly solve the weighted problem.\n";
    } else {
        cout << "\nGreedy did NOT fail on your input: both give the same total value.\n"
             << "(Greedy is not guaranteed to match DP. Failure appears when a high-value\n"
             << "request overlaps several short low-value requests that finish earlier.)\n";
    }

    return 0;
}