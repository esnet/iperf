//
// Created by hackler on 24.08.22.
//
#include <setjmp.h>

#include "iperf.h"
#ifndef IPERF_MAIN_H
#define IPERF_MAIN_H
extern jmp_buf jmp_bf;
extern struct iperf_test *test;
int main(int argc, char **argv);
void stopRun();
#endif //IPERF_MAIN_H