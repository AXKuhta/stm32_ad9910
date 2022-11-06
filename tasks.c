#include <stddef.h>
#include <stdio.h>

#define MAX_TASKS 16
void (*task_list[MAX_TASKS])() = {0};
int tasks_pending = 0;

void add_task(void f()) {
	task_list[tasks_pending++ % MAX_TASKS] = f;
};

void run_all_tasks() {
    if (tasks_pending > MAX_TASKS) {
        printf("Task list overrun: %d tasks pending.\n", tasks_pending);
        while (1) {};
    }

	for (int i = 0; i < tasks_pending; i++) {
        if (task_list[i]) {
            task_list[i]();
            task_list[i] = NULL;
        }
    }

    tasks_pending = 0;
}
