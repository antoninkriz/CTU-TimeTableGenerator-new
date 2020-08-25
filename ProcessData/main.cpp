#include <csignal>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>
#include <boost/functional/hash.hpp>

using std::cin;
using std::cout;
using std::istream;
using std::flush;
using std::endl;
using std::fixed;
using std::setw;
using std::setfill;
using std::setprecision;
using std::size_t;
using std::invalid_argument;
using std::string;
using std::stringstream;
using std::vector;
using std::hash;
using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::make_shared;
using std::any_of;
using std::all_of;
using std::count_if;
using std::abs;
using std::numeric_limits;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using boost::hash_value;
using boost::hash_combine;
using nlohmann::json;

#define NE_730 (false)
#define NE_915 (false)

/// Break all loops and stop magic
static volatile bool keepRunning = true;

/// SIGINT handler
void sigintHandler(int) {
    keepRunning = false;
}

/// Cache to store and access already stored data
/// \tparam T Any hashable type
template<typename T>
class Cache {
private:
    unordered_map<T, shared_ptr<T>> p_data;

public:
    shared_ptr<T> cache(T t) {
        if (p_data.count(t))
            return p_data[t];

        auto ptr = shared_ptr<T>(new T(t));
        p_data[t] = ptr;
        return ptr;
    }
};

enum class Type {
    Lecture,
    Tutorial,
    Laboratory
};

Type parseType(const string &s) {
    if (s == "lecture")
        return Type::Lecture;
    else if (s == "tutorial")
        return Type::Tutorial;
    else if (s == "laboratory")
        return Type::Laboratory;
    throw invalid_argument("Not a Type value");
}

string typeToString(Type t) {
    switch (t) {
        case Type::Lecture:
            return "lecture";
        case Type::Tutorial:
            return "tutorial";
        case Type::Laboratory:
            return "laboratory";
    }

    throw invalid_argument("Not a Type value");
}

enum class Week {
    Odd,
    Even
};

Week parseWeek(const string &s) {
    if (s == "odd")
        return Week::Odd;
    else if (s == "even")
        return Week::Even;
    throw invalid_argument("Not a Week value");
}

string weekToString(Week w) {
    switch (w) {
        case Week::Odd:
            return "odd";
        case Week::Even:
            return "even";
    }

    throw invalid_argument("Not a Week value");
}

class Time {
public:
    int8_t p_hours;
    int8_t p_minutes;

    bool operator==(const Time &t) const {
        return p_hours == t.p_hours && p_minutes == t.p_minutes;
    }

    bool operator<(const Time &t) const {
        return p_hours < t.p_hours || (p_hours == t.p_hours && p_minutes < t.p_minutes);
    }

    bool operator>(const Time &t) const {
        return p_hours > t.p_hours || (p_hours == t.p_hours && p_minutes > t.p_minutes);
    }

    bool operator<=(const Time &t) const {
        return *this == t || *this < t;
    }

    bool operator>=(const Time &t) const {
        return *this == t || *this > t;
    }

    int diffMinutes(const Time &t) const {
        int minutes1 = p_hours * 60 + p_minutes;
        int minutes2 = t.p_hours * 60 + t.p_minutes;

        return abs(minutes1 - minutes2);
    }

    json dumpJson() const {
        return {
                {"hours",   p_hours},
                {"minutes", p_minutes}
        };
    }
};

Time parseTime(const string &s) {
    auto is = stringstream(s);

    char c = '\0';
    int h = -1, m = -1;
    is >> h >> c >> m;
    if (c != ':' || h <= 0 || m < 0)
        throw invalid_argument("Invalid time format");
    return {(int8_t) h, (int8_t) m};
}

class EventTime {
public:
    Week p_week;
    int8_t p_day;
    Time p_from;
    Time p_to;

    bool operator==(const EventTime &et) const {
        return p_week == et.p_week
               && p_day == et.p_day
               && p_from == et.p_from
               && p_to == et.p_to;
    }

    json dumpJson() const {
        return {
                {"week", weekToString(p_week)},
                {"day",  (int) p_day},
                {"from", p_from.dumpJson()},
                {"to",   p_to.dumpJson()},
        };
    }
};

