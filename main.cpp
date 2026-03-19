#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cassert>
#include <sstream>
#include <unordered_map>
#include <memory>

using namespace std;

// Judge status enum
enum class JudgeStatus {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed
};

// Convert string to JudgeStatus
JudgeStatus stringToStatus(const string& s) {
    if (s == "Accepted") return JudgeStatus::Accepted;
    if (s == "Wrong_Answer") return JudgeStatus::Wrong_Answer;
    if (s == "Runtime_Error") return JudgeStatus::Runtime_Error;
    if (s == "Time_Limit_Exceed") return JudgeStatus::Time_Limit_Exceed;
    throw runtime_error("Invalid judge status: " + s);
}

// Convert JudgeStatus to string
string statusToString(JudgeStatus status) {
    switch (status) {
        case JudgeStatus::Accepted: return "Accepted";
        case JudgeStatus::Wrong_Answer: return "Wrong_Answer";
        case JudgeStatus::Runtime_Error: return "Runtime_Error";
        case JudgeStatus::Time_Limit_Exceed: return "Time_Limit_Exceed";
    }
    return "";
}

// Team class to store team information
class Team {
public:
    string name;

    // Problem-related data
    struct ProblemInfo {
        int total_submissions = 0;  // total submissions (including frozen)
        int wrong_attempts = 0;     // wrong attempts before first AC (excluding frozen)
        int frozen_submissions = 0; // submissions after freeze
        bool solved = false;
        int solve_time = 0;         // time of first AC
        int last_submission_time = 0;
        JudgeStatus last_submission_status;

        // For querying submissions
        vector<tuple<int, JudgeStatus, int>> submission_history; // time, status, problem_idx
    };

    vector<ProblemInfo> problems;
    int problem_count;

    // Overall statistics
    int solved_count = 0;
    int penalty_time = 0;
    vector<int> solve_times; // times when problems were solved

    // For ranking comparison
    bool operator<(const Team& other) const {
        // First by solved count (descending)
        if (solved_count != other.solved_count) {
            return solved_count > other.solved_count;
        }
        // Then by penalty time (ascending)
        if (penalty_time != other.penalty_time) {
            return penalty_time < other.penalty_time;
        }
        // Then by solve times (largest first, smaller is better)
        // solve_times is already sorted in descending order
        for (size_t i = 0; i < min(solve_times.size(), other.solve_times.size()); i++) {
            if (solve_times[i] != other.solve_times[i]) {
                return solve_times[i] < other.solve_times[i]; // smaller is better
            }
        }
        if (solve_times.size() != other.solve_times.size()) {
            return solve_times.size() > other.solve_times.size(); // more solved problems is better
        }
        // Finally by name (ascending)
        return name < other.name;
    }

    Team(const string& name, int problem_count) : name(name), problem_count(problem_count) {
        problems.resize(problem_count);
        solve_times.reserve(problem_count);
    }

    // Submit a problem
    void submit(int problem_idx, JudgeStatus status, int time) {
        auto& prob = problems[problem_idx];
        prob.total_submissions++;
        prob.last_submission_time = time;
        prob.last_submission_status = status;
        prob.submission_history.emplace_back(time, status, problem_idx);

        if (prob.solved) {
            // Already solved, no further effect
            return;
        }

        if (status == JudgeStatus::Accepted) {
            prob.solved = true;
            prob.solve_time = time;
            solved_count++;

            // Calculate penalty for this problem: 20 * wrong_attempts + solve_time
            int problem_penalty = 20 * prob.wrong_attempts + time;
            penalty_time += problem_penalty;

            // Add solve time for ranking
            solve_times.push_back(time);
            sort(solve_times.rbegin(), solve_times.rend()); // descending

            // Reset frozen submissions since problem is now solved
            prob.frozen_submissions = 0;
        } else {
            // Wrong submission before solving
            prob.wrong_attempts++;
        }
    }

    // Freeze a problem (mark that submissions after this are frozen)
    void freeze_problem(int problem_idx) {
        auto& prob = problems[problem_idx];
        if (!prob.solved) {
            prob.frozen_submissions = 0; // Start counting frozen submissions
        }
    }

    // Add a frozen submission
    void add_frozen_submission(int problem_idx, JudgeStatus status, int time) {
        auto& prob = problems[problem_idx];
        if (!prob.solved && prob.frozen_submissions >= 0) {
            prob.total_submissions++;
            prob.frozen_submissions++;
            prob.submission_history.emplace_back(time, status, problem_idx);
        }
    }

    // Unfreeze a problem - apply frozen submissions
    // Returns true if the problem becomes solved after unfreezing
    bool unfreeze_problem(int problem_idx) {
        auto& prob = problems[problem_idx];
        if (prob.solved || prob.frozen_submissions == 0) {
            return false;
        }

        // We would need to apply frozen submissions here
        // For now, just clear frozen submissions
        prob.frozen_submissions = 0;
        return false;
    }

