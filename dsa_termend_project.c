/*
 * ============================================================================
 *   SMART HOSPITAL EMERGENCY & PATIENT TRIAGE SYSTEM
 *   Data Structures and Algorithms Lab Project (C Language)
 * ============================================================================
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#ifdef _WIN32
  #include <windows.h>
  #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
  #endif
#else
  #include <unistd.h>
#endif

#define MAX_DOCTORS 3
#define QUEUE_SIZE 50
#define NAME_SIZE 30
#define MAX_SEV 5

/* ----------------------------- Structures -------------------------------- */

struct Patient {
    int id;
    char name[NAME_SIZE];
    int age;
    int severity;
    int tokenNumber;
    int arrivalRound;
    int treatedRound;
    int waitRounds;
    int doctorId;
    int status;         /* 1=Awaiting Triage, 2=In Priority Line, 3=In Consultation, 4=Discharged, 5=Preempted */
    struct Patient *next;
};

struct PQNode {
    struct Patient *patient;
    struct PQNode *next;
};

struct Doctor {
    int id;
    struct Patient *current;
    int totalServed;
    int preemptCount;
};

/* --------------------------- Global State -------------------------------- */

struct Patient *head = NULL;
struct Patient *tail = NULL;

struct Patient *regQueue[QUEUE_SIZE];
int front = 0;
int rear = -1;
int regCount = 0;

struct PQNode *pqHead = NULL;

struct Doctor doctors[MAX_DOCTORS];
const char *sevNames[MAX_SEV + 1] = {"", "Minor", "Low", "Moderate", "Urgent", "Critical"};

int statTreated[MAX_SEV + 1] = {0};
long statWaitSum[MAX_SEV + 1] = {0};
int statWaitMax[MAX_SEV + 1] = {0};

int nextId = 1;
int tokenCounter = 0;
int currentRound = 1;
int totalTreated = 0;
long totalWaitRounds = 0;
int totalPreemptions = 0;

/* ========================================================================== */
/*                          UI SECTION START                                  */
/* ========================================================================== */

#define W 76

typedef struct {
    const char *dh, *dv, *dtl, *dtr, *dbl, *dbr, *dml, *dmr;
    const char *h, *v, *tl, *tr, *bl, *br;
    const char *full, *empty, *down, *right;
} Glyphs;

static const Glyphs GU = {
    "\xE2\x95\x90", "\xE2\x95\x91", "\xE2\x95\x94", "\xE2\x95\x97",
    "\xE2\x95\x9A", "\xE2\x95\x9D", "\xE2\x95\xA0", "\xE2\x95\xA3",
    "\xE2\x94\x80", "\xE2\x94\x82", "\xE2\x94\x8C", "\xE2\x94\x90",
    "\xE2\x94\x94", "\xE2\x94\x98",
    "\xE2\x96\x88", "\xE2\x96\x91", "\xE2\x96\xBC", "\xE2\x96\xBA"
};

static const Glyphs GA = {
    "=", "|", "+", "+", "+", "+", "+", "+",
    "-", "|", "+", "+", "+", "+",
    "#", ".", "v", ">"
};

static const Glyphs *G = &GU;
static int useColor = 0;
static int isTTY = 0;
static const char *bc = "";

static const char *cRST = "", *cBLD = "", *cGRY = "", *cRED = "", *cGRN = "",
                  *cYEL = "", *cBLU = "", *cMAG = "", *cCYN = "", *cORG = "", *cWHT = "";

static void setColors(int on) {
    useColor = on;
    cRST = on ? "\033[0m"  : "";   cBLD = on ? "\033[1m"  : "";
    cGRY = on ? "\033[90m" : "";   cRED = on ? "\033[91m" : "";
    cGRN = on ? "\033[92m" : "";   cYEL = on ? "\033[93m" : "";
    cBLU = on ? "\033[94m" : "";   cMAG = on ? "\033[95m" : "";
    cCYN = on ? "\033[96m" : "";   cWHT = on ? "\033[97m" : "";
    cORG = on ? "\033[38;5;208m" : "";
}

static int hasUtf8Locale(void) {
    const char *v[3];
    int i;
    v[0] = getenv("LC_ALL"); v[1] = getenv("LC_CTYPE"); v[2] = getenv("LANG");
    for (i = 0; i < 3; i++)
        if (v[i] && (strstr(v[i], "UTF-8") || strstr(v[i], "utf-8") ||
                     strstr(v[i], "UTF8")  || strstr(v[i], "utf8")))
            return 1;
    return 0;
}

static void initUI(int argc, char **argv) {
    int i, wantColor = 0, wantUnicode = 1, forceColor = 0, forceNoColor = 0;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        isTTY = 1;
        wantColor = SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
    wantUnicode = SetConsoleOutputCP(65001) != 0;
#else
    isTTY = isatty(1);
    wantColor = isTTY;
    wantUnicode = hasUtf8Locale();
#endif

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ascii") == 0)   wantUnicode = 0;
        if (strcmp(argv[i], "--nocolor") == 0) forceNoColor = 1;
        if (strcmp(argv[i], "--color") == 0)   forceColor = 1;
    }
    if (forceColor)   wantColor = 1;
    if (forceNoColor) wantColor = 0;

    G = wantUnicode ? &GU : &GA;
    setColors(wantColor);
}

static void clearScreen(void) {
    if (useColor) {
        printf("\033[2J\033[3J\033[H");
    } else if (isTTY) {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
}

static void rep(const char *s, int n) { while (n-- > 0) fputs(s, stdout); }

static int visLen(const char *s) {
    int n = 0;
    while (*s) {
        if (*s == '\033') {
            while (*s && *s != 'm') s++;
            if (*s) s++;
            continue;
        }
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
        s++;
    }
    return n;
}

/* Copy at most `width` visible characters of `name` into out (never splits a
   UTF-8 character). If pad is non-zero, pad with spaces up to `width`. */
static void fitName(char *out, size_t n, const char *name, int width, int pad) {
    size_t i = 0, o = 0, len, k;
    int chars = 0;
    unsigned char c;
    if (n == 0) return;
    while (name[i] && chars < width) {
        c = (unsigned char)name[i];
        len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
        for (k = 1; k < len; k++)
            if (name[i + k] == '\0') { len = k; break; }
        if (o + len >= n) break;
        for (k = 0; k < len; k++) out[o++] = name[i + k];
        i += len;
        chars++;
    }
    if (pad)
        while (chars < width && o + 1 < n) { out[o++] = ' '; chars++; }
    out[o] = '\0';
}

static void apf(char *dst, size_t n, const char *fmt, ...) {
    size_t len = strlen(dst);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dst + len, n - len, fmt, ap);
    va_end(ap);
}

static void boxTop(const char *col) {
    bc = col;
    printf("%s%s", col, G->dtl); rep(G->dh, W); printf("%s%s\n", G->dtr, cRST);
}
static void boxMid(void) {
    printf("%s%s", bc, G->dml); rep(G->dh, W); printf("%s%s\n", G->dmr, cRST);
}
static void boxBot(void) {
    printf("%s%s", bc, G->dbl); rep(G->dh, W); printf("%s%s\n", G->dbr, cRST);
}
static void boxRule(void) {
    printf("%s%s%s %s", bc, G->dv, cRST, cGRY);
    rep(G->h, W - 2);
    printf("%s %s%s%s\n", cRST, bc, G->dv, cRST);
}
static void boxRow(const char *text) {
    int pad = W - 1 - visLen(text);
    printf("%s%s%s %s%s", bc, G->dv, cRST, text, cRST);
    rep(" ", pad > 0 ? pad : 0);
    printf("%s%s%s\n", bc, G->dv, cRST);
}
static void row(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    boxRow(buf);
}
static void boxCenter(const char *text, const char *col) {
    int len = visLen(text), left = (W - len) / 2, right = W - len - left;
    if (left < 0) left = 0;
    if (right < 0) right = 0;
    printf("%s%s%s", bc, G->dv, cRST);
    rep(" ", left);
    printf("%s%s%s", col, text, cRST);
    rep(" ", right);
    printf("%s%s%s\n", bc, G->dv, cRST);
}

