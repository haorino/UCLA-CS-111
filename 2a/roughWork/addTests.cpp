#include <stdlib.h>
#include <stdio.h>
#include <unordered_set.h>
#include <string.h>

using namespace std;

void main()
{
    int threads, iterations, sync, yield;
    //lab2_add tests - without yield

    unordered_set<string> tests;
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        for (iterations = 100; iterations <= 100000; iterations *= 10)
            tests.insert("./lab2_add --threads " + threads + " --iterations " + iterations + "\n");
    }

    //lab2_add tests - with yield
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        for (iterations = 10; iterations <= 100000; iterations *= 10)
            tests.insert("./lab2_add --threads " + threads + " --iterations " + iterations + " --yield >> lab2_add.csv \n");
        for (iterations = 20; iterations < 100; iterations *= 2)
            tests.insert("./lab2_add --threads " + threads + " --iterations " + iterations + " --yield >> lab2_add.csv \n");
    }

    //lab2_addtests - with sync mutex
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        tests.insert("./lab2_add --threads " + threads + " --iterations 10000" + " --yield --sync m >> lab2_add.csv \n");
    }

    //lab2_addtests - with sync compare and swap
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        tests.insert("./lab2_add --threads " + threads + " --iterations 10000" + " --yield --sync c >> lab2_add.csv \n");
    }

    //lab2_addtests - with sync spin lock
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        tests.insert("./lab2_add --threads " + threads + " --iterations 1000" + " --yield --sync s >> lab2_add.csv\n");
    }

    //lab2_addtests - with sync spin lock
    for (threads = 2; threads <= 12; threads += 2)
    {
        if (threads == 6 || threads == 10)
            continue;
        tests.insert("./lab2_add --threads " + threads + " --iterations 10000" + " --yield --sync s >> lab2_add.csv \n");
    }

    
    
}