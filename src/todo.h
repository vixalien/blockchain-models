#ifndef TODO_H
#define TODO_H

// Add a new task for the current user (returns 1 on success)
int add_task(const char *task);

// View all tasks for the current user
void view_tasks(void);

// Mark a task as complete (by task number from view_tasks)
int complete_task(int task_num);

// Verify the blockchain integrity
int verify_blockchain(void);

#endif
