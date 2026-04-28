#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<direct.h>
#include<unistd.h>
#include<dirent.h>
#include<gtk/gtk.h>
#include<windows.h>
#include<process.h>

#define MAX_PROC      100
#define QUANTUM       2
#define NUM_RESOURCES 3   

typedef struct {
    int pid;
    char name[50];
    int memory;
    int alive;
    HANDLE handle;
    HANDLE thread_handle;
} Process;

Process plist[MAX_PROC];
int pcount = 0;


int total_resources[NUM_RESOURCES]     = {10, 5, 7};
int available[NUM_RESOURCES]           = {10, 5, 7};  // starts fully free
int max_demand[MAX_PROC][NUM_RESOURCES];
int allocation[MAX_PROC][NUM_RESOURCES];
int need[MAX_PROC][NUM_RESOURCES];

GtkWidget *text_view;
char filename_global[100];


void save_file(GtkButton *button, gpointer user_data)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    FILE *file = fopen(filename_global, "w");
    if (file) {
        fprintf(file, "%s", text);
        fclose(file);
        printf("File saved successfully.\n");
    } else {
        printf("Error saving file.\n");
    }
}

static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window, *box, *button, *scroll;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    box       = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    text_view = gtk_text_view_new();

    gtk_widget_set_hexpand(text_view, TRUE);
    gtk_widget_set_vexpand(text_view, TRUE);

    scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), text_view);

    button = gtk_button_new_with_label("Save");
    g_signal_connect(button, "clicked", G_CALLBACK(save_file), NULL);

    gtk_box_append(GTK_BOX(box), scroll);
    gtk_box_append(GTK_BOX(box), button);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_widget_show(window);
}

int create(char *fname)
{
    if (strstr(fname, ".c") != NULL)
        strcpy(filename_global, fname);
    else
        sprintf(filename_global, "%s.c", fname);

    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.editor", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    return status;
}


void ls()
{
    struct dirent *d;
    DIR *dir = opendir(".");
    if (!dir) { printf("Cannot open directory\n"); return; }
    while ((d = readdir(dir)) != NULL)
        printf("%s\t", d->d_name);
    printf("\n");
    closedir(dir);
}

void cd_cmd(char *dirname)
{
    if (chdir(dirname) == 0)
        printf("Changed directory to '%s'\n", dirname);
    else
        printf("Error changing directory\n");
}

void mkdir_cmd(char *dirname)
{
    if (_mkdir(dirname) == 0)
        printf("Directory created\n");
    else
        printf("Error creating directory\n");
}

void cat(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
        char temp[100];
        sprintf(temp, "%s.c", filename);
        file = fopen(temp, "r");
    }
    if (!file) { printf("File not found\n"); return; }
    char ch;
    while ((ch = fgetc(file)) != EOF) putchar(ch);
    fclose(file);
}


void runp(char *filename)
{
    char cmd[200];
    char base[100];

    strcpy(base, filename);
    char *dot = strstr(base, ".c");
    if (dot) *dot = '\0';

    sprintf(cmd, "gcc %s.c -o %s.exe", base, base);
    if (system(cmd) != 0) { printf("Compilation failed\n"); return; }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    sprintf(cmd, "%s.exe", base);

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED,
                       NULL, NULL, &si, &pi))
    {
        printf("Process creation failed\n");
        return;
    }

    printf("Started %s with PID %ld (index %d)\n", base, pi.dwProcessId, pcount);

    plist[pcount].pid           = pi.dwProcessId;
    strcpy(plist[pcount].name, base);
    plist[pcount].memory        = rand() % 100 + 10;
    plist[pcount].alive         = 1;
    plist[pcount].handle        = pi.hProcess;
    plist[pcount].thread_handle = pi.hThread;

    for (int j = 0; j < NUM_RESOURCES; j++) {
        max_demand[pcount][j] = 0;
        allocation[pcount][j] = 0;
        need[pcount][j]       = 0;
    }

    pcount++;
}

void ps()
{
    printf("IDX\tPID\tNAME\t\tMEMORY\tSTATE\n");
    for (int i = 0; i < pcount; i++)
        printf("%d\t%d\t%-12s\t%dMB\t%s\n",
            i,
            plist[i].pid,
            plist[i].name,
            plist[i].memory,
            plist[i].alive ? "RUNNING" : "STOPPED");
}