class Event {
public:
    string p_name;
    string p_parallel;
    Type p_type;
    EventTime p_time;

    bool operator==(const Event &e) const {
        return p_name == e.p_name
               && p_parallel == e.p_parallel
               && p_time == e.p_time;
    }

    json dumpJson() const {
        return {
                {"name",     p_name},
                {"parallel", p_parallel},
                {"type",     typeToString(p_type)},
                {"time",     p_time.dumpJson()}
        };
    }
};

class TimeTable {
private:
    double p_score;
    vector<vector<shared_ptr<Event>>> p_days;

public:

    TimeTable() {
        p_score = -1;
        p_days = vector<vector<shared_ptr<Event>>>(10);
    }

    bool hasCollision(short day, Time from, Time to) const {
        return any_of(p_days[day].begin(), p_days[day].end(), [&to, &from](const shared_ptr<Event> &ev) {
            auto evFrom = ev->p_time.p_from;
            auto evTo = ev->p_time.p_to;

            if (from >= evFrom && from < evTo)
                return true;

            if (to > evFrom && to <= evTo)
                return true;

            return false;
        });
    }

    void addEvent(const shared_ptr<Event> &ev) {
        auto &day = p_days[ev->p_time.p_day];

        auto pos = std::lower_bound(day.begin(), day.end(), ev, [](const shared_ptr<Event> &a, const shared_ptr<Event> &b) {
            return a->p_time.p_from < b->p_time.p_from || (a->p_time.p_from == b->p_time.p_from && a->p_time.p_to < b->p_time.p_to);
        });

        day.insert(pos, ev);
        p_score = -1;
    }

    double getScore() {
        if (p_score != -1)
            return p_score;

        double totalScore = 0;

        // Score based on total hours and days spent in school
        int totalMinutesSpent = 0;
        int totalDaysInSchool = 0;
        for (const auto &day : p_days) {
            // Empty days
            if (day.empty()) {
                totalScore -= 30;
                continue;
            }

            // Lectures only
            int lecturesCount = count_if(day.begin(), day.end(), [](const shared_ptr<Event>& ev) { return ev->p_type == Type::Lecture; });
            if (lecturesCount == day.size()) {
                switch (lecturesCount) {
                    case 1:
                        totalScore -= 15;
                    case 2:
                        totalScore -= 10;
                    default:
                        totalScore -= 5;
                }
            }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
            if (NE_730 && day[0]->p_time.p_from <= Time{7, 30})
                totalScore += 5;

            if (NE_915 && day[0]->p_time.p_from <= Time{9, 15})
                totalScore += 1;
#pragma clang diagnostic pop
#pragma clang diagnostic pop

            // Total time spent
            totalMinutesSpent += day[0]->p_time.p_from.diffMinutes(day[day.size() - 1]->p_time.p_to);
            totalDaysInSchool++;
        }

        totalScore = (totalMinutesSpent / 60.0) * 10 * totalDaysInSchool;

        return p_score = totalScore;
    }

    json dumpJson() {
        auto jsonEvents = vector<vector<json>>(p_days.size());

        for (size_t i = 0; i < p_days.size(); i++)
            for (const auto &event : p_days[i])
                jsonEvents[i].push_back(event->dumpJson());

        return {
                {"score", this->getScore()},
                {"days", jsonEvents}
        };
    }
};

namespace std {
    template<>
    struct hash<Time> {
        size_t operator()(const Time &t) const {
            size_t seed = 0;
            hash_combine(seed, hash_value(t.p_hours));
            hash_combine(seed, hash_value(t.p_minutes));
            return seed;
        }
    };

    template<>
    struct hash<EventTime> {
        size_t operator()(const EventTime &et) const {
            size_t seed = 0;
            hash_combine(seed, hash_value(et.p_week));
            hash_combine(seed, hash_value(et.p_day));
            hash_combine(seed, hash<Time>()(et.p_from));
            hash_combine(seed, hash<Time>()(et.p_to));
            return seed;
        }
    };

    template<>
    struct hash<Event> {
        size_t operator()(const Event &e) const {
            size_t seed = 0;
            hash_combine(seed, hash_value(e.p_name));
            hash_combine(seed, hash_value(e.p_parallel));
            hash_combine(seed, hash<EventTime>()(e.p_time));
            return seed;
        }
    };
}