static void banner(const char *title, const char *col) {
    printf("\n");
    boxTop(col);
    boxCenter(title, cBLD);
    boxBot();
}

static void caption(const char *fmt, ...) {
    char b[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    printf("  %s%s%s\n\n", cGRY, b, cRST);
}

static void vsay(const char *col, const char *tag, const char *fmt, va_list ap) {
    char b[768];
    vsnprintf(b, sizeof b, fmt, ap);
    printf("  %s%s%s%s %s\n", cBLD, col, tag, cRST, b);
}
static void uiOk(const char *fmt, ...)    { va_list ap; va_start(ap, fmt); vsay(cGRN, "[  OK  ]", fmt, ap); va_end(ap); }
static void uiInfo(const char *fmt, ...)  { va_list ap; va_start(ap, fmt); vsay(cCYN, "[ INFO ]", fmt, ap); va_end(ap); }
static void uiWarn(const char *fmt, ...)  { va_list ap; va_start(ap, fmt); vsay(cYEL, "[ WARN ]", fmt, ap); va_end(ap); }
static void uiErr(const char *fmt, ...)   { va_list ap; va_start(ap, fmt); vsay(cRED, "[ FAIL ]", fmt, ap); va_end(ap); }
static void uiAlert(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vsay(cRED, "[ !!!! ]", fmt, ap); va_end(ap); }
static void uiMore(const char *fmt, ...) {
    char b[768];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    printf("           %s%s%s\n", cGRY, b, cRST);
}

static const char *sevCol(int s) {
    switch (s) {
        case 1: return cGRN;
        case 2: return cCYN;
        case 3: return cYEL;
        case 4: return cORG;
        default: return cRED;
    }
}

static const char *statusCol(int st) {
    switch (st) {
        case 1: return cYEL;
        case 2: return cCYN;
        case 3: return cGRN;
        case 4: return cGRY;
        default: return cMAG;
    }
}

static void sevBar(char *out, size_t n, int s) {
    int i;
    out[0] = '\0';
    apf(out, n, "%s", sevCol(s));
    for (i = 1; i <= 5; i++) apf(out, n, "%s", i <= s ? G->full : G->empty);
    apf(out, n, "%s", cRST);
}

static void sevBadge(char *out, size_t n, int s) {
    char bar[128];
    sevBar(bar, sizeof bar, s);
    snprintf(out, n, "%s %s%-8s%s", bar, sevCol(s), sevNames[s], cRST);
}

static void meter(char *out, size_t n, int val, int max, int width, const char *col) {
    int i, filled = (max > 0) ? (val * width) / max : 0;
    if (val > 0 && filled == 0) filled = 1;
    if (filled > width) filled = width;
    out[0] = '\0';
    apf(out, n, "%s", col);
    for (i = 0; i < filled; i++) apf(out, n, "%s", G->full);
    apf(out, n, "%s%s", cGRY, "");
    for (i = filled; i < width; i++) apf(out, n, "%s", G->empty);
    apf(out, n, "%s", cRST);
}

static void ticketRow(const char *text) {
    int pad = 36 - 1 - visLen(text);
    printf("  %s%s%s %s%s", cGRY, G->v, cRST, text, cRST);
    rep(" ", pad > 0 ? pad : 0);
    printf("%s%s%s\n", cGRY, G->v, cRST);
}
static void printTicket(const struct Patient *p, const char *heading) {
    char b[256], badge[160], nm[96];
    printf("  %s%s", cGRY, G->tl); rep(G->h, 36); printf("%s%s\n", G->tr, cRST);
    snprintf(b, sizeof b, "%s%s  TOKEN #%03d%s", cBLD, heading, p->tokenNumber, cRST);
    ticketRow(b);
    fitName(nm, sizeof nm, p->name, 18, 0);
    snprintf(b, sizeof b, "Patient : %s (P%03d)", nm, p->id);
    ticketRow(b);
    snprintf(b, sizeof b, "Age     : %d", p->age);
    ticketRow(b);
    sevBadge(badge, sizeof badge, p->severity);
    snprintf(b, sizeof b, "Severity: %s", badge);
    ticketRow(b);
    printf("  %s%s", cGRY, G->bl); rep(G->h, 36); printf("%s%s\n", G->br, cRST);
}

static void printSevLegend(void) {
    int s;
    printf("  %sSeverity scale:%s ", cGRY, cRST);
    for (s = 1; s <= MAX_SEV; s++)
        printf("%s%d=%s%s%s", sevCol(s), s, sevNames[s], cRST, s < MAX_SEV ? "  " : "\n");
    printf("\n");
}

static int readLine(char *buf, size_t n) {
    size_t len;
    if (!fgets(buf, (int)n, stdin)) return 0;
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
    if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
    return 1;
}

static void promptTag(const char *label) {
    printf("  %s%s%s %s%s%s : ", cYEL, G->right, cRST, cBLD, label, cRST);
    fflush(stdout);
}

static int parseInt(const char *s, int lo, int hi, int *out) {
    char *end;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;
    if (errno == ERANGE || v < lo || v > hi) return 0;
    *out = (int)v;
    return 1;
}

static int askInt(const char *label, int lo, int hi, int *out) {
    char line[128];
    for (;;) {
        promptTag(label);
        if (!readLine(line, sizeof line)) return 0;
        if (parseInt(line, lo, hi, out)) return 1;
        if (lo == INT_MIN && hi == INT_MAX)
            uiWarn("Please type a whole number (digits only).");
        else
            uiWarn("Please type a whole number between %d and %d.", lo, hi);
    }
}

static int askText(const char *label, char *out, size_t n) {
    char line[256], *s, *e;
    for (;;) {
        promptTag(label);
        if (!readLine(line, sizeof line)) return 0;
        s = line;
        while (isspace((unsigned char)*s)) s++;
        e = s + strlen(s);
        while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
        if (*s == '\0') { uiWarn("This field cannot be empty. Please try again."); continue; }
        strncpy(out, s, n - 1);
        out[n - 1] = '\0';
        return 1;
    }
}

static void pauseScreen(void) {
    char b[64];
    printf("\n  %s%s Press ENTER to return to the main menu...%s ", cGRY, G->right, cRST);
    fflush(stdout);
    readLine(b, sizeof b);
}

/* ========================================================================== */
/*                          UI SECTION END                                    */
/* ========================================================================== */

/* ========================================================================== */
/*                          CORE LOGIC START                                  */
/* ========================================================================== */

const char *getStatusName(int status) {
    if (status == 1) return "Awaiting Triage";
    if (status == 2) return "In Priority Line";
    if (status == 3) return "In Consultation";
    if (status == 4) return "Discharged";
    if (status == 5) return "Preempted/Paused";
    return "Unknown";
}

void initDoctors(void) {
    int i;
    for (i = 0; i < MAX_DOCTORS; i++) {
        doctors[i].id = i + 1;
        doctors[i].current = NULL;
        doctors[i].totalServed = 0;
        doctors[i].preemptCount = 0;
    }
}

int getFreeDoctorIndex(void) {
    int i;
    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current == NULL)
            return i;
    }
    return -1;
}