    // Get problem display string
    string get_problem_display(int problem_idx, bool is_frozen) const {
        const auto& prob = problems[problem_idx];

        if (is_frozen && prob.frozen_submissions > 0) {
            // Frozen problem
            if (prob.wrong_attempts == 0) {
                return "0/" + to_string(prob.frozen_submissions);
            } else {
                return "-" + to_string(prob.wrong_attempts) + "/" + to_string(prob.frozen_submissions);
            }
        }

        if (prob.solved) {
            // Solved problem
            if (prob.wrong_attempts == 0) {
                return "+";
            } else {
                return "+" + to_string(prob.wrong_attempts);
            }
        } else {
            // Unsolved problem
            if (prob.wrong_attempts == 0 && prob.total_submissions == 0) {
                return ".";
            } else {
                return "-" + to_string(prob.wrong_attempts);
            }
        }
    }

    // Query submission
    tuple<int, JudgeStatus, int> query_submission(int problem_filter, const string& status_filter_str) const {
        // problem_filter: -1 for ALL, otherwise problem index
        // status_filter_str: "ALL" or status string
        // Returns: (time, status, problem_idx)

        int last_time = -1;
        JudgeStatus last_status = JudgeStatus::Accepted;
        int last_problem_idx = -1;
        bool found = false;

        for (int i = 0; i < problem_count; i++) {
            if (problem_filter != -1 && problem_filter != i) continue;

            for (const auto& [time, status, prob_idx] : problems[i].submission_history) {
                if (status_filter_str != "ALL") {
                    JudgeStatus filter_status = stringToStatus(status_filter_str);
                    if (status != filter_status) continue;
                }

                if (time > last_time) {
                    last_time = time;
                    last_status = status;
                    last_problem_idx = prob_idx;
                    found = true;
                }
            }
        }

        if (!found) {
            return {-1, JudgeStatus::Accepted, -1};
        }
        return {last_time, last_status, last_problem_idx};
    }
};

// ICPC System class
class ICPCSystem {
private:
    bool competition_started = false;
    bool frozen = false;
    int duration_time = 0;
    int problem_count = 0;

    // Team management
    unordered_map<string, shared_ptr<Team>> teams;
    vector<string> team_order; // maintains insertion order for initial ranking

    // Scoreboard ranking (indices into team_order)
    vector<int> ranking;

    // For freeze/scroll
    map<pair<int, string>, int> frozen_problems; // (team_idx, problem_name) -> count

    // Parse problem name to index
    int problem_name_to_idx(const string& name) const {
        if (name.length() != 1) return -1;
        return name[0] - 'A';
    }

    string problem_idx_to_name(int idx) const {
        return string(1, 'A' + idx);
    }

    // Update ranking based on current team stats
    void update_ranking() {
        vector<shared_ptr<Team>> team_ptrs;
        for (const auto& name : team_order) {
            team_ptrs.push_back(teams[name]);
        }

        // Sort team pointers using Team's comparator
        sort(team_ptrs.begin(), team_ptrs.end(),
             [](const shared_ptr<Team>& a, const shared_ptr<Team>& b) {
                 return *a < *b;
             });

        // Update ranking indices
        ranking.clear();
        ranking.reserve(team_ptrs.size());
        for (const auto& team_ptr : team_ptrs) {
            // Find index in team_order
            auto it = find(team_order.begin(), team_order.end(), team_ptr->name);
            ranking.push_back(distance(team_order.begin(), it));
        }
    }

    // Get team ranking (1-based)
    int get_team_ranking(const string& team_name) const {
        auto it = find(team_order.begin(), team_order.end(), team_name);
        if (it == team_order.end()) return -1;

        int team_idx = distance(team_order.begin(), it);

        // Find in ranking
        for (size_t i = 0; i < ranking.size(); i++) {
            if (ranking[i] == team_idx) {
                return i + 1; // 1-based ranking
            }
        }
        return -1;
    }

public:
    ICPCSystem() = default;

