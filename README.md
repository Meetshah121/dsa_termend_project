# Smart Hospital Emergency & Patient Triage System

A **Data Structures & Algorithms (C Language)** lab project that simulates a hospital's emergency department — from patient intake and triage to doctor allocation, preemption, discharge, and analytics.

Built as a round-based simulation to demonstrate **Arrays, Circular Queues, and Linked Lists** in a realistic, single-file C program.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Data Structures Used](#data-structures-used)
- [How It Works](#how-it-works)
- [Getting Started](#getting-started)
- [Menu Options](#menu-options)
- [Demo Scenario](#demo-scenario)
- [Project Structure](#project-structure)
- [Syllabus Alignment](#syllabus-alignment)
- [Command-Line Flags](#command-line-flags)
- [Author](#author)

---

## Overview

This project models a **Smart Hospital Emergency & Patient Triage System** where:

- **Routine walk-in patients** join a FIFO intake queue.
- A nurse **triages** them into a **priority queue** sorted by clinical severity.
- **Code Red / Trauma patients** bypass the intake queue entirely and are fast-tracked.
- If all doctors are busy, a critical patient can **preempt** (pause) the least severe patient in a consultation bed.
- Every patient is stored permanently in a **singly linked list** for audit and analytics.

The entire system runs as a **menu-driven console application** with a clean, colour-coded UI.

---

## Features

### 🚑 Dual-Path Intake
- **Routine Walk-ins** → Circular Queue (FIFO) → Triage → Priority Queue
- **Code Red / Trauma** → Bypasses intake → Priority Queue (Severity 5)

### ⚡ Acuity Preemption Engine
- When a Code Red arrives and all doctors are busy, the system scans the doctor array and **pauses the lowest-severity patient**, returning them to the priority queue.
- The emergency patient is immediately seated.

### 🛏️ Granular Bed Controls
- **Batch round progression** — discharge all patients and advance to the next round.
- **Single-cabin manual discharge** — free one doctor early.
- **Single-cabin targeted infusion** — assign the top priority patient to a specific doctor.

### 📊 Live Monitors & Analytics
- Priority queue viewer with ranks, wait times, and paused markers.
- Intake queue viewer showing front/rear positions.
- Doctor cabin status.
- Visual patient movement diagram (dual-path pipeline + beds).
- Master patient log (linked list).
- Search by patient ID.
- Round-based efficiency statistics (wait times per severity tier, doctor workload, preemptions).

### 🎨 Polished Console UI
- Box-drawing, colour-coded severity bars, badges, and meters.
- Automatic UTF-8 / ASCII fallback.
- Auto-detects terminal colour support.

---

## Data Structures Used

| Structure | Purpose | Unit |
|-----------|---------|------|
| **Struct Array** (`struct Doctor doctors[3]`) | Doctor resources with fixed capacity | Unit 1: Arrays |
| **Circular Queue** (`regQueue[50]`) | Routine walk-in intake (FIFO) | Unit 3: Queues |
| **Singly Linked List** (`struct Patient *head`) | Permanent master patient record (O(1) append) | Unit 4: Linked Lists |
| **Sorted Linked List (Priority Queue)** (`struct PQNode *pqHead`) | Acuity-ordered treatment line | Unit 4: Linked Lists |
| **Per-Severity Arrays** (`statTreated[6]`, etc.) | Statistics tracking | Unit 1: Arrays |

### Priority Rule
Patients are ordered by:
1. **Higher severity first** (5 → 1)
2. **Lower token number breaks ties** (FIFO within same severity)

---

## How It Works

```
┌─────────────────────┐
│  Routine Walk-in    │──────┐
└─────────────────────┘      │
                             ▼
                  ┌──────────────────────┐
                  │  Intake Circular     │
                  │  Queue (FIFO)        │
                  └──────────┬───────────┘
                             │  Nurse Triage
                             ▼
┌─────────────────────┐   ┌──────────────────────┐
│  Code Red / Trauma  │──▶│  Priority Queue      │
│  (Severity 5)       │   │  (Sorted Linked List)│
└─────────────────────┘   └──────────┬───────────┘
                                     │
                             ┌───────┴────────┐
                             │  Auto-Assign   │
                             │  or Preempt    │
                             ▼                ▼
                       ┌──────────┐    ┌──────────┐
                       │ Doctor 1 │    │ Doctor 2 │ ...
                       └──────────┘    └──────────┘
                             │
                             ▼
                  ┌──────────────────────┐
                  │  Discharge (Round)   │
                  └──────────────────────┘
```

Each "round" represents a consultation cycle:
1. All active consultations conclude → patients are **discharged**.
2. Round counter increments.
3. Next batch of waiting patients is **auto-assigned** to free doctors.

---

## Getting Started

### Prerequisites
- A C compiler (GCC, Clang, or MSVC)
- A terminal that supports ANSI colours (optional — falls back gracefully)

### Compile

```bash
gcc -O2 -o triage triage.c
```

### Run

```bash
./triage
```

On Windows (MSVC):
```bash
cl triage.c
triage.exe
```

### Command-Line Flags

| Flag | Effect |
|------|--------|
| `--ascii` | Force ASCII box-drawing characters (no Unicode) |
| `--nocolor` | Disable ANSI colours |
| `--color` | Force ANSI colours even if not a TTY |

Example:
```bash
./triage --ascii --nocolor
```

---

## Menu Options

### Stage 1 — Dual-Path Intake & Triage
| # | Action |
|---|--------|
| 1 | Routine Walk-in Registration → joins Intake Queue |
| 2 | EMERGENCY CODE RED (Trauma) → fast-track / preempt a bed |
| 3 | Triage Next Routine Patient → Intake front → Priority Line |
| 4 | Triage ALL Routine Patients → batch-triage the whole Intake |

### Stage 2 — Consultation & Round Control
| # | Action |
|---|--------|
| 5 | Run Next Consultation Round → discharge all, admit next |
| 6 | Assign Patient to Single Cabin → infuse 1 doctor from queue |
| 7 | Discharge a Single Cabin → free one doctor early |

### Stage 3 — Live System Monitors
| # | Action |
|---|--------|
| 8 | View Treatment Priority Line |
| 9 | View Routine Intake Queue |
| 10 | View Doctor Cabins |
| 11 | View Patient Movement Map |

### Stage 4 — Audit Records & Analytics
| # | Action |
|---|--------|
| 12 | View Master Patient Log |
| 13 | Search Patient Record by ID |
| 14 | View Efficiency Statistics |

### System Utility
| # | Action |
|---|--------|
| 15 | Load Demo Scenario (15 patients, preemption proof) |
| 0 | System Shutdown (frees all heap memory) |

---

## Demo Scenario

Option **15** loads a scripted, 15-patient scenario across 2 rounds that demonstrates:

- **Phase 1** — 6 routine walk-ins register into the intake queue.
- **Phase 2** — All 6 are triaged; doctors fill up.
- **Phase 3** — A **Code Red** (Sara Khan, Severity 5) arrives via ambulance → triggers **preemption** of the lowest-severity patient.
- **Phase 4** — Round advances; 5 more patients arrive (including another Code Red).
- **Phase 5** — Final 3 patients admitted.

After the demo, explore options **11**, **12**, and **14** to see the pipeline, patient log, and analytics.

---

## Project Structure

```
triage.c
├── Structures (Patient, PQNode, Doctor)
├── Global State (queues, lists, doctors, stats)
├── ===== UI SECTION START =====
│   ├── Terminal detection & colour setup
│   ├── Box drawing helpers
│   ├── Severity bars & meters
│   ├── Safe input (readLine, askInt, askText)
│   └── (all display-only functions)
├── ===== UI SECTION END =====
├── ===== CORE LOGIC START =====
│   ├── Linked list ops (createPatient, findPatientById)
│   ├── Circular queue ops (enqueueReg, dequeueReg)
│   ├── Priority queue ops (pqInsert, pqPop, getPQRank)
│   ├── Allocation & preemption (autoAssignDoctors, attemptPreemption)
│   ├── Stage 1: Intake & triage
│   ├── Stage 2: Round & consultation control
│   ├── Stage 3: Live monitors
│   ├── Stage 4: Analytics
│   ├── Demo loader & cleanup
│   ├── Menu rendering
│   └── main()
└── ===== CORE LOGIC END =====
```

The UI is kept **strictly separated** at the top so the DSA logic is easy to locate and study.

---

## Syllabus Alignment

| Unit | Topic | Implementation |
|------|-------|----------------|
| **Unit 1** | Arrays | `doctors[3]` struct array; per-severity statistics arrays |
| **Unit 2** | Searching & Sorting | Linear search (`findPatientById`); sorted insertion in priority queue |
| **Unit 3** | Queues | Circular intake queue with modulo arithmetic (`front`, `rear`, `regCount`) |
| **Unit 4** | Linked Lists | Singly linked master patient list (O(1) tail append); sorted linked priority queue |



This project is submitted as academic coursework. Feel free to reference it for learning purposes.