int countFreeDoctors(void) {
    int i, count = 0;
    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current == NULL)
            count++;
    }
    return count;
}

/* ----------------- Unit 4: Master Linked List Operations ----------------- */

struct Patient *createPatient(const char *name, int age, int severity) {
    struct Patient *p = (struct Patient *)malloc(sizeof(struct Patient));
    if (!p) {
        uiErr("Heap allocation failed.");
        return NULL;
    }
    p->id = nextId++;
    fitName(p->name, NAME_SIZE, name, NAME_SIZE - 1, 0);
    p->age = age;
    p->severity = severity;
    p->tokenNumber = ++tokenCounter;
    p->arrivalRound = currentRound;
    p->treatedRound = -1;
    p->waitRounds = -1;
    p->doctorId = 0;
    p->status = 1;
    p->next = NULL;

    if (head == NULL) {
        head = tail = p;
    } else {
        tail->next = p;
        tail = p;
    }
    return p;
}

struct Patient *findPatientById(int id) {
    struct Patient *curr = head;
    while (curr != NULL) {
        if (curr->id == id)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

/* ----------------- Unit 3: Circular Queue Operations -------------------- */

int isRegFull(void)  { return regCount == QUEUE_SIZE; }
int isRegEmpty(void) { return regCount == 0; }

int enqueueReg(struct Patient *p) {
    if (isRegFull()) return 0;
    rear = (rear + 1) % QUEUE_SIZE;
    regQueue[rear] = p;
    regCount++;
    return 1;
}

struct Patient *dequeueReg(void) {
    struct Patient *p;
    if (isRegEmpty()) return NULL;
    p = regQueue[front];
    front = (front + 1) % QUEUE_SIZE;
    regCount--;
    return p;
}

/* ----------------- Unit 4: Priority Queue Operations -------------------- */

int hasPriority(struct Patient *a, struct Patient *b) {
    if (a->severity != b->severity)
        return a->severity > b->severity;
    return a->tokenNumber < b->tokenNumber;
}

int pqInsert(struct Patient *p) {
    struct PQNode *node = (struct PQNode *)malloc(sizeof(struct PQNode));
    struct PQNode *curr, *prev = NULL;

    if (!node) {
        uiErr("Heap allocation failed while queuing a patient.");
        return 0;
    }
    node->patient = p;
    node->next = NULL;

    curr = pqHead;
    while (curr != NULL && !hasPriority(node->patient, curr->patient)) {
        prev = curr;
        curr = curr->next;
    }

    if (prev == NULL) {
        node->next = pqHead;
        pqHead = node;
    } else {
        node->next = curr;
        prev->next = node;
    }
    return 1;
}

struct Patient *pqPop(void) {
    struct PQNode *temp;
    struct Patient *p;

    if (pqHead == NULL) return NULL;

    temp = pqHead;
    p = temp->patient;
    pqHead = pqHead->next;
    free(temp);
    return p;
}

int getPQSize(void) {
    int count = 0;
    struct PQNode *curr = pqHead;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    return count;
}

int getPQRank(struct Patient *p) {
    int rank = 1;
    struct PQNode *curr = pqHead;
    while (curr != NULL) {
        if (curr->patient == p) return rank;
        rank++;
        curr = curr->next;
    }
    return 0;
}

/* ----------------------- Allocation & Preemption Logic ------------------- */

/* Puts patient p into doctor docIdx's cabin. Patient-level statistics are
   recorded only on the patient's FIRST consultation, so a patient who was
   preempted and later resumes is not counted twice.
   Returns how many earlier-token patients still waiting were bypassed. */
static int assignToDoctor(struct Patient *p, int docIdx) {
    int bypassed = 0;
    struct PQNode *curr;

    for (curr = pqHead; curr != NULL; curr = curr->next) {
        if (curr->patient->tokenNumber < p->tokenNumber)
            bypassed++;
    }

    p->status = 3;
    p->doctorId = doctors[docIdx].id;
    doctors[docIdx].current = p;

    if (p->treatedRound == -1) {
        p->treatedRound = currentRound;
        p->waitRounds = currentRound - p->arrivalRound;

        doctors[docIdx].totalServed++;
        totalTreated++;
        totalWaitRounds += p->waitRounds;
        statTreated[p->severity]++;
        statWaitSum[p->severity] += p->waitRounds;
        if (p->waitRounds > statWaitMax[p->severity])
            statWaitMax[p->severity] = p->waitRounds;
    }
    return bypassed;
}

int autoAssignDoctors(void) {
    int assigned = 0;

    while (pqHead != NULL) {
        int docIdx = getFreeDoctorIndex();
        struct Patient *p;
        int bypassed, resumed;
        char badge[160];

        if (docIdx == -1) break;

        p = pqPop();
        resumed = (p->status == 5);
        bypassed = assignToDoctor(p, docIdx);

        sevBadge(badge, sizeof badge, p->severity);
        printf("  %s%s[ASSIGN]%s Token #%03d (P%03d) %s%s%s  %s  Doctor %d\n",
               cBLD, cGRN, cRST, p->tokenNumber, p->id, cBLD, p->name, cRST,
               G->right, doctors[docIdx].id);
        uiMore("Severity: %s%s%s | Registered: Round %d | Rounds Waited: %d",
               "", badge, cGRY, p->arrivalRound, p->waitRounds);

        if (resumed)
            uiMore("* Resumed after being paused by an emergency.");
        if (bypassed > 0)
            uiMore("* Priority Rule: Bypassed %d earlier token(s) with lower severity.", bypassed);

        assigned++;
    }

    if (assigned == 0 && pqHead != NULL && countFreeDoctors() == 0) {
        uiWarn("All doctors are currently occupied with patients.");
    }

    return assigned;
}

int attemptPreemption(struct Patient *criticalPatient) {
    int i;
    int targetDoc = -1;
    int lowestSev = criticalPatient->severity;
    struct Patient *preemptedPatient;
    char b1[160], b2[160], title[160], n1[96], n2[96];

    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current != NULL) {
            if (doctors[i].current->severity < lowestSev) {
                lowestSev = doctors[i].current->severity;
                targetDoc = i;
            }
        }
    }

    if (targetDoc == -1) {
        return 0;
    }

    preemptedPatient = doctors[targetDoc].current;

    /* Re-queue the paused patient FIRST; if that fails, change nothing. */
    if (!pqInsert(preemptedPatient)) return 0;

    fitName(n1, sizeof n1, preemptedPatient->name, 22, 1);
    fitName(n2, sizeof n2, criticalPatient->name, 22, 1);
    sevBadge(b1, sizeof b1, preemptedPatient->severity);
    sevBadge(b2, sizeof b2, criticalPatient->severity);
    snprintf(title, sizeof title, "!!  ACUITY PREEMPTION TRIGGERED ON DOCTOR %d  !!", doctors[targetDoc].id);

    printf("\n");
    boxTop(cRED);
    boxCenter(title, cBLD);
    boxMid();
    row("%sPAUSED   %s Token #%03d  %s %s",
        cYEL, cRST, preemptedPatient->tokenNumber, n1, b1);
    row("%sEMERGENCY%s Token #%03d  %s %s",
        cRED, cRST, criticalPatient->tokenNumber, n2, b2);
    boxBot();

    preemptedPatient->status = 5;
    preemptedPatient->doctorId = 0;
    uiWarn("Token #%03d (%s) paused and returned to Priority Line at Rank #%d.",
           preemptedPatient->tokenNumber, preemptedPatient->name, getPQRank(preemptedPatient));

    assignToDoctor(criticalPatient, targetDoc);
    doctors[targetDoc].preemptCount++;
    totalPreemptions++;