static void cleanup_process(int idx)
{
    for (int j = 0; j < NUM_RESOURCES; j++) {
        available[j]      += allocation[idx][j];
        allocation[idx][j] = 0;
        need[idx][j]       = max_demand[idx][j];
    }

    plist[idx].alive = 0;

    if (plist[idx].handle != NULL) {
        CloseHandle(plist[idx].handle);
        plist[idx].handle = NULL;
    }
    if (plist[idx].thread_handle != NULL) {
        CloseHandle(plist[idx].thread_handle);
        plist[idx].thread_handle = NULL;
    }
}

DWORD WINAPI scheduler(LPVOID arg)
{
    printf("\n[Round Robin Scheduling Started]\n");
    int i = 0;

    while (1)
    {
        int active = 0;

        for (int j = 0; j < pcount; j++)
        {
            if (!plist[j].alive) continue;

            DWORD status;
            if (plist[j].handle != NULL &&
                GetExitCodeProcess(plist[j].handle, &status))
            {
                if (status != STILL_ACTIVE) {
                    printf("Process %s (PID %d) has exited.\n",
                        plist[j].name, plist[j].pid);
                    cleanup_process(j);
                } else {
                    active = 1;
                }
            }
        }

        if (!active) { printf("[Scheduler] No active processes.\n"); break; }

        if (!plist[i].alive) { i = (i + 1) % pcount; continue; }

        DWORD exitCode;
        if (plist[i].handle == NULL ||
            !GetExitCodeProcess(plist[i].handle, &exitCode) ||
            exitCode != STILL_ACTIVE)
        {
            cleanup_process(i);
            i = (i + 1) % pcount;
            continue;
        }

        printf("[Scheduler] Running: %s (PID %d) for %ds quantum\n",
            plist[i].name, plist[i].pid, QUANTUM);

        ResumeThread(plist[i].thread_handle);
        Sleep(QUANTUM * 1000);

        if (GetExitCodeProcess(plist[i].handle, &exitCode) &&
            exitCode == STILL_ACTIVE)
        {
            SuspendThread(plist[i].thread_handle);
        }
        else
        {
            printf("Process %s (PID %d) finished during its quantum.\n",
                plist[i].name, plist[i].pid);
            cleanup_process(i);
        }

        i = (i + 1) % pcount;
    }

    printf("[Scheduler] Done.\n");
    return 0;
}


int is_safe_state(int safe_seq[], int *seq_len)
{
    int work[NUM_RESOURCES];
    int finish[MAX_PROC];

    for (int j = 0; j < NUM_RESOURCES; j++)
        work[j] = available[j];

    for (int i = 0; i < pcount; i++)
        finish[i] = !plist[i].alive;

    *seq_len = 0;

    int progress = 1;
    while (progress)
    {
        progress = 0;
        for (int i = 0; i < pcount; i++)
        {
            if (finish[i]) continue;

            int can_run = 1;
            for (int j = 0; j < NUM_RESOURCES; j++) {
                if (need[i][j] > work[j]) { can_run = 0; break; }
            }

            if (can_run) {
                for (int j = 0; j < NUM_RESOURCES; j++)
                    work[j] += allocation[i][j];

                finish[i]             = 1;
                safe_seq[(*seq_len)++] = i;
                progress              = 1;
            }
        }
    }

    for (int i = 0; i < pcount; i++)
        if (!finish[i]) return 0;

    return 1;
}


void setmax_cmd(char *args)
{
    int idx, r[NUM_RESOURCES];
    if (sscanf(args, "%d %d %d %d", &idx, &r[0], &r[1], &r[2]) != 1 + NUM_RESOURCES) {
        printf("Usage: setmax <index> <r0> <r1> <r2>\n");
        return;
    }
    if (idx < 0 || idx >= pcount) {
        printf("Invalid process index. Use 'ps' to see valid indices.\n");
        return;
    }
    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (r[j] > total_resources[j]) {
            printf("Error: max for R%d (%d) exceeds total instances (%d)\n",
                j, r[j], total_resources[j]);
            return;
        }
        max_demand[idx][j] = r[j];
        need[idx][j]       = r[j] - allocation[idx][j];
    }
    printf("Max demand set for P%d (%s): R0=%d R1=%d R2=%d\n",
        idx, plist[idx].name, r[0], r[1], r[2]);
}


