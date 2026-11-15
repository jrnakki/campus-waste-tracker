// Campus Waste Tracker - An OOP-Based Waste Recording System
// Mini Project, Object-Oriented Programming (C++17, standard library only)
//
// OOP concepts used:
//   Abstraction   - WasteRecord declares the pure virtual disposalAdvice()
//   Inheritance   - RecyclableWaste, OrganicWaste, GeneralWaste derive from WasteRecord
//   Polymorphism  - each subclass overrides disposalAdvice()
//   Encapsulation - WasteRecord keeps its data private
//   Composition   - WasteTracker contains the WasteRecord objects

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace util {

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string today() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", local);
    return buf;
}

// Parses a weight. Returns true only for a fully numeric, finite, positive value.
bool parseWeight(const std::string& text, double& out) {
    std::string t = trim(text);
    if (t.empty()) return false;
    char* end = nullptr;
    double value = std::strtod(t.c_str(), &end);
    if (end == t.c_str() || *end != '\0') return false;  // not a number
    if (!std::isfinite(value) || value <= 0.0) return false;
    out = value;
    return true;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

}  // namespace util

// ---------------------------------------------------------------------------
// Abstract base class (Abstraction + Encapsulation)
// ---------------------------------------------------------------------------
class WasteRecord {
private:
    std::string location_;
    std::string category_;
    std::string date_;
    double weightKg_;

public:
    WasteRecord(std::string location, std::string category, std::string date, double weightKg)
        : location_(std::move(location)),
          category_(std::move(category)),
          date_(std::move(date)),
          weightKg_(weightKg) {}

    virtual ~WasteRecord() = default;

    // Every waste record must give advice, but the advice depends on the category.
    virtual std::string disposalAdvice() const = 0;

    const std::string& location() const { return location_; }
    const std::string& category() const { return category_; }
    const std::string& date() const { return date_; }
    double weightKg() const { return weightKg_; }
};

// ---------------------------------------------------------------------------
// Subclasses (Inheritance + Polymorphism)
// ---------------------------------------------------------------------------
class RecyclableWaste : public WasteRecord {
public:
    RecyclableWaste(std::string location, std::string date, double weightKg)
        : WasteRecord(std::move(location), "Recyclable", std::move(date), weightKg) {}

    std::string disposalAdvice() const override {
        return "Keep clean and dry, and place it in the recycling bin (paper, plastic, metal).";
    }
};

class OrganicWaste : public WasteRecord {
public:
    OrganicWaste(std::string location, std::string date, double weightKg)
        : WasteRecord(std::move(location), "Organic", std::move(date), weightKg) {}

    std::string disposalAdvice() const override {
        return "Collect separately for composting (food scraps, leaves).";
    }
};

class GeneralWaste : public WasteRecord {
public:
    GeneralWaste(std::string location, std::string date, double weightKg)
        : WasteRecord(std::move(location), "General", std::move(date), weightKg) {}

    std::string disposalAdvice() const override {
        return "Place in the general waste bin; try to reduce mixed waste where possible.";
    }
};

// ---------------------------------------------------------------------------
// Container class (Composition): holds records, reports and file storage
// ---------------------------------------------------------------------------
class WasteTracker {
private:
    std::vector<std::unique_ptr<WasteRecord>> records_;

    // Creates the right subclass for a category name; returns nullptr if unknown.
    static std::unique_ptr<WasteRecord> createRecord(const std::string& category,
                                                     const std::string& location,
                                                     const std::string& date,
                                                     double weightKg) {
        std::string c = util::toLower(util::trim(category));
        if (c == "recyclable" || c == "1") return std::make_unique<RecyclableWaste>(location, date, weightKg);
        if (c == "organic" || c == "2")    return std::make_unique<OrganicWaste>(location, date, weightKg);
        if (c == "general" || c == "3")    return std::make_unique<GeneralWaste>(location, date, weightKg);
        return nullptr;
    }

    // --- tiny JSON reading helpers for our fixed record format ---
    static bool readString(const std::string& obj, const std::string& key, std::string& out) {
        std::size_t k = obj.find("\"" + key + "\"");
        if (k == std::string::npos) return false;
        std::size_t colon = obj.find(':', k);
        if (colon == std::string::npos) return false;
        std::size_t q = obj.find('"', colon);
        if (q == std::string::npos) return false;
        std::string value;
        for (std::size_t i = q + 1; i < obj.size(); ++i) {
            if (obj[i] == '\\' && i + 1 < obj.size()) {
                char n = obj[++i];
                value += (n == 'n') ? '\n' : (n == 't') ? '\t' : n;
            } else if (obj[i] == '"') {
                out = value;
                return true;
            } else {
                value += obj[i];
            }
        }
        return false;
    }

    static bool readNumber(const std::string& obj, const std::string& key, double& out) {
        std::size_t k = obj.find("\"" + key + "\"");
        if (k == std::string::npos) return false;
        std::size_t colon = obj.find(':', k);
        if (colon == std::string::npos) return false;
        const char* start = obj.c_str() + colon + 1;
        char* end = nullptr;
        double v = std::strtod(start, &end);
        if (end == start) return false;
        out = v;
        return true;
    }

public:
    // Returns an empty string on success, otherwise the reason for rejection.
    std::string addRecord(const std::string& location, const std::string& category,
                          const std::string& weightText) {
        std::string loc = util::trim(location);
        if (loc.empty()) return "Location cannot be blank.";

        double weight = 0.0;
        if (!util::parseWeight(weightText, weight))
            return "Weight must be a number greater than zero.";

        auto record = createRecord(category, loc, util::today(), weight);
        if (!record) return "Unknown category. Choose Recyclable, Organic or General.";

        records_.push_back(std::move(record));
        return "";
    }

    std::size_t count() const { return records_.size(); }