    uiOk("Emergency Token #%03d (%s) immediately admitted to Doctor %d bed!",
         criticalPatient->tokenNumber, criticalPatient->name, doctors[targetDoc].id);

    return 1;
}

/* -------------------------- STAGE 1: Intake ------------------------------ */

void registerRoutinePatient(void) {
    char name[NAME_SIZE + 128];
    int age, severity;
    struct Patient *p;

    banner("STAGE 1  |  ROUTINE WALK-IN REGISTRATION", cCYN);
    caption("The patient receives a token and joins the Intake Circular Queue (FIFO).");

    if (isRegFull()) {
        uiWarn("Registration hall buffer is full. Triage waiting patients first.");
        return;
    }

    if (!askText("Patient Name", name, sizeof name)) return;
    if (strlen(name) > NAME_SIZE - 1)
        uiInfo("Name is long; it will be shortened to fit %d bytes.", NAME_SIZE - 1);
    if (!askInt("Age (0-120)", 0, 120, &age)) return;
    printf("\n");
    printSevLegend();
    if (!askInt("Severity (1-5)", 1, MAX_SEV, &severity)) return;

    p = createPatient(name, age, severity);
    if (p != NULL) {
        enqueueReg(p);
        printf("\n");
        printTicket(p, "REGISTERED");
        printf("\n");
        uiOk("Issued Token #%03d to %s.", p->tokenNumber, p->name);
        uiInfo("Seated at Position #%d in the Intake Waiting Queue.", regCount);
    }
}

void emergencyCodeRedBypass(void) {
    char name[NAME_SIZE + 128];
    int age;
    struct Patient *p;

    printf("\n");
    boxTop(cRED);
    boxCenter("!!  EMERGENCY CODE RED  -  AMBULANCE / TRAUMA FAST-TRACK  !!", cBLD);
    boxBot();
    caption("Code Red patients are always Severity 5 and skip the Intake Queue.");

    if (!askText("Critical Patient Name", name, sizeof name)) return;
    if (strlen(name) > NAME_SIZE - 1)
        uiInfo("Name is long; it will be shortened to fit %d bytes.", NAME_SIZE - 1);
    if (!askInt("Age (0-120)", 0, 120, &age)) return;

    p = createPatient(name, age, 5);
    if (p == NULL) return;

    printf("\n");
    printTicket(p, "CODE RED");
    printf("\n");
    uiAlert("CODE RED ISSUED: Token #%03d assigned to %s (Severity 5: Critical).",
            p->tokenNumber, p->name);
    uiInfo("Bypassed the Intake Circular Queue buffer completely.");

    if (countFreeDoctors() > 0) {
        if (!pqInsert(p)) return;
        p->status = 2;
        printf("\n");
        autoAssignDoctors();
        return;
    }

    uiAlert("All doctor cabins are occupied! Evaluating active bed preemption...");
    if (!attemptPreemption(p)) {
        if (!pqInsert(p)) return;
        p->status = 2;
        uiWarn("All doctors are currently handling Severity 5 patients. Cannot preempt.");
        uiMore("Token #%03d placed at Priority Line Rank #%d for the very next round.",
               p->tokenNumber, getPQRank(p));
    }
}

struct Patient *triageSingle(void) {
    struct Patient *p;
    if (isRegEmpty()) return NULL;

    p = regQueue[front];          /* peek: only dequeue once the insert worked */
    if (!pqInsert(p)) return NULL;
    dequeueReg();
    p->status = 2;
    return p;
}

void triageNext(void) {
    struct Patient *p;
    banner("STAGE 1  |  TRIAGE NEXT PATIENT", cCYN);
    caption("Front patient of the Intake Queue is dequeued and inserted into the Priority Queue.");
    if (isRegEmpty()) {
        uiWarn("Intake Waiting Queue is empty.");
        return;
    }
    p = triageSingle();
    if (p == NULL) {
        uiErr("Could not move the patient into the Priority Queue.");
        return;
    }

    uiOk("Triaged Token #%03d (%s) [Sev %d: %s].", p->tokenNumber, p->name, p->severity, sevNames[p->severity]);
    uiInfo("Assigned Priority Rank: %d of %d.", getPQRank(p), getPQSize());
    printf("\n");
    autoAssignDoctors();
}

void triageAll(void) {
    int count = 0;
    banner("STAGE 1  |  TRIAGE ALL REGISTERED PATIENTS", cCYN);
    caption("Every patient in the Intake Queue moves into the Priority Queue.");
    if (isRegEmpty()) {
        uiWarn("Intake Waiting Queue is empty.");
        return;
    }

    while (!isRegEmpty()) {
        if (triageSingle() == NULL) break;
        count++;
    }
    uiOk("Successfully triaged %d patient(s) into Treatment Priority Line.", count);
    printf("\n");
    autoAssignDoctors();
}

/* ------------------- STAGE 2: Round & Consultation Control ---------------- */

void runNextRound(void) {
    int i;
    char title[96];

    snprintf(title, sizeof title, "CONCLUDING CONSULTATION ROUND %d", currentRound);
    banner(title, cGRN);
    printf("\n");

    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current != NULL) {
            struct Patient *p = doctors[i].current;
            p->status = 4;
            printf("  %s%s[DISCHRG]%s Doctor %d completed consultation for Token #%03d (%s).\n",
                   cBLD, cGRN, cRST, doctors[i].id, p->tokenNumber, p->name);
            doctors[i].current = NULL;
        } else {
            printf("  %s[ IDLE ]  Doctor %d had no patient during Round %d.%s\n",
                   cGRY, doctors[i].id, currentRound, cRST);
        }
    }

    currentRound++;
    printf("\n  %s%s%s%s ADVANCING TO ROUND %d %s%s%s\n\n", cBLD, cYEL, G->right, G->right,
           currentRound, G->right, G->right, cRST);

    autoAssignDoctors();
}

void manualDischarge(void) {
    int docId;
    banner("STAGE 2  |  MANUAL CABIN DISCHARGE", cGRN);
    caption("Free one doctor early; the cabin becomes available immediately.");
    if (!askInt("Doctor Number (1-3)", INT_MIN, INT_MAX, &docId)) return;

    if (docId < 1 || docId > MAX_DOCTORS) {
        uiErr("Invalid doctor ID.");
        return;
    }

    docId--;
    if (doctors[docId].current == NULL) {
        uiInfo("Doctor %d is already free.", docId + 1);
        return;
    }

    uiOk("Manually discharged Token #%03d (%s) from Doctor %d.",
         doctors[docId].current->tokenNumber, doctors[docId].current->name, docId + 1);
    doctors[docId].current->status = 4;
    doctors[docId].current = NULL;

    printf("\n");
    uiInfo("Doctor %d is now free and can accept an infusion or wait for the next batch.", docId + 1);
}

void assignSingleCabin(void) {
    int docId;
    struct Patient *p;
    int bypassed;
    char badge[160];

    banner("STAGE 2  |  ASSIGN PATIENT TO A SINGLE CABIN (INFUSION)", cGRN);
    caption("Admit the top patient from the Priority Queue to a chosen available doctor.");

    if (pqHead == NULL) {
        uiWarn("Priority line is empty. There are no patients waiting for treatment.");
        return;
    }

    if (countFreeDoctors() == 0) {
        uiWarn("All doctor cabins are currently busy. Discharge a cabin first.");
        return;
    }

    if (!askInt("Doctor Number to Infuse (1-3)", 1, MAX_DOCTORS, &docId)) return;
    docId--;

    if (doctors[docId].current != NULL) {
        uiErr("Doctor %d is currently occupied with Token #%03d (%s). Choose an available doctor.",
              doctors[docId].id, doctors[docId].current->tokenNumber, doctors[docId].current->name);
        return;
    }

    p = pqPop();
    bypassed = assignToDoctor(p, docId);

    sevBadge(badge, sizeof badge, p->severity);
    printf("\n");
    uiOk("Infused Doctor %d with Token #%03d (%s).", doctors[docId].id, p->tokenNumber, p->name);
    uiMore("Acuity: %s | Arrival: Round %d | Rounds Waited: %d", badge, p->arrivalRound, p->waitRounds);

    if (bypassed > 0)
        uiMore("* Priority Rule: Bypassed %d earlier token(s) with lower severity.", bypassed);
}

