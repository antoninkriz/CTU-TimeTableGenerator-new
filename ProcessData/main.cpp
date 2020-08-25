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

/// Lower score of classes starting at 7:30 or earlier - https://www.730ne.cz
static bool NE_730 = false;

/// NE730 but for classes before 9:15 AM
static bool NE_915 = false;

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
    /// Cache storage
    unordered_map<T, shared_ptr<T>> p_data;

public:
    /// Get an item from and set to a cache
    /// \param t Item to be cached and retrieved
    /// \return Shared pointer to a cached item
    shared_ptr<T> cache(T t) {
        if (p_data.count(t))
            return p_data[t];

        auto ptr = shared_ptr<T>(new T(t));
        p_data[t] = ptr;
        return ptr;
    }
};

/// Class Type
enum class Type {
    Lecture,
    Tutorial,
    Laboratory
};

/// String to a Type
/// \param s String to be parsed
/// \return Type form a string or an exception
Type parseType(const string &s) {
    if (s == "lecture")
        return Type::Lecture;
    else if (s == "tutorial")
        return Type::Tutorial;
    else if (s == "laboratory")
        return Type::Laboratory;
    throw invalid_argument("Not a Type value");
}

/// Type to a string
/// \param t Type to be converted
/// \return Type as a string or an exception
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

/// Odd or even weeks
enum class Week {
    Odd,
    Even
};

/// String to a Week
/// \param s String to be parsed
/// \return Week form a string or an exception
Week parseWeek(const string &s) {
    if (s == "odd")
        return Week::Odd;
    else if (s == "even")
        return Week::Even;
    throw invalid_argument("Not a Week value");
}

/// Week to a string
/// \param t Week to be converted
/// \return Week as a string or an exception
string weekToString(Week w) {
    switch (w) {
        case Week::Odd:
            return "odd";
        case Week::Even:
            return "even";
    }

    throw invalid_argument("Not a Week value");
}

/// Time class
class Time {
public:
    /// Hours
    int8_t p_hours;

    /// Minutes
    int8_t p_minutes;

    /// Compare time equality
    /// \param t Time to be compared to
    /// \return True when equal, false otherwise
    bool operator==(const Time &t) const {
        return p_hours == t.p_hours && p_minutes == t.p_minutes;
    }

    /// Is this time sooner?
    /// \param t  Time to compare
    /// \return True when sooner, false otherwise
    bool operator<(const Time &t) const {
        return p_hours < t.p_hours || (p_hours == t.p_hours && p_minutes < t.p_minutes);
    }

    /// Is this time later?
    /// \param t  Time to compare
    /// \return True when later, false otherwise
    bool operator>(const Time &t) const {
        return p_hours > t.p_hours || (p_hours == t.p_hours && p_minutes > t.p_minutes);
    }

    /// Is this time equal or sooner?
    /// \param t  Time to compare
    /// \return True when equal or sooner, false otherwise
    bool operator<=(const Time &t) const {
        return *this == t || *this < t;
    }

    /// Is this time equal or later?
    /// \param t  Time to compare
    /// \return True when equal or later, false otherwise
    bool operator>=(const Time &t) const {
        return *this == t || *this > t;
    }

    /// Get times difference in minutes
    /// \param t Time to compare
    /// \return
    [[nodiscard]] int diffMinutes(const Time &t) const {
        int minutes1 = p_hours * 60 + p_minutes;
        int minutes2 = t.p_hours * 60 + t.p_minutes;

        return abs(minutes1 - minutes2);
    }

    /// Convert a time to JSON
    /// \return Time as a JSON
    [[nodiscard]] json dumpJson() const {
        return {
                {"hours",   p_hours},
                {"minutes", p_minutes}
        };
    }
};

/// Parse string to a time
/// \param s String to be parsed
/// \return Time or an exception
Time parseTime(const string &s) {
    auto is = stringstream(s);

    char c = '\0';
    int h = -1, m = -1;
    is >> h >> c >> m;
    if (c != ':' || h <= 0 || m < 0)
        throw invalid_argument("Invalid time format");
    return {(int8_t) h, (int8_t) m};
}