    void viewRecords() const {
        if (records_.empty()) {
            std::cout << "\nNo records yet.\n";
            return;
        }
        std::cout << "\n" << std::left << std::setw(4) << "No." << std::setw(14) << "Location"
                  << std::setw(13) << "Category" << std::setw(12) << "Date"
                  << std::right << std::setw(10) << "Weight(kg)" << "\n";
        std::cout << std::string(53, '-') << "\n";
        std::cout << std::fixed << std::setprecision(2);
        for (std::size_t i = 0; i < records_.size(); ++i) {
            const WasteRecord& r = *records_[i];
            std::cout << std::left << std::setw(4) << (i + 1) << std::setw(14) << r.location()
                      << std::setw(13) << r.category() << std::setw(12) << r.date()
                      << std::right << std::setw(10) << r.weightKg() << "\n";
        }
    }

    void showSummary() const {
        double total = 0, recyclable = 0, organic = 0, general = 0;
        for (const auto& r : records_) {
            total += r->weightKg();
            if (r->category() == "Recyclable")   recyclable += r->weightKg();
            else if (r->category() == "Organic") organic += r->weightKg();
            else                                 general += r->weightKg();
        }
        double share = (total > 0.0) ? (recyclable / total) * 100.0 : 0.0;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n===== Summary =====\n";
        std::cout << "Records: " << records_.size() << "\n";
        std::cout << "Total waste: " << total << " kg\n";
        std::cout << "Recyclable: " << recyclable << " kg\n";
        std::cout << "Organic: " << organic << " kg\n";
        std::cout << "General: " << general << " kg\n";
        std::cout << "Recyclable share: " << share << "%\n";
        std::cout << "Classification does not confirm actual recycling.\n";

        // Polymorphism: same call, category-specific advice. Show one line per category.
        bool seenR = false, seenO = false, seenG = false;
        std::cout << "\nCategory advice:\n";
        for (const auto& r : records_) {
            bool* seen = (r->category() == "Recyclable") ? &seenR
                       : (r->category() == "Organic")    ? &seenO : &seenG;
            if (*seen) continue;
            *seen = true;
            std::cout << " - " << r->category() << ": " << r->disposalAdvice() << "\n";
        }
        if (records_.empty()) std::cout << " (no records)\n";
    }

    bool save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;
        out << std::fixed << std::setprecision(2);
        out << "[\n";
        for (std::size_t i = 0; i < records_.size(); ++i) {
            const WasteRecord& r = *records_[i];
            out << "  {\"location\": \"" << util::jsonEscape(r.location())
                << "\", \"category\": \"" << r.category()
                << "\", \"date\": \"" << r.date()
                << "\", \"weight\": " << r.weightKg() << "}"
                << (i + 1 < records_.size() ? "," : "") << "\n";
        }
        out << "]\n";
        return static_cast<bool>(out);
    }

    // Returns number of records loaded (0 if the file is missing or empty).
    std::size_t load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return 0;
        std::stringstream buffer;
        buffer << in.rdbuf();
        const std::string text = buffer.str();

        records_.clear();
        std::size_t pos = 0;
        while ((pos = text.find('{', pos)) != std::string::npos) {
            // find the matching '}' while ignoring braces inside strings
            bool inString = false;
            std::size_t end = pos;
            for (; end < text.size(); ++end) {
                char c = text[end];
                if (c == '\\' && inString) { ++end; continue; }
                if (c == '"') inString = !inString;
                else if (c == '}' && !inString) break;
            }
            if (end >= text.size()) break;

            std::string obj = text.substr(pos, end - pos + 1);
            std::string location, category, date;
            double weight = 0.0;
            if (readString(obj, "location", location) && readString(obj, "category", category) &&
                readString(obj, "date", date) && readNumber(obj, "weight", weight) && weight > 0.0) {
                auto record = createRecord(category, location, date, weight);
                if (record) records_.push_back(std::move(record));
            }
            pos = end + 1;
        }
        return records_.size();
    }
};

// ---------------------------------------------------------------------------
// Console menu
// ---------------------------------------------------------------------------
static void printMenu() {
    std::cout << "\n===== Campus Waste Tracker =====\n"
              << "1. Add waste entry\n"
              << "2. View records\n"
              << "3. Show summary\n"
              << "4. Save and exit\n"
              << "Choose an option: ";
}

int main() {
    const std::string dataFile = "waste_data.json";
    WasteTracker tracker;

    std::size_t loaded = tracker.load(dataFile);
    if (loaded > 0) std::cout << "Loaded " << loaded << " saved record(s).\n";

    std::string choice;
    while (true) {
        printMenu();
        if (!std::getline(std::cin, choice)) choice = "4";  // end of input: save and exit
        choice = util::trim(choice);

        if (choice == "1") {
            std::string location, category, weight;
            std::cout << "Location (e.g. Classroom): ";
            std::getline(std::cin, location);
            std::cout << "Category (1 = Recyclable, 2 = Organic, 3 = General): ";
            std::getline(std::cin, category);
            std::cout << "Weight in kg: ";
            std::getline(std::cin, weight);

            std::string error = tracker.addRecord(location, category, weight);
            if (error.empty()) std::cout << "Entry added.\n";
            else               std::cout << "Entry rejected: " << error << "\n";
        } else if (choice == "2") {
            tracker.viewRecords();
        } else if (choice == "3") {
            tracker.showSummary();
        } else if (choice == "4") {
            if (tracker.save(dataFile)) std::cout << "Saved " << tracker.count() << " record(s) to " << dataFile << ". Goodbye!\n";
            else                        std::cout << "Could not save to " << dataFile << ".\n";
            break;
        } else {
            std::cout << "Invalid option. Please enter 1-4.\n";
        }
    }
    return 0;
}