/* -------------------------- STAGE 3: Live Monitors ----------------------- */

void showPriorityQueue(void) {
    struct PQNode *curr = pqHead;
    int rank = 1;
    long sumWait = 0;
    char badge[160];

    banner("STAGE 3  |  TREATMENT PRIORITY QUEUE", cYEL);
    caption("Sorted linked list: higher severity first, lower token number breaks ties.");
    if (pqHead == NULL) {
        uiInfo("Priority line is empty. No patients waiting for treatment.");
        return;
    }

    boxTop(cYEL);
    row("%s%-4s  %-5s  %-15s  %-3s  %-14s  %-3s  %-4s  %-9s%s", cBLD,
        "RANK", "TOKEN", "NAME", "AGE", "SEVERITY", "ARR", "WAIT", "STATUS", cRST);
    boxRule();
    while (curr != NULL) {
        struct Patient *p = curr->patient;
        int currentWait = currentRound - p->arrivalRound;
        int paused = (p->status == 5);
        char nm[64];
        fitName(nm, sizeof nm, p->name, 15, 1);
        sevBadge(badge, sizeof badge, p->severity);
        row("%-4d  #%03d   %s  %-3d  %s  R%-2d  %-4d  %s%-9s%s",
            rank, p->tokenNumber, nm, p->age, badge, p->arrivalRound, currentWait,
            paused ? cMAG : cGRN, paused ? "PAUSED" : "WAITING", cRST);
        sumWait += currentWait;
        rank++;
        curr = curr->next;
    }
    boxRule();
    row("Total Waiting: %s%d%s    Average Rounds Waited So Far: %s%.2f%s",
        cBLD, rank - 1, cRST, cBLD, (double)sumWait / (rank - 1), cRST);
    boxBot();
}

void showRegistrationQueue(void) {
    int i;
    char badge[160], idx[64];

    banner("STAGE 3  |  ROUTINE INTAKE QUEUE (CIRCULAR BUFFER)", cYEL);
    caption("First In, First Out. The patient at position 1 is triaged next.");
    if (isRegEmpty()) {
        uiInfo("Intake hall is empty. No routine patients awaiting triage.");
        return;
    }

    boxTop(cYEL);
    row("%s%-4s  %-5s  %-15s  %-3s  %-14s  %-3s  %-9s%s", cBLD,
        "POS", "TOKEN", "NAME", "AGE", "SEVERITY", "ARR", "MARKER", cRST);
    boxRule();
    for (i = 0; i < regCount; i++) {
        struct Patient *p = regQueue[(front + i) % QUEUE_SIZE];
        const char *mark = "";
        char nm[64];
        fitName(nm, sizeof nm, p->name, 15, 1);
        if (i == 0 && regCount == 1) mark = "FRONT/REAR";
        else if (i == 0) mark = "FRONT";
        else if (i == regCount - 1) mark = "REAR";
        sevBadge(badge, sizeof badge, p->severity);
        snprintf(idx, sizeof idx, "%s", mark);
        row("%-4d  #%03d   %s  %-3d  %s  R%-2d  %s%-10s%s",
            i + 1, p->tokenNumber, nm, p->age, badge, p->arrivalRound, cCYN, idx, cRST);
    }
    boxRule();
    row("In Waiting Hall: %s%d%s of %d slots   %s(front index = %d, rear index = %d)%s",
        cBLD, regCount, cRST, QUEUE_SIZE, cGRY, front, rear, cRST);
    boxBot();
}

void showDoctors(void) {
    int i;
    char badge[160], sevcell[200], tokcell[32];

    banner("STAGE 3  |  DOCTOR CONSULTATION CABINS", cYEL);
    caption("Doctors are stored in a struct array (Unit 1).");

    boxTop(cYEL);
    row("%s%-8s  %-12s  %-10s  %-14s  %-6s  %-9s%s", cBLD,
        "CABIN", "STATUS", "TOKEN", "SEVERITY", "SERVED", "PREEMPTED", cRST);
    boxRule();
    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current == NULL) {
            row("Doctor %d  %s%-12s%s  %-10s  %-14s  %-6d  %-9d",
                doctors[i].id, cGRN, "AVAILABLE", cRST, "-", "-",
                doctors[i].totalServed, doctors[i].preemptCount);
        } else {
            sevBadge(badge, sizeof badge, doctors[i].current->severity);
            snprintf(sevcell, sizeof sevcell, "%s", badge);
            snprintf(tokcell, sizeof tokcell, "#%03d", doctors[i].current->tokenNumber);
            row("Doctor %d  %s%-12s%s  %-10s  %s  %-6d  %-9d",
                doctors[i].id, cRED, "IN CONSULT", cRST, tokcell, sevcell,
                doctors[i].totalServed, doctors[i].preemptCount);
        }
    }
    boxRule();
    row("Available Cabins: %s%d%s of %d", cBLD, countFreeDoctors(), cRST, MAX_DOCTORS);
    boxBot();
}

void showVisualWorkflow(void) {
    int i, shown;
    struct PQNode *pCurr = pqHead;
    char title[96], line[1024], badge[160], nm[64];

    snprintf(title, sizeof title, "LIVE PATIENT MOVEMENT DIAGRAM  -  ROUND %d", currentRound);
    banner(title, cMAG);
    printf("\n");

    boxTop(cMAG);

    row("%s(1) ROUTINE INTAKE QUEUE  [FIFO]%s               Waiting: %s%d%s",
        cBLD, cRST, cBLD, regCount, cRST);
    line[0] = '\0';
    apf(line, sizeof line, "    FRONT %s ", G->right);
    if (isRegEmpty()) {
        apf(line, sizeof line, "%s[ EMPTY ]%s ", cGRY, cRST);
    } else {
        for (i = 0; i < regCount && i < 4; i++) {
            struct Patient *p = regQueue[(front + i) % QUEUE_SIZE];
            apf(line, sizeof line, "%s[#%03d S%d]%s %s ", sevCol(p->severity),
                p->tokenNumber, p->severity, cRST, G->right);
        }
        if (regCount > 4) apf(line, sizeof line, "[+%d more] %s ", regCount - 4, G->right);
    }
    apf(line, sizeof line, "REAR");
    boxRow(line);
    row("%s                         %s", cGRY, cRST);
    row("                     %s%s%s  Nurse triage checks routine cases", cYEL, G->down, cRST);
    row("");

    row("%s%s CODE RED / TRAUMA %s  bypasses intake and joins here (or preempts a bed)",
        cBLD, cRED, cRST);
    row("                     %s%s%s", cYEL, G->down, cRST);
    row("%s(2) PRIORITY QUEUE  [acuity sorted]%s            Waiting: %s%d%s",
        cBLD, cRST, cBLD, getPQSize(), cRST);
    line[0] = '\0';
    apf(line, sizeof line, "    HEAD %s ", G->right);
    if (pqHead == NULL) {
        apf(line, sizeof line, "%s[ NO PATIENTS IN LINE ]%s ", cGRY, cRST);
    } else {
        shown = 0;
        while (pCurr != NULL && shown < 3) {
            apf(line, sizeof line, "%s[#%03d S%d%s]%s %s ", sevCol(pCurr->patient->severity),
                pCurr->patient->tokenNumber, pCurr->patient->severity,
                (pCurr->patient->status == 5) ? "*" : "", cRST, G->right);
            pCurr = pCurr->next;
            shown++;
        }
        if (pCurr != NULL) apf(line, sizeof line, "[+%d more] %s ", getPQSize() - shown, G->right);
    }
    apf(line, sizeof line, "NULL");
    boxRow(line);
    row("    %s* = paused earlier by an emergency%s", cGRY, cRST);
    row("                     %s%s%s  Batch dispatch, single infusion, or preemption", cYEL, G->down, cRST);
    row("");

    row("%s(3) DOCTOR BEDS%s", cBLD, cRST);
    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current == NULL) {
            row("    Bed %d  Doctor %d  %s[ AVAILABLE / IDLE ]%s", i + 1, doctors[i].id, cGRN, cRST);
        } else {
            sevBadge(badge, sizeof badge, doctors[i].current->severity);
            fitName(nm, sizeof nm, doctors[i].current->name, 14, 1);
            row("    Bed %d  Doctor %d  %s[ BUSY ]%s #%03d %s %s",
                i + 1, doctors[i].id, cRED, cRST, doctors[i].current->tokenNumber,
                nm, badge);
        }
    }
    boxBot();
}