/// Time of an event
class EventTime {
public:
    /// Is this event in an even or an odd week
    Week p_week;

    /// Day number (0 = Monday, 4 = Friday, 5 = Monday, 9 = Friday)
    int8_t p_day;

    /// Time when this event starts
    Time p_from;

    /// Time when this event ends
    Time p_to;

    /// Is an event time equal with this one?
    /// \param et EventTime to compare
    /// \return True when equal, false otherwise
    bool operator==(const EventTime &et) const {
        return p_week == et.p_week
               && p_day == et.p_day
               && p_from == et.p_from
               && p_to == et.p_to;
    }

    /// Convert en event time to JSON
    /// \return Event time as a JSON
    [[nodiscard]] json dumpJson() const {
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
    /// Name of the event
    string p_name;

    /// Parallel name
    string p_parallel;

    /// Type of this event
    Type p_type;

    /// Time of this event
    EventTime p_time;

    /// Is an event equal with this one?
    /// \param e Event to compare
    /// \return True when equal, false otherwise
    bool operator==(const Event &e) const {
        return p_name == e.p_name
               && p_parallel == e.p_parallel
               && p_type == e.p_type
               && p_time == e.p_time;
    }

    /// Convert en event to JSON
    /// \return Event as a JSON
    [[nodiscard]] json dumpJson() const {
        return {
                {"name",     p_name},
                {"parallel", p_parallel},
                {"type",     typeToString(p_type)},
                {"time",     p_time.dumpJson()}
        };
    }
};

/// Time table class
class TimeTable {
private:
    /// Score of this time table
    double p_score;

    /// Days holding lists of events
    vector<vector<shared_ptr<Event>>> p_days;

public:
    /// Create an empty time table
    TimeTable() {
        p_score = -1;
        p_days = vector<vector<shared_ptr<Event>>>(10);
    }

    /// Would there be a collision for a given time frame?
    /// \param day Day to read from
    /// \param from Beginning of the time frame
    /// \param to End of the time frame
    /// \return True when collision occurs, false when not
    [[nodiscard]] bool hasCollision(short day, Time from, Time to) const {
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

    /// Add a new event to the time table.
    /// Check for collision before using this method - no collision checks happen!
    /// \param ev Event to be added
    void addEvent(const shared_ptr<Event> &ev) {
        auto &day = p_days[ev->p_time.p_day];

        auto pos = lower_bound(day.begin(), day.end(), ev, [](const shared_ptr<Event> &a, const shared_ptr<Event> &b) {
            return a->p_time.p_from < b->p_time.p_from || (a->p_time.p_from == b->p_time.p_from && a->p_time.p_to < b->p_time.p_to);
        });

        day.insert(pos, ev);
        p_score = -1;
    }

    /// Get (and silently set) a score for this time table
    /// \return Score of this time table
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

            // Check for classes before and at 7:30 AM
            if (NE_730 && day[0]->p_time.p_from <= Time{7, 30})
                totalScore += 5;

            // Check for classes before and at 9:15 AM
            if (NE_915 && day[0]->p_time.p_from <= Time{9, 15})
                totalScore += 1;

            // Total time spent
            totalMinutesSpent += day[0]->p_time.p_from.diffMinutes(day[day.size() - 1]->p_time.p_to);
            totalDaysInSchool++;
        }

        // Add total time in school score
        totalScore += (totalMinutesSpent / 60.0) * 10 * totalDaysInSchool;

        return p_score = totalScore;
    }

    /// Convert a time table to JSON
    /// \return Time table as a JSON
    [[nodiscard]] json dumpJson() const {
        auto jsonEvents = vector<vector<json>>(p_days.size());

        for (size_t i = 0; i < p_days.size(); i++)
            for (const auto &event : p_days[i])
                jsonEvents[i].push_back(event->dumpJson());

        return {
                {"score", p_score},
                {"days", jsonEvents}
        };
    }
};