typedef vector<shared_ptr<Event>> Parallel;
typedef vector<Parallel> Parallels;
typedef unordered_map<Type, Parallels> Course;
typedef unordered_map<string, Course> Data;

Data buildData(const json &j, Cache<Event> &cacheEvents) {
    auto dataParsed = Data();
    for (auto&[courseName, courseTypes] : j.items()) {
        auto courseParsed = Course();

        for (auto &[courseType, parallels] : courseTypes.items()) {
            auto parallelsParsed = Parallels();
            auto type = parseType(courseType);

            for (auto &parallel : parallels) {
                auto parallelParsed = Parallel();

                for (auto &event : parallel) {

                    auto e = Event{
                            event["name"],
                            event["parallel"],
                            type,
                            EventTime{
                                    parseWeek(event["time"]["week"]),
                                    (int8_t) event["time"]["day"],
                                    parseTime(event["time"]["from"]),
                                    parseTime(event["time"]["to"]),
                            }
                    };

                    parallelParsed.push_back(cacheEvents.cache(e));
                }

                parallelsParsed.push_back(parallelParsed);
            }

            courseParsed[type] = parallelsParsed;
        }

        dataParsed[courseName] = courseParsed;
    }

    return dataParsed;
}

class Solver {
private:
    double p_bestScore;
    int p_combinations;
    vector<shared_ptr<TimeTable>> p_tables;
    vector<Parallels> p_flatData;

public:
    explicit Solver(const Data &data) {
        p_bestScore = numeric_limits<double>::max();
        p_combinations = 1;

        for (const auto &[subject, types] : data) {
            for (const auto &[type, parallels] : types) {
                if (parallels.empty())
                    continue;

                p_combinations *= parallels.size();
                p_flatData.push_back(parallels);
            }
        }
    }

    void solve(const shared_ptr<TimeTable> &timeTable = make_shared<TimeTable>(), size_t flatPos = 0) {
        for (const auto &parallel : p_flatData[flatPos]) {
            // Kill recursion when SIGINT comes
            if (!keepRunning) break;

            static auto start = high_resolution_clock::now();
            static auto steps = 0;
            steps++;

            // Print progress
            if (steps % 1000 == 0) {
                // How long did it take from the start?
                auto tmr = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();

                // Percentage progress of all combinations
                auto prc = (steps / (double) p_combinations) * 100;

                // Clear current line and print a new one over it
                cout << "\r\e[K" << flush
                     << setprecision(5) << fixed << prc << " - " << (tmr / 1000.0) << "s - " << steps << flush;
            }

            // Check for collisions
            auto canAdd = !any_of(parallel.begin(), parallel.end(), [&timeTable](const shared_ptr<Event> &ev) {
                return timeTable->hasCollision(ev->p_time.p_day, ev->p_time.p_from, ev->p_time.p_to);
            });

            if (!canAdd)
                continue;

            // Add parallel
            auto timeTableNew = make_shared<TimeTable>(*timeTable);
            for (const auto &ev : parallel)
                timeTableNew->addEvent(ev);

            // If the score is worse then skip
            if (timeTableNew->getScore() > p_bestScore)
                continue;

            // Is the table complete?
            if (flatPos == p_flatData.size() - 1) {
                if (timeTableNew->getScore() < p_bestScore) {
                    p_bestScore = timeTableNew->getScore();
                    p_tables.clear();
                }

                p_tables.push_back(timeTableNew);
            } else {
                this->solve(timeTableNew, flatPos + 1);
            }
        }
    }

    json dumpJson() const {
        auto jsonTables = vector<json>();

        for (const auto &table : p_tables)
            jsonTables.push_back(table->dumpJson());

        return jsonTables;
    }
};


int main() {
    // Handle SIGINT
    signal(SIGINT, sigintHandler);

    json j;
    try {
        cin >> j;
    } catch (error_t) {
        cout << "Json can not be parsed";
        return 1;
    }

    auto cacheEvents = Cache<Event>();
    auto data = buildData(j, cacheEvents);

    auto solver = Solver(data);
    solver.solve();

    cout << solver.dumpJson();

    return 0;
}