/* -------------------------- STAGE 4: Analytics --------------------------- */

void showAllPatients(void) {
    struct Patient *curr = head;
    char badge[160], waitBuf[16], docBuf[8], nm[64];
    int total = 0;

    banner("STAGE 4  |  MASTER PATIENT LOG", cMAG);
    caption("Permanent record kept in a singly linked list (append at tail, O(1)).");
    if (head == NULL) {
        uiInfo("No patient records found.");
        return;
    }

    boxTop(cMAG);
    row("%s%-4s %-5s %-13s %-3s %-14s %-16s %-3s %-6s%s", cBLD,
        "ID", "TOKEN", "NAME", "AGE", "SEVERITY", "STATUS", "DR", "WAIT", cRST);
    boxRule();
    while (curr != NULL) {
        if (curr->waitRounds >= 0) snprintf(waitBuf, sizeof(waitBuf), "%d rds", curr->waitRounds);
        else snprintf(waitBuf, sizeof(waitBuf), "-");

        if (curr->doctorId > 0) snprintf(docBuf, sizeof(docBuf), "D%d", curr->doctorId);
        else snprintf(docBuf, sizeof(docBuf), "-");

        sevBadge(badge, sizeof badge, curr->severity);
        fitName(nm, sizeof nm, curr->name, 13, 1);
        row("P%03d #%03d  %s %-3d %s %s%-16s%s %-3s %-6s",
            curr->id, curr->tokenNumber, nm, curr->age, badge,
            statusCol(curr->status), getStatusName(curr->status), cRST, docBuf, waitBuf);
        total++;
        curr = curr->next;
    }
    boxRule();
    row("Total records: %s%d%s", cBLD, total, cRST);
    boxBot();
}

void searchPatient(void) {
    int id;
    struct Patient *p;
    char badge[160];

    banner("STAGE 4  |  SEARCH RECORD BY PATIENT ID", cMAG);
    caption("Linear search through the master linked list.");
    if (!askInt("Patient ID (e.g. 3 for P003)", INT_MIN, INT_MAX, &id)) return;

    p = findPatientById(id);
    if (p == NULL) {
        uiErr("Record not found for Patient ID P%03d.", id);
        return;
    }

    sevBadge(badge, sizeof badge, p->severity);
    printf("\n");
    boxTop(cMAG);
    row("%sPATIENT SUMMARY  P%03d%s", cBLD, p->id, cRST);
    boxRule();
    row("%sToken Assigned   %s : #%03d", cGRY, cRST, p->tokenNumber);
    row("%sName             %s : %s", cGRY, cRST, p->name);
    row("%sAge              %s : %d", cGRY, cRST, p->age);
    row("%sSeverity         %s : %s", cGRY, cRST, badge);
    row("%sArrival Round    %s : Round %d", cGRY, cRST, p->arrivalRound);
    row("%sCurrent Status   %s : %s%s%s", cGRY, cRST, statusCol(p->status), getStatusName(p->status), cRST);
    if (p->treatedRound >= 0) {
        row("%sAssigned Doctor  %s : Doctor %d", cGRY, cRST, p->doctorId);
        row("%sConsultation Rd  %s : Round %d", cGRY, cRST, p->treatedRound);
        row("%sRounds Waited    %s : %d round(s)", cGRY, cRST, p->waitRounds);
    } else {
        row("%sRounds Waited    %s : %d round(s) %s(still waiting)%s", cGRY, cRST,
            currentRound - p->arrivalRound, cYEL, cRST);
    }
    boxBot();
}

void showStatistics(void) {
    int sev, i;
    int cTriage = 0, cPriority = 0, cConsult = 0, cDone = 0, maxWait = 0;
    long currentWaitSum = 0;
    struct Patient *curr = head;
    char m[200], badge[160], a[32], x[32];
    int maxServed = 0;

    while (curr != NULL) {
        if (curr->status == 1) cTriage++;
        else if (curr->status == 2 || curr->status == 5) cPriority++;
        else if (curr->status == 3) cConsult++;
        else if (curr->status == 4) cDone++;

        if (curr->status == 1 || curr->status == 2 || curr->status == 5) {
            int w = currentRound - curr->arrivalRound;
            currentWaitSum += w;
            if (w > maxWait) maxWait = w;
        }
        curr = curr->next;
    }

    banner("STAGE 4  |  HOSPITAL ROUND & QUEUE ANALYTICS", cMAG);
    printf("\n");

    boxTop(cMAG);
    row("%sOVERVIEW%s", cBLD, cRST);
    boxRule();
    row("%-30s %s%d%s", "Current System Round", cBLD, currentRound, cRST);
    row("%-30s %s%d%s", "Total Tokens Generated", cBLD, tokenCounter, cRST);
    meter(m, sizeof m, cTriage, tokenCounter, 20, cYEL);
    row("%-30s %-4d %s", "Awaiting Intake Triage", cTriage, m);
    meter(m, sizeof m, cPriority, tokenCounter, 20, cCYN);
    row("%-30s %-4d %s", "Waiting in Priority Line", cPriority, m);
    meter(m, sizeof m, cConsult, tokenCounter, 20, cGRN);
    row("%-30s %-4d %s", "Currently in Consultation", cConsult, m);
    meter(m, sizeof m, cDone, tokenCounter, 20, cGRY);
    row("%-30s %-4d %s", "Discharged / Completed", cDone, m);
    row("%-30s %s%d%s", "Total Preemption Swaps", totalPreemptions ? cRED : cBLD, totalPreemptions, cRST);
    row("%-30s %s%d of %d%s", "Free Doctor Cabins", cBLD, countFreeDoctors(), MAX_DOCTORS, cRST);
    boxBot();

    printf("\n");
    boxTop(cMAG);
    row("%sWAIT ANALYSIS (in rounds)%s", cBLD, cRST);
    boxRule();
    if (totalTreated > 0) {
        row("%-30s %s%.2f rounds%s  (total %ld across %d patients)", "Overall Avg Rounds to Doctor",
            cBLD, (double)totalWaitRounds / totalTreated, cRST, totalWaitRounds, totalTreated);
    } else {
        row("%-30s %sN/A (no consultations started yet)%s", "Overall Avg Rounds to Doctor", cGRY, cRST);
    }
    if (cTriage + cPriority > 0) {
        row("%-30s %s%.2f rounds%s  (longest wait: %d)", "Current Queue Avg Wait",
            cBLD, (double)currentWaitSum / (cTriage + cPriority), cRST, maxWait);
    }
    boxBot();

    printf("\n");
    boxTop(cMAG);
    row("%sROUNDS WAITED BY SEVERITY TIER%s", cBLD, cRST);
    boxRule();
    row("%s%-14s  %-8s  %-12s  %-12s%s", cBLD, "SEVERITY", "TREATED", "AVG WAIT", "MAX WAIT", cRST);
    for (sev = MAX_SEV; sev >= 1; sev--) {
        sevBadge(badge, sizeof badge, sev);
        if (statTreated[sev] > 0) {
            snprintf(a, sizeof a, "%.2f rds", (double)statWaitSum[sev] / statTreated[sev]);
            snprintf(x, sizeof x, "%d rds", statWaitMax[sev]);
            row("%s  %-8d  %-12s  %-12s", badge, statTreated[sev], a, x);
        } else {
            row("%s  %-8d  %-12s  %-12s", badge, 0, "-", "-");
        }
    }
    boxBot();

    for (i = 0; i < MAX_DOCTORS; i++)
        if (doctors[i].totalServed > maxServed) maxServed = doctors[i].totalServed;

    printf("\n");
    boxTop(cMAG);
    row("%sDOCTOR WORKLOAD & PREEMPTION HISTORY%s", cBLD, cRST);
    boxRule();
    row("%s%-8s  %-8s  %-10s  %-20s%s", cBLD, "DOCTOR", "SERVED", "PREEMPTED", "WORKLOAD", cRST);
    for (i = 0; i < MAX_DOCTORS; i++) {
        meter(m, sizeof m, doctors[i].totalServed, maxServed, 20, cCYN);
        row("Doctor %d  %-8d  %-10d  %s",
            doctors[i].id, doctors[i].totalServed, doctors[i].preemptCount, m);
    }
    boxBot();
}