namespace std {
    /// Add hashing for the Time class
    template<>
    struct hash<Time> {
        /// Create a hash for the Time class
        /// \param e Time to get the hash for
        /// \return Hash of the Time class
        size_t operator()(const Time &t) const {
            size_t seed = 0;
            hash_combine(seed, hash_value(t.p_hours));
            hash_combine(seed, hash_value(t.p_minutes));
            return seed;
        }
    };

    /// Add hashing for the EventTime class
    template<>
    struct hash<EventTime> {
        /// Create a hash for the EventTime class
        /// \param e EventTime to get the hash for
        /// \return Hash of the EventTime class
        size_t operator()(const EventTime &et) const {
            size_t seed = 0;
            hash_combine(seed, hash_value(et.p_week));
            hash_combine(seed, hash_value(et.p_day));
            hash_combine(seed, hash<Time>()(et.p_from));
            hash_combine(seed, hash<Time>()(et.p_to));
            return seed;
        }
    };

    /// Add hashing for the Event class
    template<>
    struct hash<Event> {
        /// Create a hash for the Event class
        /// \param e Event to get the hash for
        /// \return Hash of the Event class
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

/// Build data from a json object
/// \param j Json to read from
/// \param cacheEvents Cache to be used
/// \return Parsed data object
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

/// Solver class
class Solver {
private:
    /// Best score (so far)
    double p_bestScore;

    /// Best time tables (so far)
    vector<shared_ptr<TimeTable>> p_tables;

    /// Flattened data
    vector<Parallels> p_flatData;

public:
    explicit Solver(const Data &data) {
        p_bestScore = numeric_limits<double>::max();

        for (const auto &[subject, types] : data) {
            for (const auto &[type, parallels] : types) {
                if (parallels.empty())
                    continue;

                p_flatData.push_back(parallels);
            }
        }
    }

    /// Recursively solve time tables
    /// \param timeTable Time table to currently solve
    /// \param flatPos Position in the flattened data
    void solve(const shared_ptr<TimeTable> &timeTable = make_shared<TimeTable>(), size_t flatPos = 0) {
        for (const auto &parallel : p_flatData[flatPos]) {
            // Kill recursion when SIGINT comes
            if (!keepRunning) break;

            static auto start = high_resolution_clock::now();
            static auto steps = 0;
            steps++;

            // Check for collisions
            auto canAdd = !any_of(parallel.begin(), parallel.end(), [&timeTable](const shared_ptr<Event> &ev) {
                return timeTable->hasCollision(ev->p_time.p_day, ev->p_time.p_from, ev->p_time.p_to);
            });

            // Collision, skip!
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

    /// Convert all best time tables to JSON
    /// \return All best time tables as a JSON
    [[nodiscard]] json dumpJson() const {
        auto jsonTables = vector<json>();

        for (const auto &table : p_tables)
            jsonTables.push_back(table->dumpJson());

        return jsonTables;
    }
};


int main(int argc, char *argv[]) {
    // Handle SIGINT
    signal(SIGINT, sigintHandler);

    // Parse args
    auto args = vector<string>(argv + 1, argv + argc);
    for (const auto &arg : args) {
        if (arg == "ne730") {
            NE_730 = true;
        } else if (arg == "ne915") {
            NE_915 = true;
        } else {
            cout << "Unknown parameter!" << endl;
            return 1;
        }
    }

    // Parse JSON from STDIN
    json j;
    try {
        cin >> j;
    } catch (error_t) {
        cout << "Json can not be parsed" << endl;
        return 1;
    }

    // Build data and cache
    auto cacheEvents = Cache<Event>();
    auto data = buildData(j, cacheEvents);

    // Solve time tables
    auto solver = Solver(data);
    solver.solve();

    cout << solver.dumpJson();

    return 0;
}