    // Command handlers
    void handle_addteam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }

        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        teams[team_name] = make_shared<Team>(team_name, problem_count);
        team_order.push_back(team_name);
        cout << "[Info]Add successfully.\n";
    }

    void handle_start(int duration, int problem_cnt) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        duration_time = duration;
        problem_count = problem_cnt;
        competition_started = true;

        // Initialize all teams with proper problem count
        for (auto& [name, team] : teams) {
            team->problems.resize(problem_count);
            team->problem_count = problem_count;
        }

        cout << "[Info]Competition starts.\n";
    }

    void handle_submit(const string& problem_name, const string& team_name,
                      const string& status_str, int time) {
        if (!competition_started) return;

        int problem_idx = problem_name_to_idx(problem_name);
        if (problem_idx < 0 || problem_idx >= problem_count) return;

        JudgeStatus status = stringToStatus(status_str);
        auto& team = teams[team_name];

        if (frozen && !team->problems[problem_idx].solved) {
            // Frozen submission
            team->add_frozen_submission(problem_idx, status, time);

            // Track frozen problem
            frozen_problems[{problem_name_to_idx(problem_name), team_name}]++;
        } else {
            // Normal submission
            team->submit(problem_idx, status, time);
        }
    }

    void handle_flush() {
        if (!competition_started) return;

        update_ranking();
        cout << "[Info]Flush scoreboard.\n";
    }

    void handle_freeze() {
        if (!competition_started) return;

        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        frozen = true;
        frozen_problems.clear();

        // Mark all unsolved problems as potentially frozen
        for (auto& [name, team] : teams) {
            for (int i = 0; i < problem_count; i++) {
                if (!team->problems[i].solved) {
                    team->freeze_problem(i);
                }
            }
        }

        cout << "[Info]Freeze scoreboard.\n";
    }

    void handle_scroll() {
        if (!competition_started) return;

        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush to update ranking
        update_ranking();

        // Output scoreboard before scrolling
        print_scoreboard();

        // TODO: Implement actual unfreezing logic
        // For now, just clear frozen state
        frozen = false;
        frozen_problems.clear();

        // Output scoreboard after scrolling
        print_scoreboard();
    }

    void handle_query_ranking(const string& team_name) {
        if (!competition_started) return;

        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        int rank = get_team_ranking(team_name);
        cout << team_name << " NOW AT RANKING " << rank << "\n";
    }

    void handle_query_submission(const string& team_name, const string& problem_name,
                                const string& status_str) {
        if (!competition_started) return;

        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        auto& team = teams[team_name];

        // Parse filters
        int problem_filter = (problem_name == "ALL") ? -1 : problem_name_to_idx(problem_name);

        auto [time, status, prob_idx] = team->query_submission(problem_filter, status_str);

        if (time == -1) {
            cout << "Cannot find any submission.\n";
        } else {
            string problem_str = problem_idx_to_name(prob_idx);
            cout << team_name << " " << problem_str << " " << statusToString(status) << " " << time << "\n";
        }
    }

    void handle_end() {
        cout << "[Info]Competition ends.\n";
    }

    void print_scoreboard() {
        update_ranking();

        for (size_t i = 0; i < ranking.size(); i++) {
            int team_idx = ranking[i];
            const auto& team_name = team_order[team_idx];
            const auto& team = teams[team_name];

            cout << team_name << " " << (i + 1) << " "
                 << team->solved_count << " " << team->penalty_time;

            for (int j = 0; j < problem_count; j++) {
                bool is_frozen = frozen && frozen_problems.count({j, team_name}) > 0;
                cout << " " << team->get_problem_display(j, is_frozen);
            }
            cout << "\n";
        }
    }
};

// Main function
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            ss >> team_name;
            system.handle_addteam(team_name);
        }
        else if (command == "START") {
            string dummy;
            int duration, problem_cnt;
            ss >> dummy >> duration >> dummy >> problem_cnt;
            system.handle_start(duration, problem_cnt);
        }
        else if (command == "SUBMIT") {
            string problem_name, by_dummy, team_name, with_dummy, status_str, at_dummy;
            int time;
            ss >> problem_name >> by_dummy >> team_name >> with_dummy >> status_str >> at_dummy >> time;
            system.handle_submit(problem_name, team_name, status_str, time);
        }
        else if (command == "FLUSH") {
            system.handle_flush();
        }
        else if (command == "FREEZE") {
            system.handle_freeze();
        }
        else if (command == "SCROLL") {
            system.handle_scroll();
        }
        else if (command == "QUERY_RANKING") {
            string team_name;
            ss >> team_name;
            system.handle_query_ranking(team_name);
        }
        else if (command == "QUERY_SUBMISSION") {
            string team_name, where_dummy, problem_token, and_dummy, status_token;
            ss >> team_name >> where_dummy >> problem_token >> and_dummy >> status_token;

            // Parse problem_token (format: "PROBLEM=XYZ")
            string problem_name = "ALL";
            size_t eq_pos = problem_token.find('=');
            if (eq_pos != string::npos) {
                problem_name = problem_token.substr(eq_pos + 1);
            }

            // Parse status_token (format: "STATUS=XYZ")
            string status_str = "ALL";
            eq_pos = status_token.find('=');
            if (eq_pos != string::npos) {
                status_str = status_token.substr(eq_pos + 1);
            }

            system.handle_query_submission(team_name, problem_name, status_str);
        }
        else if (command == "END") {
            system.handle_end();
            break;
        }
    }

    return 0;
}