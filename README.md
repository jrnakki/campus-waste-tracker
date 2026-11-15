# Campus Waste Tracker

An OOP-based waste recording system: a small console application written in C++ (C++17, standard library only) for recording ordinary, non-hazardous campus waste and summarising the entries.

**Subject:** Object-Oriented Programming | **Course:** Information Technology, 3rd Semester
**Institute:** Maharaja Agrasen Institute of Technology, Delhi | **Academic Year:** 2026-27

**Team:** Nakshatra, Atishya, Pushkar

## About the project

Campus waste is often collected without a simple record of its type or quantity, which makes it hard to see how much could be separated for recycling or composting. Campus Waste Tracker lets a user enter the location, category and weight of waste, view saved entries and generate a summary. Records are stored in a JSON file so they are available in later sessions.

This project supports awareness of responsible waste handling under **SDG 12: Responsible Consumption and Production** (Target 12.5).

## Features

- Add entries with location, category (Recyclable / Organic / General), date and weight in kg
- View all saved records in a table
- Summary with total weight, per-category totals and the recyclable share (%)
- Category-specific disposal advice
- Save to and reload from `waste_data.json`
- Input validation: blank locations and invalid, zero or negative weights are rejected

## OOP design

| Concept | Where it appears |
|---|---|
| Abstraction | `WasteRecord` is an abstract class that declares the pure virtual method `disposalAdvice()` |
| Inheritance | `RecyclableWaste`, `OrganicWaste` and `GeneralWaste` derive from `WasteRecord` |
| Polymorphism | Each subclass overrides `disposalAdvice()`, so one call gives category-specific advice |
| Encapsulation | `WasteRecord` keeps location, category, date and weight private, exposed through getters |
| Composition | `WasteTracker` contains the `WasteRecord` objects and handles reports and file storage |

```
WasteRecord (abstract)
├── RecyclableWaste
├── OrganicWaste
└── GeneralWaste

WasteTracker  ◆── contains ──> WasteRecord objects
```

## Requirements

- A C++17 compiler (g++ 7+ , clang++ 5+, or MSVC 2017+)
- No external libraries

## Build and run

```bash
g++ -std=c++17 -Wall -Wextra -o waste_tracker main.cpp
./waste_tracker
```

On Windows (MinGW):

```bash
g++ -std=c++17 -o waste_tracker.exe main.cpp
waste_tracker.exe
```

## Usage

```
===== Campus Waste Tracker =====
1. Add waste entry
2. View records
3. Show summary
4. Save and exit
```

Choose `4` to save and exit. Data is stored in `waste_data.json` in the folder you run the program from, and is loaded automatically next time.

## Sample run

Simulated inputs: Classroom / Recyclable / 2.5 kg, Canteen / Organic / 3 kg, Corridor / General / 1.5 kg.

```
===== Summary =====
Records: 3
Total waste: 7.00 kg
Recyclable: 2.50 kg
Organic: 3.00 kg
General: 1.50 kg
Recyclable share: 35.71%
Classification does not confirm actual recycling.
```

## Testing

Functional checks were done with simulated data:

- Sample entries gave the correct totals and category percentages
- Save and reload preserved the summary
- An empty record list gave a zero total
- Invalid or non-positive weights and blank locations were rejected

These are functional checks, not field measurements.

## Limitations

- Results come from simulated sample entries, not field data
- Waste is entered manually; nothing is measured automatically
- Classification as recyclable does not confirm actual recycling
- No prediction model is used
- Three categories of ordinary, non-hazardous waste only

## Future scope

- Edit and delete options
- Date-wise reports
- A graphical interface
- A record of actual recycling or composting

## Reference

- United Nations, Sustainable Development Goal 12: https://sdgs.un.org/goals/goal12
