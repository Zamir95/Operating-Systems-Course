.DEFAULT_GOAL := compile
MAKEFLAGS += --silent

compile:
	cc -pthread -w -D_POSIX_PTHREAD_SEMANTICS -std=gnu99 New_alarm_mutex.c