/* -------------------------- Demo & Teardown ------------------------------ */

static void phase(const char *fmt, ...) {
    char b[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    printf("\n  %s%s%s%s %s %s\n\n", cBLD, cYEL, G->right, G->right, b, cRST);
}

static void demoLine(const char *tag, const char *col, const struct Patient *p, const char *tail) {
    char badge[160], nm[64];
    sevBadge(badge, sizeof badge, p->severity);
    fitName(nm, sizeof nm, p->name, 16, 1);
    printf("  %s%s%-14s%s Token #%03d  %s age %2d  %s  %s%s%s\n",
           cBLD, col, tag, cRST, p->tokenNumber, nm, p->age, badge, cGRY, tail, cRST);
}

void loadDemo(void) {
    int i, n;
    struct Patient *p;

    static const struct {
        const char *name;
        int age;
        int sev;
        int route;
    } demoData[15] = {
        {"Ravi Sharma",    45, 1, 0},
        {"Anita Desai",    30, 2, 0},
        {"Meena Iyer",      8, 3, 0},
        {"Karan Mehta",    62, 4, 0},
        {"Vikram Rao",     55, 3, 0},
        {"Pooja Hegde",    24, 2, 0},
        {"Sara Khan",      27, 5, 1},
        {"Amitabh Joshi",  70, 4, 0},
        {"Neha Verma",     19, 1, 0},
        {"Suresh Nair",    51, 5, 1},
        {"Deepak Patil",   38, 2, 0},
        {"Kavita Sen",     42, 3, 0},
        {"Rajesh Khanna",  66, 5, 1},
        {"Simran Kaur",    29, 1, 0},
        {"Manoj Bajpayee", 58, 4, 0}
    };

    banner("DEMO  |  BULK SCENARIO: 15 PATIENTS ACROSS ROUNDS", cBLU);

    if (head != NULL || currentRound != 1) {
        printf("\n");
        uiWarn("Bulk Demo requires a fresh start. Restart the program or ensure no records exist.");
        return;
    }

    phase("PHASE 1: Routine Intake Registration (Tokens 1-6)");
    for (i = 0; i < 6; i++) {
        p = createPatient(demoData[i].name, demoData[i].age, demoData[i].sev);
        if (p == NULL) return;
        enqueueReg(p);
        demoLine("[INTAKE QUEUE]", cCYN, p, "");
    }

    phase("PHASE 2: Triaging Routine Cases to Priority Queue & Filling Doctors");
    n = 0;
    while (!isRegEmpty()) {
        if (triageSingle() == NULL) break;
        n++;
    }
    uiInfo("Triaged %d patient(s) into the Priority Queue.", n);
    autoAssignDoctors();

    phase("PHASE 3: AMBULANCE ARRIVAL (Code Red Preemption Event)");
    p = createPatient(demoData[6].name, demoData[6].age, demoData[6].sev);
    if (p == NULL) return;
    uiAlert("CODE RED BYPASS: Token #%03d (%s) arrived via ambulance!", p->tokenNumber, p->name);
    if (!attemptPreemption(p)) {
        if (pqInsert(p)) p->status = 2;
        uiWarn("No lower-severity patient to preempt; Token #%03d queued in the Priority Line.",
               p->tokenNumber);
    }

    phase("PHASE 4: Advancing to Round 2 & Admitting Tokens 8-12");
    runNextRound();

    printf("\n");
    for (i = 7; i < 12; i++) {
        p = createPatient(demoData[i].name, demoData[i].age, demoData[i].sev);
        if (p == NULL) return;
        if (demoData[i].route == 1) {
            demoLine("[CODE RED]", cRED, p, "-> direct to Priority Queue");
            if (pqInsert(p)) p->status = 2;
        } else {
            enqueueReg(p);
            demoLine("[INTAKE QUEUE]", cCYN, p, "");
        }
    }
    n = 0;
    while (!isRegEmpty()) {
        if (triageSingle() == NULL) break;
        n++;
    }
    uiInfo("Triaged %d patient(s) into the Priority Queue.", n);
    autoAssignDoctors();

    phase("PHASE 5: Admitting Remaining Cases (Tokens 13-15)");
    for (i = 12; i < 15; i++) {
        p = createPatient(demoData[i].name, demoData[i].age, demoData[i].sev);
        if (p == NULL) return;
        if (demoData[i].route == 1) {
            demoLine("[CODE RED]", cRED, p, "-> direct to Priority Queue");
            if (pqInsert(p)) p->status = 2;
        } else {
            enqueueReg(p);
            demoLine("[INTAKE QUEUE]", cCYN, p, "-> triaged immediately");
            triageSingle();
        }
    }
    autoAssignDoctors();

    printf("\n");
    boxTop(cGRN);
    boxCenter("SUCCESS  -  15 DEMO ENTRIES INGESTED ACROSS 2 ROUNDS", cBLD);
    boxMid();
    row("Option 11 : Live Dual-Path Pipeline & Bed Allocation Map");
    row("Option 12 : All 15 master records in the Singly Linked List");
    row("Option 14 : Wait-round analytics across all 5 severity levels");
    boxBot();
}

void cleanup(void) {
    struct Patient *pCurr = head, *pNext;
    struct PQNode *qCurr = pqHead, *qNext;

    while (pCurr != NULL) {
        pNext = pCurr->next;
        free(pCurr);
        pCurr = pNext;
    }
    while (qCurr != NULL) {
        qNext = qCurr->next;
        free(qCurr);
        qCurr = qNext;
    }
    head = tail = NULL;
    pqHead = NULL;
}

/* ------------------------------- Menus ----------------------------------- */

static char menuMsg[160] = "";

static void menuItem(int num, const char *label, const char *desc, const char *labelCol) {
    row("%s%2d%s  %s%-30s%s %s%s%s", cBLD, num, cRST, labelCol, label, cRST, cGRY, desc, cRST);
}

static void stageHeader(const char *text, const char *col) {
    row("%s%s%s%s", cBLD, col, text, cRST);
}

static void showMenu(void) {
    int i;
    char strip[512] = "";

    clearScreen();
    printf("\n");
    boxTop(cCYN);
    boxCenter("SMART HOSPITAL EMERGENCY & PATIENT TRIAGE SYSTEM", cBLD);
    boxCenter("Round-Based Simulation  |  Arrays - Queues - Linked Lists", cGRY);
    boxMid();
    row("%sROUND%s %s%d%s    %sINTAKE QUEUE%s %s%d%s    %sPRIORITY QUEUE%s %s%d%s    %sFREE CABINS%s %s%d/%d%s",
        cGRY, cRST, cBLD, currentRound, cRST,
        cGRY, cRST, cBLD, regCount, cRST,
        cGRY, cRST, cBLD, getPQSize(), cRST,
        cGRY, cRST, cBLD, countFreeDoctors(), MAX_DOCTORS, cRST);
    for (i = 0; i < MAX_DOCTORS; i++) {
        if (doctors[i].current == NULL)
            apf(strip, sizeof strip, "%s[D%d FREE]%s  ", cGRN, doctors[i].id, cRST);
        else
            apf(strip, sizeof strip, "%s[D%d BUSY S%d]%s  ", sevCol(doctors[i].current->severity),
                doctors[i].id, doctors[i].current->severity, cRST);
    }
    row("%sCABINS%s  %s", cGRY, cRST, strip);
    boxMid();

    stageHeader("STAGE 1  |  DUAL-PATH INTAKE & TRIAGE", cCYN);
    menuItem(1, "Routine Walk-in Registration",  "joins the Intake Queue",        cWHT);
    menuItem(2, "EMERGENCY CODE RED (Trauma)",   "fast-track / preempt a bed",    cRED);
    menuItem(3, "Triage Next Routine Patient",   "Intake front -> Priority Line", cWHT);
    menuItem(4, "Triage ALL Routine Patients",   "batch-triage the whole Intake", cWHT);
    boxRule();

    stageHeader("STAGE 2  |  CONSULTATION & ROUND CONTROL", cGRN);
    menuItem(5, "Run Next Consultation Round",   "discharge all, admit next",     cWHT);
    menuItem(6, "Assign Patient to Single Cabin","infuse 1 doctor from queue",    cWHT);
    menuItem(7, "Discharge a Single Cabin",      "free one doctor early",         cWHT);
    boxRule();

    stageHeader("STAGE 3  |  LIVE SYSTEM MONITORS", cYEL);
    menuItem(8,  "View Treatment Priority Line", "acuity-ranked order, paused",   cWHT);
    menuItem(9,  "View Routine Intake Queue",    "FIFO registration hall",        cWHT);
    menuItem(10, "View Doctor Cabins",           "who is treating whom",          cWHT);
    menuItem(11, "View Patient Movement Map",    "dual-path pipeline + beds",     cWHT);
    boxRule();

    stageHeader("STAGE 4  |  AUDIT RECORDS & ANALYTICS", cMAG);
    menuItem(12, "View Master Patient Log",      "full linked-list history",      cWHT);
    menuItem(13, "Search Patient Record by ID",  "details + round tracking",      cWHT);
    menuItem(14, "View Efficiency Statistics",   "avg waits + preemptions",       cWHT);
    boxRule();

    stageHeader("SYSTEM UTILITY", cBLU);
    menuItem(15, "Load Demo Scenario",           "15 patients, preemption proof", cWHT);
    menuItem(0,  "System Shutdown",              "free all heap memory",          cWHT);
    boxBot();

    if (menuMsg[0]) {
        printf("\n  %s%s%s\n", cRED, menuMsg, cRST);
        menuMsg[0] = '\0';
    }
    printf("\n");
}

static void showSplash(void) {
    char b[64];
    int s;
    clearScreen();
    printf("\n");
    boxTop(cCYN);
    boxCenter("", cBLD);
    boxCenter("SMART HOSPITAL EMERGENCY & PATIENT TRIAGE SYSTEM", cBLD);
    boxCenter("Data Structures & Algorithms Lab Project  (C)", cGRY);
    boxCenter("", cBLD);
    boxMid();
    row("%sHOW IT WORKS%s", cBLD, cRST);
    row("  %s1.%s Routine patients wait in a %sCircular Queue%s (FIFO).", cYEL, cRST, cBLD, cRST);
    row("  %s2.%s A nurse triages them into a %sPriority Queue%s (sorted linked list).", cYEL, cRST, cBLD, cRST);
    row("  %s3.%s %sCode Red%s patients skip the intake queue entirely.", cYEL, cRST, cRED, cRST);
    row("  %s4.%s If all doctors are busy, a Code Red %spreempts%s the least severe case.", cYEL, cRST, cBLD, cRST);
    row("  %s5.%s Every patient is also stored in a permanent %sLinked List%s.", cYEL, cRST, cBLD, cRST);
    boxRule();
    row("%sSEVERITY COLOURS%s", cBLD, cRST);
    {
        char leg[1024], bar[128];
        leg[0] = '\0';
        for (s = 1; s <= MAX_SEV; s++) {
            sevBar(bar, sizeof bar, s);
            apf(leg, sizeof leg, "%s %s%s%s%s", bar, sevCol(s), sevNames[s], cRST, s < MAX_SEV ? "  " : "");
        }
        row("  %s", leg);
    }
    boxBot();
    printf("\n  %s%s Press ENTER to begin...%s ", cBLD, G->right, cRST);
    fflush(stdout);
    readLine(b, sizeof b);
}

/* ------------------------------- Main ------------------------------------ */

int main(int argc, char **argv) {
    int choice = -1;
    char line[128];

    initUI(argc, argv);
    initDoctors();
    showSplash();

    for (;;) {
        showMenu();
        promptTag("Enter your choice (0-15)");
        if (!readLine(line, sizeof line)) break;
        if (!parseInt(line, 0, 15, &choice)) {
            snprintf(menuMsg, sizeof menuMsg,
                     "[ FAIL ] Invalid selection. Please enter a number from 0 to 15.");
            continue;
        }

        clearScreen();
        switch (choice) {
            case 1:  registerRoutinePatient(); break;
            case 2:  emergencyCodeRedBypass(); break;
            case 3:  triageNext();             break;
            case 4:  triageAll();              break;
            case 5:  runNextRound();           break;
            case 6:  assignSingleCabin();      break;
            case 7:  manualDischarge();        break;
            case 8:  showPriorityQueue();      break;
            case 9:  showRegistrationQueue();  break;
            case 10: showDoctors();            break;
            case 11: showVisualWorkflow();     break;
            case 12: showAllPatients();        break;
            case 13: searchPatient();          break;
            case 14: showStatistics();         break;
            case 15: loadDemo();               break;
            case 0:  break;
        }
        if (choice == 0) break;
        pauseScreen();
    }

    clearScreen();
    banner("SYSTEM SHUTDOWN", cCYN);
    printf("\n");
    uiInfo("Shutting down hospital triage system. Releasing heap memory...");
    cleanup();
    uiOk("Shutdown complete. Memory deallocated.");
    printf("\n");
    return 0;
}

/* ========================================================================== */
/*                          CORE LOGIC END                                    */
/* ========================================================================== */