void request_cmd(char *args)
{
    int idx, req[NUM_RESOURCES];
    if (sscanf(args, "%d %d %d %d", &idx, &req[0], &req[1], &req[2]) != 1 + NUM_RESOURCES) {
        printf("Usage: request <index> <r0> <r1> <r2>\n");
        return;
    }
    if (idx < 0 || idx >= pcount || !plist[idx].alive) {
        printf("Invalid or dead process index.\n");
        return;
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (req[j] > need[idx][j]) {
            printf("DENIED: Request for R%d (%d) exceeds declared need (%d). "
                   "Process is claiming more than it said it would need.\n",
                j, req[j], need[idx][j]);
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (req[j] > available[j]) {
            printf("DENIED: R%d requested (%d) but only %d available. "
                   "Process %s must wait.\n",
                j, req[j], available[j], plist[idx].name);
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++) {
        available[j]       -= req[j];
        allocation[idx][j] += req[j];
        need[idx][j]       -= req[j];
    }

    int safe_seq[MAX_PROC], seq_len;
    if (is_safe_state(safe_seq, &seq_len))
    {
        printf("GRANTED: Resources allocated to P%d (%s).\n",
            idx, plist[idx].name);
        printf("Safe sequence: ");
        for (int k = 0; k < seq_len; k++)
            printf("P%d(%s)%s", safe_seq[k], plist[safe_seq[k]].name,
                   k < seq_len - 1 ? " -> " : "\n");
    }
    else
    {
        for (int j = 0; j < NUM_RESOURCES; j++) {
            available[j]       += req[j];
            allocation[idx][j] -= req[j];
            need[idx][j]       += req[j];
        }
        printf("DENIED: Granting this request would cause an UNSAFE STATE "
               "(potential deadlock). Request rolled back.\n");
    }
}


void release_cmd(char *args)
{
    int idx, rel[NUM_RESOURCES];
    if (sscanf(args, "%d %d %d %d", &idx, &rel[0], &rel[1], &rel[2]) != 1 + NUM_RESOURCES) {
        printf("Usage: release <index> <r0> <r1> <r2>\n");
        return;
    }
    if (idx < 0 || idx >= pcount) {
        printf("Invalid process index.\n");
        return;
    }
    for (int j = 0; j < NUM_RESOURCES; j++) {
        if (rel[j] > allocation[idx][j]) {
            printf("Error: trying to release more of R%d (%d) than held (%d).\n",
                j, rel[j], allocation[idx][j]);
            return;
        }
    }
    for (int j = 0; j < NUM_RESOURCES; j++) {
        allocation[idx][j] -= rel[j];
        available[j]       += rel[j];
        need[idx][j]       += rel[j];
    }
    printf("Released from P%d (%s). Available now: R0=%d R1=%d R2=%d\n",
        idx, plist[idx].name, available[0], available[1], available[2]);
}

void deadlock_cmd()
{
    printf("\n===================================================\n");
    printf("           RESOURCE ALLOCATION STATE\n");
    printf("===================================================\n");
    printf("Total     : R0=%-3d R1=%-3d R2=%-3d\n",
        total_resources[0], total_resources[1], total_resources[2]);
    printf("Available : R0=%-3d R1=%-3d R2=%-3d\n\n",
        available[0], available[1], available[2]);

    printf("%-4s %-12s  ALLOCATION       MAX              NEED\n",
        "IDX", "NAME");
    printf("---------------------------------------------------\n");

    for (int i = 0; i < pcount; i++) {
        printf("%-4d %-12s  R0=%-2d R1=%-2d R2=%-2d  "
                               "R0=%-2d R1=%-2d R2=%-2d  "
                               "R0=%-2d R1=%-2d R2=%-2d\n",
            i, plist[i].name,
            allocation[i][0],  allocation[i][1],  allocation[i][2],
            max_demand[i][0],  max_demand[i][1],  max_demand[i][2],
            need[i][0],        need[i][1],        need[i][2]);
    }

    int safe_seq[MAX_PROC], seq_len;
    printf("\n");
    if (is_safe_state(safe_seq, &seq_len)) {
        printf("[SAFE] System is in a SAFE STATE.\n");
        printf("Safe sequence: ");
        for (int k = 0; k < seq_len; k++)
            printf("P%d(%s)%s", safe_seq[k], plist[safe_seq[k]].name,
                   k < seq_len - 1 ? " -> " : "\n");
    } else {
        printf("[UNSAFE] System is in an UNSAFE STATE — deadlock possible!\n");
    }
    printf("===================================================\n\n");
}


void paging()
{
    int frames[3] = {-1, -1, -1};
    int pages[]   = {1, 2, 3, 1, 4, 2};
    int front = 0;

    printf("FIFO Paging:\n");
    for (int i = 0; i < 6; i++) {
        int found = 0;
        for (int j = 0; j < 3; j++) {
            if (frames[j] == pages[i]) {
                found = 1;
                printf("Page %d -> HIT\n", pages[i]);
                break;
            }
        }
        if (!found) {
            frames[front] = pages[i];
            front = (front + 1) % 3;
            printf("Page %d -> FAULT\n", pages[i]);
        }
    }
}
void remfil(char *args)
{
    remove(args)==0 ? printf("File removed successfully.\n") : printf("Error removing file.\n");
}

void remdir(char *args)
{
    rmdir(args)==0 ? printf("Directory removed successfully.\n") : printf("Error removing directory.\n");
}   



int main()
{
    mkdir("root");
    chdir("root");

    memset(max_demand, 0, sizeof(max_demand));
    memset(allocation, 0, sizeof(allocation));
    memset(need,       0, sizeof(need));

    char input[200];
    printf("Mini OS Simulator\n");
    printf("Type 'help' for a list of commands.\n\n");

    while (1)
    {
        printf(">>> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        char *cmd  = strtok(input, " ");
        char *rest = strtok(NULL, ""); 
        if (!cmd) continue;

        if (strcmp(cmd, "help") == 0)
            printf("Commands:\n"
                   "  whoami\n"
                   "  ls\n"
                   "  cd <dir>\n"
                   "  mkdir <dir>\n"
                   "  cat <file>\n"
                   "  nano <file>\n"
                   "  run <file>               compile and launch (suspended)\n"
                   "  ps                       list processes with index\n"
                   "  schedule                 start round-robin scheduler\n"
                   "  page                     FIFO page replacement demo\n"
                   "  setmax  <idx> <r0> <r1> <r2>  declare max resource need\n"
                   "  request <idx> <r0> <r1> <r2>  request resources (Banker's)\n"
                   "  release <idx> <r0> <r1> <r2>  release held resources\n"
                   "  deadlock                 show allocation table & safe state\n"
                   "  exit\n");

        else if (strcmp(cmd, "whoami")   == 0) printf("I am OS simulator\n");
        else if (strcmp(cmd, "ls")       == 0) ls();
        else if (strcmp(cmd, "cd")       == 0 && rest) cd_cmd(rest);
        else if (strcmp(cmd, "mkdir")    == 0 && rest) mkdir_cmd(rest);
        else if (strcmp(cmd, "cat")      == 0 && rest) cat(rest);
        else if (strcmp(cmd, "nano")     == 0 && rest) create(rest);
        else if (strcmp(cmd, "run")      == 0 && rest) runp(rest);
        else if (strcmp(cmd, "ps")       == 0) ps();
        else if (strcmp(cmd, "page")     == 0) paging();
        else if (strcmp(cmd, "setmax")   == 0 && rest) setmax_cmd(rest);
        else if (strcmp(cmd, "request")  == 0 && rest) request_cmd(rest);
        else if (strcmp(cmd, "release")  == 0 && rest) release_cmd(rest);
        else if (strcmp(cmd, "deadlock") == 0) deadlock_cmd();
        else if (strcmp(cmd, "rm") == 0) remfil(rest);
        else if (strcmp(cmd, "rmdir") == 0) remdir(rest);

        else if (strcmp(cmd, "schedule") == 0)
        {
            if (pcount == 0) {
                printf("No processes to schedule. Use 'run <file>' first.\n");
            } else {
                HANDLE hScheduler = CreateThread(NULL, 0, scheduler, NULL, 0, NULL);
                if (!hScheduler) {
                    printf("Failed to start scheduler thread.\n");
                } else {
                    printf("Scheduler running...\n");
                    WaitForSingleObject(hScheduler, INFINITE);
                    CloseHandle(hScheduler);
                    printf("Scheduling complete.\n");
                }
            }
        }

        else if (strcmp(cmd, "exit") == 0) break;
        else printf("Unknown command. Type 'help'.\n");
    }

    return 0;
}