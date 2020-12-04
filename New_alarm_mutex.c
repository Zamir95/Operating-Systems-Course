/*
 * New_alarm_mutex.c
 *
 * Authors:
 * Eli Frungorts - 215659501
 * Zamir Lalji - 212779997
 * AmirHossein Razavi - 216715963
 * Muhammed mujahid - 213731435
 */
#include <pthread.h>
#include <time.h>
#include <regex.h>
#include "errors.h"

#define DEBUG


typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 id;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    int                 owner;
    char                message[64];
} alarm_t;

typedef struct thread_tag {
    struct thread_tag    *link;
    char                 id;
    pthread_t           *thread;
    alarm_t             *first;
    alarm_t             *second;
} thread_t;


pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;
thread_t *thread_list = NULL;

void insert_a(alarm_t * alarm){
    /*
     * This will simply insert a new alarm into the list sorting it
     * in order of increasing id
     */
    alarm_t **current, *next;
    int status;
    status = pthread_mutex_lock (&alarm_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    current = &alarm_list;
    next = *current;
    while (next != NULL) {
        if (next->id >= alarm->id) {
            alarm->link = next;
            *current = alarm;
            break;
        }
        current = &next->link;
        next = next->link;
    }
    if (next == NULL) {
        *current = alarm;
        alarm->link = NULL;
    }
    status = pthread_mutex_unlock (&alarm_mutex);
    if (status != 0)
         err_abort (status, "Unlock mutex");
}

alarm_t * search_a(int id){
    /*
     * given an id, this will search the list for an alarm, if an alarm is
     * not found it will return null
     */
    alarm_t **current, *next;
    int status;
    status = pthread_mutex_lock (&alarm_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    current = &alarm_list;
    next = *current;
    while (next != NULL) {
        if ((*current)->id == id) {
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
            return *current;
        }
        current = &next->link;
        next = next->link;
    }
    status = pthread_mutex_unlock (&alarm_mutex);
    if (status != 0)
         err_abort (status, "Unlock mutex");
    return NULL;
}

void remove_a(int id){
    /*
     * This code will remove an alarm from the alarm list, if
     * an alarm with given id does not exist, do nothing
     */
    alarm_t *prev, *current ,*next;
    int status;
    status = pthread_mutex_lock (&alarm_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");

    current = alarm_list;
    next = current->link;
    if(current->id == id){
        alarm_list = next;
        free(current);
    } else {
        while (next != NULL) {
            prev = current;
            current = next;
            next = current->link;
            if (current->id == id) {
                prev->link = next;
                free(current);
                break;
            }
        }   
    }
    status = pthread_mutex_unlock (&alarm_mutex);
    if (status != 0)
         err_abort (status, "Unlock mutex");

}

int alarms_count(){
    /*
     * this will simply return the total number of alarms in the list
     */
    int ret = 0;
    int status;
    alarm_t **last, *next;
    status = pthread_mutex_lock(&alarm_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    last = &alarm_list;
    next = *last;
    while (next != NULL) {
        ret++;
        last = &next->link;
        next = next->link;
    }
    status = pthread_mutex_unlock(&alarm_mutex);
    if (status != 0)
        err_abort (status, "Unlock mutex");
    return ret;
}

void append_thread(thread_t * thread){
    /*
     * This will append a new thread to the list
     */
    int status;
    status = pthread_mutex_lock (&thread_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    if(thread_list == NULL){
        thread_list = thread;
    } else {
        thread_t *current = thread_list;
        while(current->link != NULL){
            current = current->link;
        }
        current->link = thread;
    }
    status = pthread_mutex_unlock (&thread_mutex);
    if (status != 0)
         err_abort (status, "Unlock mutex");
}

void remove_thread(int id){
    /*
     * This code will remove a thread from the thread list
     */
    thread_t *prev, *current ,*next;
    int status;
    status = pthread_mutex_lock (&thread_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");

    current = thread_list;
    next = current->link;
    if(current->id == id){
        thread_list = next;
        free(current);
    } else {
        while (next != NULL) {
            prev = current;
            current = next;
            next = current->link;
            if (current->id == id) {
                prev->link = next;
                free(current);
                break;
            }
        }   
    }
    status = pthread_mutex_unlock (&thread_mutex);
    if (status != 0)
         err_abort (status, "Unlock mutex");

}

int thread_count(){
    /*
     * this will simply return the total number of alarms in the list
     */
    int ret = 0;
    int status;
    thread_t **last, *next;
    status = pthread_mutex_lock(&thread_mutex);
    if (status != 0)
        err_abort (status, "Lock mutex");
    last = &thread_list;
    next = *last;
    while (next != NULL) {
        ret++;
        last = &next->link;
        next = next->link;
    }
    status = pthread_mutex_unlock(&thread_mutex);
    if (status != 0)
        err_abort (status, "Unlock mutex");
    return ret;
}

void *alarm_thread (void *arg){
    // method wide variable declarations
    alarm_t **current, *next, *first, *second;
    int first_id, second_id;
    int now, status;
    int counter = 0;
    thread_t *thread = arg;
    // infinity loop
    while(1){
        // sleep for 1 second, give the main thread time to access mutex
        sleep(1);
        if(counter >= 5){
            counter = -1;
        }
        counter++;
        // does the current thread have an alarm assigned to it?
        if(thread->first == NULL){ // NO
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");

            // Search for an unassigned alarm
            current = &alarm_list;
            next = *current;
            while (next != NULL) {
                if (next->owner == NULL) {
                    thread->first = next;
                    next->owner = thread->id;
                    first_id = thread->first->id;
#ifdef DEBUG
                    printf(
                        "Display Thread %d Assigned Alarm(%d) at %d: %d %s\n",
                        thread->id,
                        next->id,
                        time(NULL),
                        next->time - time(NULL),
                        next->message
                    );
#endif
                    break;
                }
                current = &next->link;
                next = next->link;
            }
            // unlock mutex
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
   
         } else { // YES, alarm is assigned
            now = time(NULL);
            //check if the main thread deleted the alarm
            first = search_a(first_id);
            //is the alarm deleted?
            if(first != NULL){ // NO
                if (first->time <= now){
                    // raise alarm and purge from list
                    printf ("\r(%d) %s\n", first->seconds, first->message);
                    remove_a(first->id); // thread safe; mutex is inside method
                    thread->first = NULL;
                }else if(counter >= 5){
                    // printf(
                    //     "Alarm %d printed by Alarm Display Thread %d at %d: %d %s\n",
                    //     first->id,
                    //     thread->id,
                    //     time(NULL),
                    //     next->time - time(NULL),
                    //     next->message
                    // );
                }
            } else { // Yes, alarm has been deleted from master list
                // set to NULL, let next iteration to assign it
#ifdef DEBUG
                printf(
                    "Display Alarm Thread %d Removed Alarm %d at %d: %d %s)\n",
                    thread->id,
                    second->id,
                    time(NULL),
                    next->time - time(NULL),
                    next->message
                );
#endif
                thread->first = NULL;
            }
        }
        // SAME LOGIC AS FIRST, wont bother documenting behaviour
        if(thread->second == NULL){
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
       
            current = &alarm_list;
            next = *current;
            while (next != NULL) {
                if (next->owner == NULL) {
                    thread->second = next;
                    next->owner = thread->id;
                    second_id = thread->second->id;
#ifdef DEBUG
                    printf(
                        "Display Thread %d Assigned Alarm(%d) at %d: %d %s\n",
                        thread->id,
                        next->id,
                        time(NULL),
                        next->time - time(NULL),
                        next->message
                    );
#endif
                    break;
                }
                current = &next->link;
                next = next->link;
            }
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
   
         } else { // if thread is monitoring an alarm with id = second_id
            now = time(NULL);
            second = search_a(second_id); 
            if(second != NULL){
                if (second->time <= now){
                    // raise alarm and purge from list
                    printf ("\r(%d) %s\n", second->seconds, second->message);
                    remove_a(second->id); // thread safe; mutex is inside method
                    thread->second = NULL;
            
                } else if(counter >= 5) {
#ifdef DEBUG
                     printf(
                        "Alarm %d printed by Alarm Display Thread %d at %d: %d %s\n",
                        second->id,
                        thread->id,
                        time(NULL),
                        next->time - time(NULL),
                        next->message
                    );
#endif
                }
            } else { // deleted from master list
#ifdef DEBUG
                printf(
                    "Display Alarm Thread %d Removed Alarm %d at %d: %d %s)\n",
                    thread->id,
                    second->id,
                    time(NULL),
                    next->time - time(NULL),
                    next->message
                );
#endif
                thread->second = NULL;
            }
        }
        if(thread->first == NULL && thread->second == NULL){
#ifdef DEBUG
            printf(
                "Display Alarm Thread %d Exiting at %d\n",
                thread->id,
                time(NULL)
            );
#endif
            remove_thread(thread->id);
            pthread_exit(0);
        }
        // move to bottom of scheduling queue, allow main thread to take priority
        sched_yield();
    }
}


int main (int argc, char *argv[]) {
    // method wide variable declaration
    int status;
    char line[128];
    alarm_t *alarm, **current, *next;
    while (1) {
        printf("\ralarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        // parse line, attempt to match input with regex
        regex_t add, change, view, cancel;
        regmatch_t groups[4];
        strtok(line, "\n");
        regcomp(&add, "Start_Alarm\\(([0-9]+)\\): ([0-9]+) (.+)", REG_EXTENDED);
        regcomp(&change, "Change_Alarm\\(([0-9]+)\\): ([0-9]+) (.+)", REG_EXTENDED);
        regcomp(&cancel, "Cancel_Alarm\\(([0-9]+)\\)", REG_EXTENDED);
        regcomp(&view, "View_Alarms", REG_EXTENDED);
        // add alarm case
        if(regexec(&add, line, 0, groups, 0) == 0){
            alarm = (alarm_t*)malloc (sizeof (alarm_t));
            if (alarm == NULL)
                errno_abort ("Allocate alarm");
            /*
            * Parse input line into seconds (%d) and a message
            * (%64[^\n]), consisting of up to 64 characters
            * separated from the seconds by whitespace.
            */
            sscanf(line, "Start_Alarm(%d): %d %64[^\n]",&alarm->id, &alarm->seconds, alarm->message);
            if ((search_a(alarm->id) != NULL) || (alarm->seconds <= 2)){
                fprintf (stderr, "Bad command\n");
                free (alarm);
            } else {
                alarm->time = time(NULL) + alarm->seconds;
#ifdef DEBUG
                printf(
                    "Alarm %d inserted into Alarm List at %d: %d %s\n",
                    alarm->id, time(NULL),
                    alarm->time - time(NULL),
                    alarm->message
                );
#endif
                 // create thread if all are occupied
                if(2*(thread_count()) <= alarms_count()){
                    // define a thread object, which is a linked list of threads
                    // that contain a pointer to a thread along with other thread related info
                    thread_t *temp = (thread_t*)malloc (sizeof (thread_t));
                    if(temp == NULL){
                        errno_abort("Allocated thread");
                    }
                    temp->id = thread_count() + 1;
                    temp->first = NULL;
                    temp->second = NULL;
                    temp->link = NULL;
                    status = pthread_create(&temp->thread, NULL, alarm_thread, temp);
                    if (status != 0)
                        err_abort (status, "Create alarm thread");
                    printf(
                        "New Display Alarm Thread %d Created at %d: %d %s\n",
                        temp->id,
                        time(NULL),
                        alarm->time - time(NULL),
                        alarm->message
                    );
                    append_thread(temp);
                }
                insert_a(alarm);
            }
        }
        // change alarm case
        else if(regexec(&change, line, 0, NULL, 0) != REG_NOMATCH){
            alarm = (alarm_t*)malloc (sizeof (alarm_t));
            if (alarm == NULL)
                errno_abort ("Allocate alarm");
            /*
            * Parse input line into seconds (%d) and a message
            * (%64[^\n]), consisting of up to 64 characters
            * separated from the seconds by whitespace.
            */
            sscanf(
                line,
                "Change_Alarm(%d): %d %64[^\n]",
                &alarm->id,
                &alarm->seconds,
                alarm->message
            );
            alarm_t * temp = search_a(alarm->id);
            if ((alarm->seconds <= 2) || temp == NULL){
                fprintf (stderr, "Bad command\n");
            } else {
                // creates an alarm object, remaps old values into new location
                // then deletes local alarm object
                temp->seconds = alarm->seconds;
                strcpy(temp->message,alarm->message);
                temp->time = time(NULL) + alarm->seconds;
                printf(
                    "Alarm %d changed at %d: %d %s\n",
                    alarm->id,
                    time(NULL),
                    alarm->time - time(NULL),
                    alarm->message
                );
            }
            free (alarm);
        }
        // remove alarm case
        else if (regexec(&cancel, line, 0, NULL, 0) != REG_NOMATCH){
            int id;
            sscanf(line, "Cancel_Alarm(%d)", &id);
            if(search_a(id) == NULL){
                fprintf (stderr, "Bad command\n");
            } else {
                // removes alarm using mutex safe remove_a method
                // allows the alarm threads to manually check if their alarms
                // still exist
                alarm_t * temp = search_a(id);
                printf(
                    "Alarm %d Canceled at %d: %d %s\n",
                    id,
                    time(NULL),
                    temp->time - time(NULL),
                    temp->message
                );
                remove_a(id);
            }
        }
        // view alarms case
        else if(regexec(&view, line, 0, NULL, 0) != REG_NOMATCH){
            status = pthread_mutex_lock (&thread_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
            thread_t *thread;
            // loops through the threads linked list, and prints out all
            // cooresponding information
            for (thread = thread_list; thread != NULL; thread = thread->link) {
                printf("Display Thread %d Assigned:\n", thread->id);
                if(thread->first != NULL){
                    printf(
                        "\t%da. Alarm %d: Created at %d, Assigned at %d %d %s\n",
                        thread->id,
                        thread->first->id,
                        thread->first->time,
                        thread->first->time,
                        thread->first->time - time(NULL),
                        thread->first->message
                    );
                }
                if(thread->second != NULL){
                    printf(
                        "\t%db. Alarm %d: Created at %d, Assigned at %d %d %s\n",
                        thread->id,
                        thread->second->id,
                        thread->second->time,
                        thread->second->time,
                        thread->second->time - time(NULL),
                        thread->second->message
                    );
                }
            }
            status = pthread_mutex_unlock (&thread_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }else{
            printf("No Match\n", NULL);
        }
    }
}
